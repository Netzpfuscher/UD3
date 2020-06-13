/*
 * UD3
 *
 * Copyright (c) 2018 Jens Kerrinnes
 * Copyright (c) 2015 Steve Ward
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "interrupter.h"
#include "hardware.h"
#include "ZCDtoPWM.h"
#include "autotune.h"
#include "cli_common.h"
#include <device.h>
#include <math.h>

uint16 int1_prd, int1_cmp;

uint8_t tr_running = 0;

uint8_t blocked=0; 


void handle_qcw(){
    if(SG_Timer_ReadCounter() < timer.time_stop){
        qcw_reg = 0;
		ramp.modulation_value = 0;
        QCW_enable_Control = 0;
        return;
    }
    
    if (ramp.modulation_value < 255) {
		ramp.modulation_value += param.qcw_ramp;
		if (ramp.modulation_value > 255)
			ramp.modulation_value = 255;
        qcw_modulate(ramp.modulation_value);
	}
}

void handle_qcw_synth(){
    if(SG_Timer_ReadCounter() < timer.time_stop){
        QCW_enable_Control = 0;
        return;
    }
}


//CY_ISR(OCD_ISR)
//{
//    //QCW_enable_Control = 0;
//}

uint8_t interrupter_get_kill(void){
    return blocked;   
}

void interrupter_kill(void){
    blocked=1;
    tr_running=0;
    interrupter.pw =0;
    param.pw=0;
    update_interrupter();
}

void interrupter_unkill(void){
    blocked=0;
}

void initialize_interrupter(void) {

	//over current detection
	//    OCD_StartEx(OCD_ISR);

	//initialize the PWM generators for safe PW and PRD
	interrupter1_WritePeriod(65000);
	interrupter1_WriteCompare2(65000);
	interrupter1_WriteCompare1(64999);

	int1_prd = 65000;
	int1_cmp = 64999;
	ramp.modulation_value = 0;

	//Start up timers
	interrupter1_Start();

	params.min_tr_prd = INTERRUPTER_CLK_FREQ / configuration.max_tr_prf;

	/* Variable declarations for int1_dma */
	uint8 int1_dma_Chan;
	uint8 int1_dma_TD[4];
	int1_dma_Chan = int1_dma_DmaInitialize(int1_dma_BYTES_PER_BURST, int1_dma_REQUEST_PER_BURST,
										   HI16(int1_dma_SRC_BASE), HI16(int1_dma_DST_BASE));
	int1_dma_TD[0] = CyDmaTdAllocate();
	int1_dma_TD[1] = CyDmaTdAllocate();
	int1_dma_TD[2] = CyDmaTdAllocate();
	int1_dma_TD[3] = CyDmaTdAllocate();
	CyDmaTdSetConfiguration(int1_dma_TD[0], 2, int1_dma_TD[1], int1_dma__TD_TERMOUT_EN | TD_AUTO_EXEC_NEXT);
	CyDmaTdSetConfiguration(int1_dma_TD[1], 2, int1_dma_TD[2], int1_dma__TD_TERMOUT_EN | TD_AUTO_EXEC_NEXT);
	CyDmaTdSetConfiguration(int1_dma_TD[2], 2, int1_dma_TD[3], int1_dma__TD_TERMOUT_EN | TD_AUTO_EXEC_NEXT);
	CyDmaTdSetConfiguration(int1_dma_TD[3], 2, int1_dma_TD[0], int1_dma__TD_TERMOUT_EN);
	CyDmaTdSetAddress(int1_dma_TD[0], LO16((uint32)&int1_prd), LO16((uint32)interrupter1_PERIOD_LSB_PTR));
	CyDmaTdSetAddress(int1_dma_TD[1], LO16((uint32)&int1_cmp), LO16((uint32)interrupter1_COMPARE1_LSB_PTR));
	CyDmaTdSetAddress(int1_dma_TD[2], LO16((uint32)&int1_prd), LO16((uint32)interrupter1_COMPARE2_LSB_PTR));
	CyDmaTdSetAddress(int1_dma_TD[3], LO16((uint32)&int1_prd), LO16((uint32)interrupter1_COUNTER_LSB_PTR));
	CyDmaChSetInitialTd(int1_dma_Chan, int1_dma_TD[0]);
	CyDmaChEnable(int1_dma_Chan, 1);
}

