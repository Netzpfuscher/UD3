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
#include "DutyCompressor.h"
#include "cli_common.h"
#include "qcw.h"
#include "tasks/tsk_fault.h"
#include "tasks/tsk_midi.h"
#include "VMSWrapper.h"
#include "SidProcessor.h"
#include "MidiProcessor.h"
#include "NoteMapper.h"

#include <device.h>
#include <math.h>

uint16_t int1_prd, int1_cmp;

void interrupter_kill(void){
    sysfault.interlock = 1;
    SigGen_killAudio();
    param.pw=0;
    interrupter_updateTR();
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

// Adds an alarm with the specified message to the alarm queue and stops the UD3
// TODO: Make this a global function so it can be used everywhere resources and memory are allocated.
void critical_error(const char *message, int32_t val) {
    alarm_push(ALM_PRIO_CRITICAL, message, val);
    interrupter_kill();
}

// One-time initialization of the interrupter components (called once at system startup)
void initialize_interrupter(void) {
    //initialize both signal generator and duty compressor
    SigGen_init();
    Comp_init();
    
    Opamp_2_Start();

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
    
    CyDmaChEnable(int1_dma_Chan, 1);
    
    configure_interrupter();
}

void interrupter_init_safe(){
        // Safe defaults in case anything fails.
    interrupter_kill();
   
  	int1_prd = 65000;
	int1_cmp = 64999;

  	//initialize the PWM generators for safe PW and PRD
	interrupter1_WritePeriod(int1_prd);
	interrupter1_WriteCompare1(int1_cmp);      // pwm1 will be true for one clock, then false.  int1_prd - int1_cmp is the on time for the pulse.
  	interrupter1_WriteCompare2(int1_prd);      // pwm2 will be false for one clock then true
  
}

// Called whenever an interrupter-related param is changed or eeprom is loaded.
void configure_interrupter()
{

    //Init interrupter to some safe values
    interrupter_init_safe();

    // The minimum interrupter period for transient mode in clock ticks (which are equal to microseconds here).
	params.min_tr_prd = INTERRUPTER_CLK_FREQ / configuration.max_tr_prf;
    
    // Disable interrupter
    interrupter.mode = INTR_MODE_OFF;
}

void interrupter_oneshot(uint32_t pw, uint32_t vol) {
    if(tsk_fault_is_fault() || configuration.ext_interrupter) return;
    
	if (vol < MAX_VOL) {
		ct1_dac_val[0] = params.min_tr_cl_dac_val + ((vol * params.diff_tr_cl_dac_val) >> 15);
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
	interrupter1_control_Control = INT_ENA;
	interrupter1_control_Control = INT_KILL_ALL;
    CyGlobalIntEnable;
}

void interrupter_oneshotRaw(uint32_t pw_us, uint32_t dacValue_counts) {
    //is sysfault triggered?
    if(tsk_fault_is_fault() || configuration.ext_interrupter) return;
    
    //is the output already on?
    //TODO add this check either in hardware or software
        
    //is the set current still valid?
	if (dacValue_counts > params.max_tr_cl_dac_val) {
		dacValue_counts = params.max_tr_cl_dac_val;
	}
    
    //update dac value
    ct1_dac_val[0] = dacValue_counts;
    
    //is the period still valid?
	if (pw_us > configuration.max_tr_pw) {
		pw_us = configuration.max_tr_pw;
	}
    
    //compute period and compare register values
	uint16_t prd = param.offtime + pw_us;
    
	CyGlobalIntDisable;
    
    //update int timer value
	int1_prd = prd - 3;
	int1_cmp = prd - pw_us - 3;
    
    //trigger pulse
	interrupter1_control_Control = INT_ENA;
	interrupter1_control_Control = INT_KILL_ALL;
    CyGlobalIntEnable;
}


void interrupter_update_ext() {

	ct1_dac_val[0] = params.max_tr_cl_dac_val;
    
    //set the pulsewidth timer to the maximum the coil can do
    uint32_t pw = configuration.max_tr_pw;
    
    //TODO is this actually necessary? We would only be enforcing this offtime when the interrupter off signal comes from the timer itself
    uint16_t prd = param.offtime + pw;
    
	/* Update Interrupter PWMs with new period/pw */
	CyGlobalIntDisable;
	int1_prd = prd - 3;
	int1_cmp = prd - pw - 3;
    
    switch(configuration.ext_interrupter){
        case 0:
            interrupter1_control_Control = INT_KILL_ALL;
        break;
        case 1:
            interrupter1_control_Control = INT_EXT_ENA;
        break;
        case 2:
            interrupter1_control_Control = INT_EXT_ENA | INT_EXT_INV;
        break;
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
        interrupter1_control_Control = INT_KILL_ALL;
        system_fault_Control = sfflag;
    }
    return pdPASS;
}

uint8_t callback_interrupter_mod(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    //modulation change no longer supported!
    
    return pdPASS;
}

void interrupter_updateTR() {
    //are we in tr mode?
    if(param.synth != SYNTH_TR) return;
    
    //interrupter no longer generates pulses itself, thats all done by siggen. So instead of updating the hardware we just update siggen with the new TR parameters
    int32_t frequency_dHz = 10000000 / param.pwd;
    SigGen_setVoiceTR(1, param.pw, MAX_VOL, frequency_dHz, param.burst_on * 1000, (param.burst_on == 0) ? 0 : param.burst_off * 1000);
}

//synth parameter was changed => update siggen mode
uint8_t callback_SynthFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    //this also needs to reset both vms and sid
    SidProcessor_resetSid();
    MidiProcessor_resetMidi();
    SigGen_switchSynthMode(param.synth);
    return 1;
}

/*****************************************************************************
* Callback if a transient mode parameter is changed
* Updates the interrupter hardware
******************************************************************************/
uint8_t callback_PWFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle) {
    
   if(configuration.ext_interrupter){
        interrupter_update_ext();
    }else if(param.synth == SYNTH_TR){
        interrupter_updateTR();
    }else if(param.synth == SYNTH_MIDI){
        VMSW_pulseWidthChangeHandler();
    }

	return pdPASS;
}

