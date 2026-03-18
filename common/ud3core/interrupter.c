/**
 * @file interrupter.c
 * @brief Interrupter control and pulse generation implementation
 *
 * Implements the core pulse timing system for the Tesla coil. Manages DMA-driven
 * PWM updates, transient mode operation, MIDI/SID synthesis integration, burst
 * timing, and external interrupter input. Coordinates with signal generator and
 * duty compressor for automated modulation.
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
#include <stdlib.h>

uint16_t int1_prd, int1_cmp;
interrupter_params interrupter;

/**
 * @brief Emergency stop - disable all interrupter output
 *
 * Sets interlock flag to prevent operation, kills all audio synthesis,
 * zeros pulse width parameter, and updates hardware to safe state.
 */
void interrupter_kill(void){
    sysfault.interlock = 1;
    SigGen_killAudio();
    param.pw=0;
    interrupter_updateTR();
}

/**
 * @brief Release interlock to allow interrupter operation
 *
 * Clears the interlock flag. Does not automatically restart operation.
 */
void interrupter_unkill(void){
    sysfault.interlock=0;
}

/** @brief Interrupter DMA channel handle */
uint8 int1_dma_Chan;

/** @brief Number of transfer descriptors per channel */
#define N_TD 4

/** @brief DMA channel handles for multi-channel operation */
uint8 ch_dma_Chan[N_CHANNEL];

/** @brief Transfer descriptors for multi-channel DMA */
uint8_t ch_dma_TD[N_CHANNEL][N_TD];

/** @brief DMA bytes per burst for current modulation mode */
#define MODULATION_CUR_BYTES 1

/** @brief DMA bytes per burst for pulse width modulation mode */
#define MODULATION_PW_BYTES  8

/** @brief DMA requests per burst */
#define MODULATION_REQUEST_PER_BURST  1

/**
 * @brief Push critical error alarm and stop UD3
 * @param message Error message string
 * @param val Error value/code
 *
 * Adds alarm to queue with critical priority and calls interrupter_kill().
 * TODO: Make this a global function for use throughout codebase.
 */
void critical_error(const char *message, int32_t val) {
    alarm_push(ALM_PRIO_CRITICAL, message, val);
    interrupter_kill();
}

/**
 * @brief One-time initialization of interrupter hardware and DMA
 *
 * Initializes:
 * - Signal generator and duty compressor
 * - Op-amp for analog feedback
 * - PWM timer with safe default values (65000 period, minimal on-time)
 * - DMA channel for automated PWM register updates (4 TDs in circular chain)
 *
 * DMA transfers int1_prd and int1_cmp to interrupter1 PWM component registers.
 * Must be called once at system startup.
 */
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

/**
 * @brief Initialize interrupter to safe default state
 *
 * Kills interrupter output, sets large period (65000) and minimal compare
 * value (64999) for safe 1µs pulse width. Used during configuration changes
 * or fault recovery.
 */
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

/**
 * @brief Configure interrupter based on current parameter settings
 *
 * Initializes to safe state, calculates minimum transient mode period from
 * configured maximum PRF (pulse repetition frequency), and disables interrupter.
 * Called when EEPROM is loaded or interrupter settings change.
 */
void configure_interrupter()
{

    //Init interrupter to some safe values
    interrupter_init_safe();

    // The minimum interrupter period for transient mode in clock ticks (which are equal to microseconds here).
	params.min_tr_prd = INTERRUPTER_CLK_FREQ / configuration.max_tr_prf;
    
    // Disable interrupter
    interrupter.mode = INTR_MODE_OFF;
}

/**
 * @brief Fire a single pulse with specified parameters (scaled volume)
 * @param pw Pulse width in microseconds
 * @param vol Volume (0 to INT16_MAX, scaled to current limit DAC)
 *
 * Generates a single transient-mode pulse. Volume is scaled to DAC value
 * based on min/max current limit settings. Enforces max_tr_pw safety limit.
 * Returns immediately if fault exists or external interrupter is active.
 * Updates hardware with atomic interrupt disable during register writes.
 */
