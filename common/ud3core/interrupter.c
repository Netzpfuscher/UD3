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
#include "SignalGenerator.h"
#include "cli_common.h"
#include "qcw.h"
#include "tasks/tsk_fault.h"
#include "tasks/tsk_midi.h"

#include <device.h>
#include <math.h>

uint16_t int1_prd, int1_cmp;
uint16_t ch_prd[N_CHANNEL], ch_cmp[N_CHANNEL];
uint8_t ch_cur[N_CHANNEL];

void interrupter_kill(void){
    sysfault.interlock = 1;
    interrupter.mode = INTR_MODE_OFF;
    interrupter.pw =0;
    param.pw=0;
    update_interrupter();
}

void interrupter_unkill(void){
    sysfault.interlock=0;
}

uint8 int1_dma_Chan;


#define N_TD 4
uint8 ch_dma_Chan[N_CHANNEL];
uint8_t ch_dma_TD[N_CHANNEL][N_TD];

#define MODULATION_CUR_BYTES 1
#define MODULATION_PW_BYTES  8
#define MODULATION_REQUEST_PER_BURST  1

void interrupter_reconf_dma(enum interrupter_modulation mod){
    if(mod==INTR_MOD_PW){
        
    ch_dma_Chan[0] = Ch1_DMA_DmaInitialize(MODULATION_PW_BYTES, MODULATION_REQUEST_PER_BURST,
										   HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    ch_dma_Chan[1] = Ch2_DMA_DmaInitialize(MODULATION_PW_BYTES, MODULATION_REQUEST_PER_BURST,
										   HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    ch_dma_Chan[2] = Ch3_DMA_DmaInitialize(MODULATION_PW_BYTES, MODULATION_REQUEST_PER_BURST,
										   HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    ch_dma_Chan[3] = Ch4_DMA_DmaInitialize(MODULATION_PW_BYTES, MODULATION_REQUEST_PER_BURST,
										   HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    for(uint8_t ch=0;ch<N_CHANNEL;ch++){
        CyDmaTdSetConfiguration(ch_dma_TD[ch][0], 2, ch_dma_TD[ch][1], TD_AUTO_EXEC_NEXT);
	    CyDmaTdSetConfiguration(ch_dma_TD[ch][1], 2, ch_dma_TD[ch][2], TD_AUTO_EXEC_NEXT);
	    CyDmaTdSetConfiguration(ch_dma_TD[ch][2], 2, ch_dma_TD[ch][3], TD_AUTO_EXEC_NEXT);
	    CyDmaTdSetConfiguration(ch_dma_TD[ch][3], 2, ch_dma_TD[ch][0], Ch1_DMA__TD_TERMOUT_EN);  
        
        CyDmaTdSetAddress(ch_dma_TD[ch][0], LO16((uint32)&ch_prd[ch]), LO16((uint32)interrupter1_PERIOD_LSB_PTR));
    	CyDmaTdSetAddress(ch_dma_TD[ch][1], LO16((uint32)&ch_cmp[ch]), LO16((uint32)interrupter1_COMPARE1_LSB_PTR));
    	CyDmaTdSetAddress(ch_dma_TD[ch][2], LO16((uint32)&ch_prd[ch]), LO16((uint32)interrupter1_COMPARE2_LSB_PTR));
    	CyDmaTdSetAddress(ch_dma_TD[ch][3], LO16((uint32)&ch_prd[ch]), LO16((uint32)interrupter1_COUNTER_LSB_PTR));
        
        CyDmaChSetInitialTd(ch_dma_Chan[ch], ch_dma_TD[ch][0]);
    }
    
    }else if(mod==INTR_MOD_CUR){
    
    ch_dma_Chan[0] = Ch1_DMA_DmaInitialize(MODULATION_CUR_BYTES, MODULATION_REQUEST_PER_BURST,
										   HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    ch_dma_Chan[1] = Ch2_DMA_DmaInitialize(MODULATION_CUR_BYTES, MODULATION_REQUEST_PER_BURST,
										   HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    ch_dma_Chan[2] = Ch3_DMA_DmaInitialize(MODULATION_CUR_BYTES, MODULATION_REQUEST_PER_BURST,
										   HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    ch_dma_Chan[3] = Ch4_DMA_DmaInitialize(MODULATION_CUR_BYTES, MODULATION_REQUEST_PER_BURST,
										   HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    
    for(uint8_t ch=0;ch<N_CHANNEL;ch++){
        CyDmaTdSetConfiguration(ch_dma_TD[ch][0], 1, ch_dma_TD[ch][0], Ch1_DMA__TD_TERMOUT_EN);
        
        CyDmaTdSetAddress(ch_dma_TD[ch][0], LO16((uint32)&ch_cur[ch]), LO16((uint32)&ct1_dac_val[0]));
        
        CyDmaChSetInitialTd(ch_dma_Chan[ch], ch_dma_TD[ch][0]);
    }
        
    }
}

// Adds an alarm with the specified message to the alarm queue and stops the UD3
// TODO: Make this a global function so it can be used everywhere resources and memory are allocated.
void critical_error(const char *message, int32_t val) {
    alarm_push(ALM_PRIO_CRITICAL, message, val);
    interrupter_kill();
}

// One-time initialization of the interrupter components (called once at system startup)
void initialize_interrupter(void) {
	//over current detection
	//    OCD_StartEx(OCD_ISR);

	//initialize the PWM generators for safe PW and PRD
	int1_prd = 65000;   // An arbitrary large number to count down from
	int1_cmp = 64999;
    
	interrupter1_WritePeriod(int1_prd);
	interrupter1_WriteCompare1(int1_cmp);      // pwm1 will be true for one clock, then false.  int1_prd - int1_cmp is the on time for the pulse.
  	interrupter1_WriteCompare2(int1_prd);      // pwm2 will be false for one clock then true.  This creates the int1_fin signal.

	//Start up timers
	interrupter1_Start();
    
	// Variable declarations for int1_dma.  This transfers int1_prd and int1_cmp to the interrupter1 component.
	uint8_t int1_dma_TD[4];
	int1_dma_Chan = int1_dma_DmaInitialize(int1_dma_BYTES_PER_BURST, int1_dma_REQUEST_PER_BURST,
										   HI16(int1_dma_SRC_BASE), HI16(int1_dma_DST_BASE));
    
    for(int i=0; i<4; ++i){
    	int1_dma_TD[i] = CyDmaTdAllocate();
        if(int1_dma_TD[i] == DMA_INVALID_TD)
            critical_error("CyDmaTdAllocate failure INT", i);
    }
    
	CyDmaTdSetConfiguration(int1_dma_TD[0], 2, int1_dma_TD[1], int1_dma__TD_TERMOUT_EN | TD_AUTO_EXEC_NEXT);
	CyDmaTdSetConfiguration(int1_dma_TD[1], 2, int1_dma_TD[2], int1_dma__TD_TERMOUT_EN | TD_AUTO_EXEC_NEXT);
	CyDmaTdSetConfiguration(int1_dma_TD[2], 2, int1_dma_TD[3], int1_dma__TD_TERMOUT_EN | TD_AUTO_EXEC_NEXT);
	CyDmaTdSetConfiguration(int1_dma_TD[3], 2, int1_dma_TD[0], int1_dma__TD_TERMOUT_EN);
    
	CyDmaTdSetAddress(int1_dma_TD[0], LO16((uint32)&int1_prd), LO16((uint32)interrupter1_PERIOD_LSB_PTR));
	CyDmaTdSetAddress(int1_dma_TD[1], LO16((uint32)&int1_cmp), LO16((uint32)interrupter1_COMPARE1_LSB_PTR));
	CyDmaTdSetAddress(int1_dma_TD[2], LO16((uint32)&int1_prd), LO16((uint32)interrupter1_COMPARE2_LSB_PTR));
	CyDmaTdSetAddress(int1_dma_TD[3], LO16((uint32)&int1_prd), LO16((uint32)interrupter1_COUNTER_LSB_PTR));
	CyDmaChSetInitialTd(int1_dma_Chan, int1_dma_TD[0]);
    
    //Allocate memory for DDS DMA TDs
    for(uint8_t ch=0;ch<N_CHANNEL;ch++){
        for(uint8_t i=0;i<N_TD;i++){
            ch_dma_TD[ch][i] = CyDmaTdAllocate();
            if(ch_dma_TD[ch][i] ==  DMA_INVALID_TD)
                critical_error("CyDmaTdAllocate failure DDS", i);
         }
    }

    DDS32_1_Start();
    DDS32_2_Start();

    configure_interrupter();
}

void interrupter_init_safe(){
        // Safe defaults in case anything fails.
  	int1_prd = 65000;
	int1_cmp = 64999;

  	//initialize the PWM generators for safe PW and PRD
	interrupter1_WritePeriod(int1_prd);
	interrupter1_WriteCompare1(int1_cmp);      // pwm1 will be true for one clock, then false.  int1_prd - int1_cmp is the on time for the pulse.
  	interrupter1_WriteCompare2(int1_prd);      // pwm2 will be false for one clock then true
  
}

// Called whenever an interrupter-related param is changed oe eeprom is loaded.
void configure_interrupter()
{

    //Init interrupter to some safe values
    interrupter_init_safe();

    // The minimum interrupter period for transient mode in clock ticks (which are equal to microseconds here).
	params.min_tr_prd = INTERRUPTER_CLK_FREQ / configuration.max_tr_prf;
    
    // Disable interrupter
    interrupter.mode = INTR_MODE_OFF;
    interrupter_reconf_dma(interrupter.mod);    // May be changed when eeprom is loaded or interrupter.mod is changed via cli
   
    DDS32_1_Disable_ch(0);
    DDS32_1_Disable_ch(1);
    DDS32_2_Disable_ch(0);
    DDS32_2_Disable_ch(1);
    
    interrupter_DMA_mode(INTR_DMA_DDS);         // set to synth mode by default
}


void interrupter_DMA_mode(enum interrupter_DMA mode){
    interrupter.dma_mode = mode;
    switch(mode){
        case INTR_DMA_TR:
            CyDmaChDisable(ch_dma_Chan[0]);
            CyDmaChDisable(ch_dma_Chan[1]);
            CyDmaChDisable(ch_dma_Chan[2]);
            CyDmaChDisable(ch_dma_Chan[3]);
            CyDmaChEnable(int1_dma_Chan, 1);
        break;
        case INTR_DMA_DDS:
            if(interrupter.mod == INTR_MOD_PW){
                CyDmaChDisable(int1_dma_Chan);
            }else{
                CyDmaChEnable(int1_dma_Chan, 1);
            }
            CyDmaChEnable(ch_dma_Chan[0], 1);
            CyDmaChEnable(ch_dma_Chan[1], 1);
            CyDmaChEnable(ch_dma_Chan[2], 1);
            CyDmaChEnable(ch_dma_Chan[3], 1);
        break;
    }
}

void interrupter_set_pw(uint8_t ch, uint16_t pw){
    if(ch<N_CHANNEL){
        ct1_dac_val[0] = params.max_tr_cl_dac_val;
        uint16_t prd = param.offtime + pw;
        ch_prd[ch] = prd - 3;
        ch_cmp[ch] = prd - pw - 3; 
    }
}

void interrupter_set_pw_vol(uint8_t ch, uint16_t pw, uint32_t vol){
    if(ch<N_CHANNEL){
        
        if(pw>configuration.max_tr_pw) pw = configuration.max_tr_pw;
        
        if(interrupter.mod==INTR_MOD_PW){
            if(vol>MAX_VOL)vol=MAX_VOL;
            uint32_t temp = ((uint32_t)pw*(vol>>6))>>17;
            if(temp > configuration.max_tr_pw) temp = configuration.max_tr_pw;
            uint16_t prd = param.offtime + temp;
            ch_prd[ch] = prd - 3;
            ch_cmp[ch] = prd - temp - 3; 
        }else{
            if (vol < MAX_VOL) {
        		ch_cur[ch] = params.min_tr_cl_dac_val + ((vol * params.diff_tr_cl_dac_val) >> 23);
                if(ch_cur[ch]>params.max_tr_cl_dac_val) ch_cur[ch] = params.max_tr_cl_dac_val;
        	} else {
        		ch_cur[ch] = params.max_tr_cl_dac_val;
        	}
            uint16_t prd = param.offtime + pw;
            int1_prd = prd - 3;
	        int1_cmp = prd - pw - 3;
        }
    }
}



void interrupter_oneshot(uint32_t pw, uint32_t vol) {
    if(sysfault.interlock) return;
    
	if (vol < MAX_VOL) {
		ct1_dac_val[0] = params.min_tr_cl_dac_val + ((vol * params.diff_tr_cl_dac_val) >> 23);
        if(ct1_dac_val[0] > params.max_tr_cl_dac_val) ct1_dac_val[0] = params.max_tr_cl_dac_val;
	} else {
		ct1_dac_val[0] = params.max_tr_cl_dac_val;
	}
	uint16_t prd;
	if (pw > configuration.max_tr_pw) {
		pw = configuration.max_tr_pw;
	}
    prd = param.offtime + pw;
	/* Update Interrupter PWMs with new period/pw */
	CyGlobalIntDisable;
	int1_prd = prd - 3;
	int1_cmp = prd - pw - 3;
	interrupter1_control_Control = 0b0001;
	interrupter1_control_Control = 0b0000;
    CyGlobalIntEnable;
}


void interrupter_update_ext() {

	ct1_dac_val[0] = params.max_tr_cl_dac_val;
    
    uint32_t pw = configuration.max_tr_pw;
    //uint32_t pw = param.pw;
    
    //if (pw > configuration.max_tr_pw) {
	//	pw = configuration.max_tr_pw;
	//}

    uint16_t prd = param.offtime + pw;
	/* Update Interrupter PWMs with new period/pw */
	CyGlobalIntDisable;
	int1_prd = prd - 3;
	int1_cmp = prd - pw - 3;
    
    if(configuration.ext_interrupter==1){
	    interrupter1_control_Control = 0b1100;
    }else{
        interrupter1_control_Control = 0b0100;
    }
    CyGlobalIntEnable;
}

uint8_t callback_ext_interrupter(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    if(configuration.ext_interrupter){
        alarm_push(ALM_PRIO_WARN, "INT: External interrupter active", configuration.ext_interrupter);
        interrupter_update_ext();
    }else{
        uint8 sfflag = system_fault_Read();
        sysflt_set(pdFALSE); //halt tesla coil operation during updates!
        interrupter1_control_Control = 0b0000;
        system_fault_Control = sfflag;
    }
    return pdPASS;
}

uint8_t callback_interrupter_mod(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    uint8 sfflag = system_fault_Read();
    sysflt_set(pdFALSE); //halt tesla coil operation during updates!
    interrupter_reconf_dma(interrupter.mod);
    if(interrupter.mode!=INTR_MODE_OFF){
        interrupter_DMA_mode(INTR_DMA_TR);
    }else{
        interrupter_DMA_mode(INTR_DMA_DDS);
    }
    
    system_fault_Control = sfflag;
    return pdPASS;
}

void update_interrupter() {
    
	/* Check if PW = 0, this indicates that the interrupter should be shut off */
	if (interrupter.pw == 0 || sysfault.interlock) {
		interrupter1_control_Control = 0b0000;
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
	limited_pw = (uint32_t)((uint32_t)interrupter.pw * 1000ul) / interrupter.prd; //gives duty cycle as 0.1% increment
	if (limited_pw > configuration.max_tr_duty - param.temp_duty) {
		limited_pw = (uint32_t)(configuration.max_tr_duty - param.temp_duty) * (uint32_t)interrupter.prd / 1000ul;
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
			interrupter1_control_Control = 0b0011;
			interrupter1_control_Control = 0b0010;
		}
	}
	CyGlobalIntEnable;
}

/*****************************************************************************
* Callback if a transient mode parameter is changed
* Updates the interrupter hardware
******************************************************************************/
uint8_t callback_TRFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle) {
    
    uint32_t temp = (1000 * param.pw) / configuration.max_tr_pw;
    param.pwp = temp;
    
    switch(interrupter.mode){
        case INTR_MODE_OFF:
        
        break;
        case INTR_MODE_TR:
        	interrupter.pw = param.pw;
        	interrupter.prd = param.pwd;
            update_interrupter();
        break;
        default:
            if(configuration.ext_interrupter  && param.synth == SYNTH_OFF){
                interrupter_update_ext();
            }else if (param.synth == SYNTH_MIDI || param.synth == SYNTH_MIDI_QCW){
                SigGen_limit();
            }
        break; 
    }

	return pdPASS;
}

/*****************************************************************************
* Callback if a transient mode parameter is changed (percent ontime)
* Updates the interrupter hardware
******************************************************************************/
uint8_t callback_TRPFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle) {
    
    uint32_t temp;
    temp = (configuration.max_tr_pw * param.pwp) / 1000;
    param.pw = temp;
    
    interrupter.pw = param.pw;
    
    if (param.synth == SYNTH_MIDI || param.synth == SYNTH_MIDI_QCW){
        SigGen_limit();
    }
    
	if (interrupter.mode!=INTR_MODE_OFF) {
		update_interrupter();
	}
    if(configuration.ext_interrupter){
        interrupter_update_ext();
    }
    
	return pdPASS;
}

/*****************************************************************************
* Timer callback for burst mode
******************************************************************************/
void vBurst_Timer_Callback(TimerHandle_t xTimer){
    uint16_t bon_lim;
    uint16_t boff_lim;
    if(interrupter.burst_state == BURST_ON){
        interrupter.pw = 0;
        update_interrupter();
        interrupter.burst_state = BURST_OFF;
        if(!param.burst_off){
            boff_lim=2;
        }else{
            boff_lim=param.burst_off;
        }
        xTimerChangePeriod( xTimer, boff_lim / portTICK_PERIOD_MS, 0 );
    }else{
        interrupter.pw = param.pw;
        update_interrupter();
        interrupter.burst_state = BURST_ON;
        if(!param.burst_on){
            bon_lim=2;
        }else{
            bon_lim=param.burst_on;
        }
        xTimerChangePeriod( xTimer, bon_lim / portTICK_PERIOD_MS, 0 );
    }
}

/*****************************************************************************
* Callback if a burst mode parameter is changed
******************************************************************************/
uint8_t callback_BurstFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle) {
    if(interrupter.mode!=INTR_MODE_OFF){
        if(interrupter.xBurst_Timer==NULL && param.burst_off > 0){
            interrupter.burst_state = BURST_ON;
            interrupter.xBurst_Timer = xTimerCreate("Bust-Tmr", param.burst_on / portTICK_PERIOD_MS, pdFALSE,(void * ) 0, vBurst_Timer_Callback);
            if(interrupter.xBurst_Timer != NULL){
                xTimerStart(interrupter.xBurst_Timer, 0);
                ttprintf("Burst Enabled\r\n");
                interrupter.mode=INTR_MODE_TR;
            }else{
                interrupter.pw = 0;
                ttprintf("Cannot create burst Timer\r\n");
                interrupter.mode=INTR_MODE_OFF;
            }
        }else if(interrupter.xBurst_Timer!=NULL && !param.burst_off){
            if(xTimerDelete(interrupter.xBurst_Timer, 200 / portTICK_PERIOD_MS) != pdFALSE){
                interrupter.xBurst_Timer = NULL;
                interrupter.burst_state = BURST_ON;
                update_interrupter();
                interrupter.mode=INTR_MODE_TR;
                ttprintf("\r\nBurst Disabled\r\n");
            }else{
                ttprintf("Cannot delete burst Timer\r\n");
                interrupter.burst_state = BURST_ON;
            }
        }
    }
	return pdPASS;
}


/*****************************************************************************
* starts or stops the transient mode (classic mode)
* also spawns a timer for the burst mode.
******************************************************************************/
uint8_t CMD_tr(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args) {
    
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf("Transient [start/stop]");
        return TERM_CMD_EXIT_SUCCESS;
    }
    
    if(strcmp(args[0], "start") == 0){
        interrupter_DMA_mode(INTR_DMA_TR);
        
        interrupter.pw = param.pw;
		interrupter.prd = param.pwd;
        update_interrupter();

		interrupter.mode=INTR_MODE_TR;
        
        callback_BurstFunction(NULL, 0, handle);
        
		ttprintf("Transient Enabled\r\n");
       
        return TERM_CMD_EXIT_SUCCESS;
    }

	if(strcmp(args[0], "stop") == 0){
        SigGen_switch_synth(param.synth);
        if (interrupter.xBurst_Timer != NULL) {
			if(xTimerDelete(interrupter.xBurst_Timer, 100 / portTICK_PERIOD_MS) != pdFALSE){
			    interrupter.xBurst_Timer = NULL;
                interrupter.burst_state = BURST_ON;
            }
        }

        ttprintf("Transient Disabled\r\n");    
 
		interrupter.pw = 0;
		update_interrupter();
        if(configuration.ext_interrupter) interrupter_update_ext();
		interrupter.mode=INTR_MODE_OFF;
		
		return TERM_CMD_EXIT_SUCCESS;
	}
    return TERM_CMD_EXIT_SUCCESS;
}