/*****************************************************************************
* Callback if a transient mode parameter is changed (percent ontime)
* Updates the interrupter hardware
******************************************************************************/
uint8_t callback_VolFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle) {
    SigGen_setMasterVol(param.vol);
	return pdPASS;
}

/*****************************************************************************
* Callback if a burst mode parameter is changed
******************************************************************************/
uint8_t callback_BurstFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle) {
    interrupter_updateTR();
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
        //switch synth to tr mode
        param.synth = SYNTH_TR;
        SigGen_switchSynthMode(param.synth);
        
        interrupter_updateTR();
        
		ttprintf("Transient Enabled\r\n");
       
    }else if(strcmp(args[0], "stop") == 0){
        //switch synth off
        param.synth = SYNTH_OFF;
        SigGen_switchSynthMode(param.synth);
        
        ttprintf("Transient Disabled\r\n");    
 
	}
    return TERM_CMD_EXIT_SUCCESS;
}



void Synthmon_printMIDI(TERMINAL_HANDLE * handle){
    uint32_t freq=0;

    //print what the voices are doing
    for(uint8_t i=0;i<SIGGEN_VOICECOUNT;i++){
        VMS_VoiceData_t * cvData = &(VMSW_getVoiceData()[i]);
        
        ttprintnlf("Voice %d: \r\n", i);
        ttprintnlf("\tNote %02d from Ch %02d at Volume %04x %s\r\n", cvData->sourceNote, cvData->sourceChannel, cvData->baseVolume, cvData->on ? "ON" : "OFF");
        ttprintnlf("\tFreq=%05ddHz PW=%05dus curVOL=%04x HPV=%d noise=%d\r\n", cvData->freqCurrent, cvData->onTimeCurrent, cvData->volumeCurrent, cvData->hypervoiceCount, cvData->noiseCurrent);
    }
    
    ttprintnlf("Maps: \r\n");
        
    //print map names
    for(uint8_t i=0;i<MIDI_CHANNELCOUNT;i++){
        const char * name = Mapper_getProgrammName(i);
        
        if(name != NULL){
            ttprintnlf("[%d]=\"% 20s\" ", i, name);
        }else{
            ttprintnlf("[%d]= % 20s  ", i, "NO MAP LOADED");
        }
        
        i++;
        
        if(i < MIDI_CHANNELCOUNT){
            name = Mapper_getProgrammName(i);
        
            if(name != NULL){
                ttprintnlf("[%d]=\"% 20s\" ", i, name);
            }else{
                ttprintnlf("[%d]= % 20s  ", i, "NO MAP LOADED");
            }
        }else{
            ttprintnlf("\r\n", i);
        }
        
        i++;
        
        if(i < MIDI_CHANNELCOUNT){
            name = Mapper_getProgrammName(i);
        
            if(name != NULL){
                ttprintnlf("[%d]=\"% 20s\"\r\n", i, name);
            }else{
                ttprintnlf("[%d]= % 20s  \r\n", i, "NO MAP LOADED");
            }
        }else{
            ttprintnlf("\r\n", i);
        }
    }
}

void Synthmon_printSID(TERMINAL_HANDLE * handle){
    
    for(uint8_t i=0;i<N_SIDCHANNEL;i++){
        SIDChannelData_t * chData = &(SID_getChannelData()[i]);
        
        ttprintnlf("Channel %d: \r\n", i);
        ttprintnlf("\tFreq=%05ddHz curVOL=%04x WAVE=%s\r\n", chData->frequency_dHz, chData->currentEnvelopeVolume, SID_getWaveName(chData));
        ttprintnlf("\tADSR state=%d masterVolume=%d\r\n", chData->adsrState, SID_filterData.channelVolume[i]);
    }
}

uint8_t CMD_SynthMon(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    TERM_sendVT100Code(handle, _VT100_CURSOR_DISABLE,0);
    TERM_sendVT100Code(handle, _VT100_CLS,0);
    
            
    while(Term_check_break(handle,100)){
        ttprintnlf("Synthesizer status:    [CTRL+C] for quit\r\n\n");
        
        if(param.synth == SYNTH_SID || param.synth == SYNTH_SID_QCW){
            Synthmon_printSID(handle);
        
        }else if(param.synth == SYNTH_MIDI || param.synth == SYNTH_MIDI_QCW){ 
            Synthmon_printMIDI(handle);
            
        }
        
        if(param.synth == 0){
            ttprintnlf("\r\nNo synthesizer active\r\n");   
        }else{
            //if any synth is active print compressor state
            ttprintnlf("\r\n\nCompressor state:\r\n");
            ttprintnlf("\tstate=%01d volume=%04x\r\n", Comp_getState(), Comp_getGain());
        }
            
        //reset cursor
        TERM_sendVT100Code(handle, _VT100_CURSOR_POS1, 0);
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    TERM_sendVT100Code(handle, _VT100_CURSOR_ENABLE,0);
    TERM_sendVT100Code(handle, _VT100_CLS,0);
    
    return TERM_CMD_EXIT_SUCCESS;
}