void interrupter_oneshot(uint32_t pw, uint32_t vol) {
    if(tsk_fault_is_fault() || configuration.ext_interrupter) return;
    
	if (vol < MAX_VOL) {
		ct1_dac_val[0] = params.min_tr_cl_dac_val + ((vol * params.diff_tr_cl_dac_val) >> 15);
        if(ct1_dac_val[0] > params.max_tr_cl_dac_val) ct1_dac_val[0] = params.max_tr_cl_dac_val;
	} else {
		ct1_dac_val[0] = params.max_tr_cl_dac_val;
	}
	if (pw > configuration.max_tr_pw) {
		pw = configuration.max_tr_pw;
	}
	uint16_t prd = param.offtime + pw;
	/* Update Interrupter PWMs with new period/pw */
	CyGlobalIntDisable;
	int1_prd = prd - 3;
	int1_cmp = prd - pw - 3;
	interrupter1_control_Control = INT_ENA;
	interrupter1_control_Control = INT_KILL_ALL;
    CyGlobalIntEnable;
}

/**
 * @brief Fire a single pulse with raw DAC value (no scaling)
 * @param pw_us Pulse width in microseconds
 * @param dacValue_counts Raw DAC value in counts for current limit
 *
 * Generates a single pulse with direct DAC control (no volume scaling).
 * Used for precise current limit control. Enforces max_tr_pw safety limit.
 * Returns immediately if fault exists or external interrupter is active.
 */
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

/**
 * @brief Update interrupter for external input mode
 *
 * Configures hardware to accept external gate/trigger signals. Sets current
 * limit DAC to maximum and pulse width to configured max_tr_pw. External
 * signal controls actual pulse timing. Supports normal and inverted polarity
 * based on configuration.ext_interrupter (0=off, 1=normal, 2=inverted).
 */
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

/**
 * @brief Callback when external interrupter setting changes
 * @param params Parameter array (unused)
 * @param index Parameter index (unused)
 * @param handle Terminal handle (unused)
 * @return pdPASS
 *
 * If external interrupter enabled: pushes warning alarm and configures hardware.
 * If disabled: halts tesla coil operation and disables interrupter control.
 */
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

/**
 * @brief Callback when modulation mode parameter changes
 * @param params Parameter array (unused)
 * @param index Parameter index (unused)
 * @param handle Terminal handle (unused)
 * @return pdPASS
 *
 * Note: Modulation mode changes (PW vs current) no longer supported.
 */
uint8_t callback_interrupter_mod(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    //modulation change no longer supported!
    
    return pdPASS;
}

/**
 * @brief Update signal generator with current TR (transient) mode parameters
 *
 * Pushes pulse width, volume, frequency, and burst parameters from param struct
 * to the signal generator. Interrupter no longer generates pulses directly in TR
 * mode - all pulse generation delegated to SigGen. Returns early if not in TR mode
 * or if period (pwd) is zero (to avoid division by zero).
 */
void interrupter_updateTR() {
    //are we in tr mode?
    if(param.synth != SYNTH_TR) return;
    
    //interrupter no longer generates pulses itself, thats all done by siggen. So instead of updating the hardware we just update siggen with the new TR parameters
    if(param.pwd == 0) return;
    int32_t frequency_dHz = 10000000 / param.pwd;
    SigGen_setVoiceTR(1, param.pw, MAX_VOL, frequency_dHz, param.burst_on * 1000, (param.burst_on == 0) ? 0 : param.burst_off * 1000);
}

/**
 * @brief Callback when synthesizer mode parameter changes
 * @param params Parameter array (unused)
 * @param index Parameter index (unused)
 * @param handle Terminal handle (unused)
 * @return pdTRUE (accept change)
 *
 * Resets both MIDI and SID processors and switches signal generator to new mode.
 * Mode options: SYNTH_OFF, SYNTH_TR, SYNTH_MIDI, SYNTH_SID.
 */
