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
#include "ZCDtoPWM.h"
#include "autotune.h"
#include "cli_common.h"
#include <device.h>
#include <math.h>

uint16 int1_prd, int1_cmp, int2_prd, int2_cmp, int3_prd, int3_cmp;

uint16 min_tr_prd;
uint8_t tr_running = 0;

volatile uint8 qcw_dl_ovf_counter;

uint8_t blocked=0; 

extern const uint8_t kill_msg[3];

CY_ISR(int_wd_ISR) {
	interrupter1_control_Control = 0;
	QCW_enable_Control = 0;
	ramp.modulation_value = 0;
    interrupter_kill();
    USBMIDI_1_callbackLocalMidiEvent(0, (uint8_t*)kill_msg);
}

CY_ISR(qcw_end_ISR) {
	if (qcw_reg && ramp.modulation_value > 0) {
		qcw_reg = 0;
		ramp.modulation_value = 0;
		ramp_control();
	}
	QCW_enable_Control = 0;
}

CY_ISR(qcw_dl_ovf_ISR) {
	qcw_dl_ovf_counter = 1;
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

	QCW_duty_limiter_WritePeriod(65535);
	QCW_duty_limiter_WriteCompare(65535 - configuration.max_qcw_pw); //this register sets the max PW output

	int1_prd = 65000;
	int1_cmp = 64999;
	int2_prd = 65000;
	int2_cmp = 64999;
	ramp.modulation_value = 0;
	ramp.modulation_value_previous = 0;

	//Start up timers
	interrupter1_Start();
	QCW_duty_limiter_Start();


	qcw_dl_ovf_StartEx(qcw_dl_ovf_ISR);
	qcw_end_StartEx(qcw_end_ISR);

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

void ramp_control(void) {
	if (ramp.modulation_value != 0  && !blocked) {
		if (ramp.modulation_value_previous == 0) //indicates a new QCW pulse starting
		{
			//before starting the QCW pulse, capture the previous QCW period from the QCW_duty_limiter PWM, then calculate the permissible PW for that PWM
			if (qcw_dl_ovf_counter)
				ramp.qcw_recorded_prd = 65535; //if there was an overflow of this PWM, then the period is > 655mS, so clamp it there
			else
				ramp.qcw_recorded_prd = 65535 - QCW_duty_limiter_ReadCounter(); //else, look at the counter to see where we are at since last pulse
			qcw_dl_ovf_counter = 0;
			ramp.qcw_limiter_pw = (ramp.qcw_recorded_prd * configuration.max_qcw_duty) / 1000;
			if ((ramp.qcw_limiter_pw > configuration.max_qcw_pw) || (ramp.qcw_limiter_pw == 0)) {
				ramp.qcw_limiter_pw = configuration.max_qcw_pw;
			}
			//the next stuff is time sensitive, so disable interrupts to avoid glitches
			CyGlobalIntDisable;
			QCW_duty_limiter_WriteCounter(65535);						//restart counter at top value
			QCW_duty_limiter_WriteCompare(65535 - ramp.qcw_limiter_pw); //put in limiting compare match value
			//now enable the QCW interrupter
			QCW_enable_Control = 1;
			//------------------>watchdog_reset_Control = 1;
			//------------------>watchdog_reset_Control = 0;
			params.pwmb_psb_val = params.pwm_top - params.pwmb_start_psb_val;
			CyGlobalIntEnable;
		} else {
			//linearize modulation value based on fb_filter_out period
			ramp.shift_period = (ramp.modulation_value * (params.pwm_top - fb_filter_out)) >> 8;
			//assign new modulation value to the params.pwmb_psb_val ram
			if ((ramp.shift_period + params.pwmb_start_psb_val) > (params.pwmb_start_prd - 4)) {
				params.pwmb_psb_val = 4;
			} else {
				params.pwmb_psb_val = params.pwm_top - (ramp.shift_period + params.pwmb_start_psb_val);
			}
		}
	} else //(ramp.modulation_value == 0) //indicates the switching should stop
	{
		QCW_enable_Control = 0;
	}
	//grab the old modulation value so it can be used to detect start of ramp
	ramp.modulation_value_previous = ramp.modulation_value;
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
	return;
}

void update_interrupter() {
    if(blocked){
        interrupter.pw=0;
    }
    
	/* Check if PW = 0, this indicates that the interrupter should be shut off */
	if (interrupter.pw == 0) {
		interrupter1_control_Control = 0b00;
		interrupter.duty = 0;
	}

	/* Check the pulsewidth command */
	if (interrupter.pw > configuration.max_tr_pw) {
		interrupter.pw = configuration.max_tr_pw;
	}

	/* Check that the period is long enough to meet restrictions, if not, scale it */
	if (interrupter.prd < params.min_tr_prd) {
		interrupter.prd = params.min_tr_prd;
	}

	/* Compute the duty cycle and mod the PW if required */
	interrupter.duty = (uint32)((uint32)interrupter.pw * 1000ul) / interrupter.prd; //gives duty cycle as 0.1% increment
	if (interrupter.duty > configuration.max_tr_duty - param.temp_duty) {
		interrupter.duty_limited_pw = (uint32)(configuration.max_tr_duty - param.temp_duty) * (uint32)interrupter.prd / 1000ul;
	} else {
		interrupter.duty_limited_pw = interrupter.pw;
	}
	ct1_dac_val[0] = params.max_tr_cl_dac_val;

	/* Update interrupter registers */
	CyGlobalIntDisable;
	if (interrupter.pw != 0) {
		int1_prd = interrupter.prd - 3;
		int1_cmp = interrupter.prd - interrupter.duty_limited_pw - 3;
		if (interrupter1_control_Control == 0) {
			interrupter1_control_Control = 0b11;
			interrupter1_control_Control = 0b10;
		}
	}
	CyGlobalIntEnable;
}