void qcw_start(){
    timer.time_start = SG_Timer_ReadCounter();
    uint32_t sg_period = ((1000UL*SG_Trim_ReadPeriod())/BCLK__BUS_CLK__MHZ); //ns
    uint32_t cycles_to_stop = (configuration.max_qcw_pw*10000)/sg_period;
    uint32_t cycles_since_last_pulse = timer.time_stop-timer.time_start;
    
    uint32_t cycles_to_stop_limited = (cycles_since_last_pulse * configuration.max_qcw_duty) / 500;
	if ((cycles_to_stop_limited > cycles_to_stop) || (cycles_to_stop_limited == 0)) {
		cycles_to_stop_limited = cycles_to_stop;
	}

    timer.time_stop = timer.time_start - cycles_to_stop_limited;
    
	//the next stuff is time sensitive, so disable interrupts to avoid glitches
	CyGlobalIntDisable;
	//now enable the QCW interrupter
	QCW_enable_Control = 1;
	params.pwmb_psb_val = params.pwm_top - params.pwmb_start_psb_val;
	CyGlobalIntEnable;
}

void qcw_modulate(uint16_t val){
#if USE_DEBUG_DAC  
    if(QCW_enable_Control) DEBUG_DAC_SetValue(val); 
#endif
    //linearize modulation value based on fb_filter_out period
	uint16_t shift_period = (val * (params.pwm_top - fb_filter_out)) >> 8;
	//assign new modulation value to the params.pwmb_psb_val ram
	if ((shift_period + params.pwmb_start_psb_val) > (params.pwmb_start_prd - 4)) {
		params.pwmb_psb_val = 4;
	} else {
		params.pwmb_psb_val = params.pwm_top - (shift_period + params.pwmb_start_psb_val);
	}  
}

void qcw_stop(){
    qcw_reg = 0;
#if USE_DEBUG_DAC 
    DEBUG_DAC_SetValue(0);
#endif
    QCW_enable_Control = 0;
}

void interrupter_oneshot(uint16_t pw, uint8_t vol) {
    if(blocked) return;
    
	if (vol < 127) {
		ct1_dac_val[0] = params.min_tr_cl_dac_val + ((vol * params.diff_tr_cl_dac_val) >> 7);
	} else {
		ct1_dac_val[0] = params.max_tr_cl_dac_val;
	}
	uint16_t prd;
	if (pw > configuration.max_tr_pw) {
		pw = configuration.max_tr_pw;
	}
	prd = 60000;
	/* Update Interrupter PWMs with new period/pw */
	CyGlobalIntDisable;
	int1_prd = prd - 3;
	int1_cmp = prd - pw - 3;
	interrupter1_control_Control = 0b01;
	interrupter1_control_Control = 0b00;
    CyGlobalIntEnable;
}

void update_interrupter() {
    
	/* Check if PW = 0, this indicates that the interrupter should be shut off */
	if (interrupter.pw == 0 || blocked) {
		interrupter1_control_Control = 0b00;
        return;
	}
    uint16_t limited_pw;
    
	/* Check the pulsewidth command */
	if (interrupter.pw > configuration.max_tr_pw) {
		interrupter.pw = configuration.max_tr_pw;
	}

	/* Check that the period is long enough to meet restrictions, if not, scale it */
	if (interrupter.prd < params.min_tr_prd) {
		interrupter.prd = params.min_tr_prd;
	}

	/* Compute the duty cycle and mod the PW if required */
	limited_pw = (uint32)((uint32)interrupter.pw * 1000ul) / interrupter.prd; //gives duty cycle as 0.1% increment
	if (limited_pw > configuration.max_tr_duty - param.temp_duty) {
		limited_pw = (uint32)(configuration.max_tr_duty - param.temp_duty) * (uint32)interrupter.prd / 1000ul;
	} else {
		limited_pw = interrupter.pw;
	}
	ct1_dac_val[0] = params.max_tr_cl_dac_val;

	/* Update interrupter registers */
	CyGlobalIntDisable;
	if (interrupter.pw != 0) {
		int1_prd = interrupter.prd - 3;
		int1_cmp = interrupter.prd - limited_pw - 3;
		if (interrupter1_control_Control == 0) {
			interrupter1_control_Control = 0b11;
			interrupter1_control_Control = 0b10;
		}
	}
	CyGlobalIntEnable;
}