uint8_t callback_SynthFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    //this also needs to reset both vms and sid
    SidProcessor_resetSid();
    MidiProcessor_resetMidi();
    SigGen_switchSynthMode(param.synth);
    return 1;
}

/**
 * @brief Callback when pulse width or period parameters change
 * @param params Parameter array (unused)
 * @param index Parameter index (unused)
 * @param handle Terminal handle (unused)
 * @return pdPASS
 *
 * Handles parameter changes in pulse width (pw) or period (pwd). Routes update
 * to appropriate subsystem based on current mode:
 * - External interrupter: Updates external gate configuration
 * - TR mode: Updates signal generator TR parameters
 * - MIDI mode: Notifies MIDI processor of pulse width change
 */
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

/**
 * @brief Callback when volume parameter changes
 * @param params Parameter array (unused)
 * @param index Parameter index (unused)
 * @param handle Terminal handle (unused)
 * @return pdPASS
 *
 * Updates master volume in signal generator. Volume is percentage of maximum
 * current limit (0-100%).
 */
uint8_t callback_VolFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle) {
    SigGen_setMasterVol(param.vol);
	return pdPASS;
}

/**
 * @brief Callback when burst mode parameters change
 * @param params Parameter array (unused)
 * @param index Parameter index (unused)
 * @param handle Terminal handle (unused)
 * @return pdPASS
 *
 * Triggered by changes to burst_on or burst_off parameters. Updates signal
 * generator with new burst timing in TR mode.
 */
uint8_t callback_BurstFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle) {
    interrupter_updateTR();
	return pdPASS;
}


/**
 * @brief CLI command to start or stop transient (TR) mode
 * @param handle Terminal handle for output
 * @param argCount Number of arguments (expects 1)
 * @param args Argument array: args[0] = "start" or "stop"
 * @return TERM_CMD_EXIT_SUCCESS
 *
 * Usage: `tr start` - Switch to TR mode and configure signal generator
 *        `tr stop`  - Switch to SYNTH_OFF mode and halt output
 *
 * TR mode is the classic continuous interrupter mode with configurable pulse
 * width, period, and burst timing.
 */
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

/**
 * @brief CLI command to fire a single pulse with specified parameters
 * @param handle Terminal handle for output
 * @param argCount Number of arguments (expects 2)
 * @param args Argument array: args[0] = ontime (µs), args[1] = volume (0-INT16_MAX)
 * @return TERM_CMD_EXIT_SUCCESS on success, TERM_CMD_EXIT_ERROR on invalid input
 *
 * Usage: `oneshot <ontime_us> <volume>`
 *
 * Volume is interpreted as SigGen volume (0 to INT16_MAX), mapped to current
 * limit DAC values. Ontime must be positive and within safety limits.
 */
uint8_t CMD_oneshot(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args) {
    if (argCount != 2) {
        ttprintf("Usage: oneshot <ontime> <volume>\n\r");
        return TERM_CMD_EXIT_ERROR;
    }
    char* end_ptr = 0;
    unsigned ontime_us = strtoul(args[0], &end_ptr, 10);
    unsigned volume = strtoul(args[1], &end_ptr, 10);
    if (ontime_us == 0 || volume == 0) {
        ttprintf("Expected volume and ontime to be positive\n\r");
        return TERM_CMD_EXIT_ERROR;
    }
    SigGen_pulseData_t pulse = {
        // Does not matter for single shot, just make it "big enough" for the siggen to be happy
        .period = SIGGEN_MIN_PERIOD,
        .onTime = ontime_us,
        .current = volume,
    };
    SigGen_queuePulse(&pulse);
    return TERM_CMD_EXIT_SUCCESS;
}
