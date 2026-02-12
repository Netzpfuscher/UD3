/**
 * @file SignalGenerator.c
 * @brief Polyphonic pulse generator implementation
 *
 * Core signal generation system providing:
 * - 6-voice polyphonic synthesis with independent timing and amplitude control
 * - Real-time pulse generation at 8kHz task rate (MIDI_ISR_Hz)
 * - Hardware timer ISR for precise pulse timing via ring buffer
 * - Hypervoice support (multiple sub-pulses per period for harmonic enrichment)
 * - Duty cycle calculation and limiting across all active voices
 * - Master volume control with 15-bit fixed-point scaling
 * - Burst mode for rhythmic on/off patterns
 * - Integration with DutyCompressor for dynamic range management
 *
 * Signal flow:
 * 1. Audio engines (MIDI/SID/VMS) call SigGen_setVoice*() to update voice parameters
 * 2. SigGen_task() runs at 8kHz, generates pulses from voice states
 * 3. SigGen_limit() applies safety constraints (duty, current, min OT/period)
 * 4. Pulses queued into ring buffer via SigGen_queuePulse()
 * 5. SigGen_PulseTimerISR() consumes pulses and commands hardware via interrupter_oneshotRaw()
 *
 * Thread safety:
 * - Voice parameters written by audio engines, read by SigGen_task()
 * - Ring buffer uses ISR-safe primitives for pulse queue
 * - taskData accessed from both task and ISR contexts
 *
 * Copyright (c) 2021 Jens Kerrinnes
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

#include <stdlib.h>
#include <limits.h>
#include <cytypes.h>

#include "clock.h"
#include "qcw.h"
#include "ZCDtoPWM.h"
#include "cli_common.h"

#include "FreeRTOS.h"
#include "task.h"
#include "SignalGenerator.h"
#include "DutyCompressor.h"
#include "RingBuffer/include/RingBuffer.h"
#include "interrupter.h"
#include "tasks/tsk_cli.h"
#include "telemetry.h"
#ifdef SIMULATOR
#include "tsk_audio.h"
#endif

/* ===== Static Variables ===== */

/** @brief Minimum on-time threshold (calculated from max_tr_pw * SigGen_minOtOffset%) */
static int32_t SigGen_minOt = 0;

/** @brief Pointer to main task data (voice array, pulse buffer, TR burst state) */
static SigGen_taskData_t * taskData;

/** @brief Forward declaration of main signal generation task */
static void SigGen_task(void * params);

/** @brief Master volume for all voices (0-MAX_VOL, typically 0-100), 15-bit fixed-point scaling */
static uint32_t masterVolume = MAX_VOL;

/** @brief Current synthesis mode (SYNTH_OFF/MIDI/SID/TR/MIDI_QCW/SID_QCW) */
static uint8_t synthMode = SYNTH_OFF;

/** @brief Current pulse being timed by hardware ISR (pre-loaded for next period) */
static SigGen_pulseData_t readPulse;

/** @brief Per-voice bitmask flags for UI feedback (VoiceFlags[i] = 1<<i) */
uint32_t VoiceFlags[SIGGEN_VOICECOUNT];

/** @brief Output enable flag (1=enabled, 0=disabled) */
static uint32_t isEnabled = 1;

/** @brief Bitmask of voices requiring immediate termination (noise mode switching) */
static volatile uint32_t voicesToEradicate = 0;

/* ===== Conversion Macros ===== */

/** @brief Convert milliseconds to timer period counts (32kHz timer: 32 counts/µs) */
#define SIGGEN_MS_TO_PERIOD_COUNT(X) (X) * 32000
/** @brief Convert microseconds to timer period counts (32kHz timer: 32 counts/µs) */
#define SIGGEN_US_TO_PERIOD_COUNT(X) (X) * 32
/** @brief Convert timer period counts to microseconds (right shift by 5 = divide by 32) */
#define SIGGEN_PERIOD_COUNT_TO_US(X) (X) >> 5
/** @brief Convert microseconds to on-time counts (1:1 for hardware PWM) */
#define SIGGEN_US_TO_OT_COUNT(X) (X)
/** @brief Convert siggen volume (0-INT16_MAX) to DAC current value using linear scaling */
#define SIGGEN_VOLUME_TO_CURRENT_DAC_VALUE(X) (params.min_tr_cl_dac_val + (((X) * params.diff_tr_cl_dac_val) >> 15))

/* ===== Control Flags ===== */

/** @brief Flag bit to indicate long pulse clearing required */
#define SIGGEN_CLEAR_LONG_PULSE 0x8000
/** @brief Mask for voices requiring immediate termination (all 8 bits set) */
#define SIGGEN_ERADICATE_REQUIRED 0xff
/** @brief Delay threshold for considering a pulse "long" (milliseconds) */
#define SIGGEN_LONG_DELAY_THRESHOLD_ms 2

/* ===== Timer Control Macros ===== */

/** @brief Check if pulse timer is currently running */
#define SigGen_isTimerRunning() (interrupterTimebase_ReadControlRegister() & interrupterTimebase_CTRL_ENABLE)
/** @brief Start pulse timer and enable ISR */
#define SigGen_startTimer() interrupterTimebase_WriteControlRegister(interrupterTimebase_ReadControlRegister() | interrupterTimebase_CTRL_ENABLE); SigGen_enableTimerISR();
/** @brief Stop pulse timer and disable ISR */
#define SigGen_stopTimer() interrupterTimebase_WriteControlRegister(interrupterTimebase_ReadControlRegister() & ~interrupterTimebase_CTRL_ENABLE); SigGen_disableTimerISR();
/** @brief Check if timer ISR is enabled */
#define SigGen_isTimerISREnabled() interrupterIRQ_GetState()
/** @brief Disable timer ISR */
#define SigGen_disableTimerISR() interrupterIRQ_Disable()
/** @brief Enable timer ISR */
#define SigGen_enableTimerISR() interrupterIRQ_Enable()

/* ===== Pulse Manipulation Macros ===== */

/** @brief Extract main pulse from voice state into pulse descriptor */
#define SIGGEN_GET_PULSE(VOICE, TARGETPULSE) TARGETPULSE.current = VOICE.pulseVolume; TARGETPULSE.period = VOICE.counter; TARGETPULSE.onTime = VOICE.limitedPulseWidth_us;
/** @brief Extract hypervoice pulse from voice state into pulse descriptor */
#define SIGGEN_GET_PULSE_HPV(VOICE, TARGETPULSE) TARGETPULSE.current = VOICE.hpvVolume; TARGETPULSE.period = VOICE.currHPVCounter; TARGETPULSE.onTime = VOICE.limitedHpvPulseWidth_us;
/** @brief Copy pulse descriptor (all three fields: current, period, onTime) */
#define SIGGEN_COPY_PULSE(TARGETPULSE, SRCPULSE) TARGETPULSE.current = SRCPULSE.current; TARGETPULSE.period = SRCPULSE.period; TARGETPULSE.onTime = SRCPULSE.onTime;

/* ===== Forward Declarations ===== */

/**
 * @brief Set hypervoice parameters for a voice (internal)
 * @param voice Voice index (0-5)
 * @param count Number of hypervoice sub-pulses per period
 * @param pulseWidth Hypervoice pulse width in microseconds
 * @param volume Hypervoice volume (0-INT16_MAX)
 * @param phase Phase offset (0-1024, where 1024=100%)
 */
static void SigGen_setHyperVoiceParams(uint32_t voice, uint32_t count, uint32_t pulseWidth, uint32_t volume, uint32_t phase);

/**
 * @brief Set main voice parameters (internal, used by all voice setter variants)
 * @param voice Voice index (0-5)
 * @param enabled Enable (1) or disable (0) voice
 * @param pulseWidth Pulse width in microseconds
 * @param volume Volume (0-INT16_MAX)
 * @param frequencyTenths Frequency in tenths of Hz
 * @param noiseAmplitude White noise amplitude (0 = disabled)
 * @param burstOn_us Burst on-time in microseconds (0 = no burst)
 * @param burstOff_us Burst off-time in microseconds
 */
static void SigGen_setVoiceParams(uint32_t voice, uint32_t enabled, int32_t pulseWidth, int32_t volume, int32_t frequencyTenths, int32_t noiseAmplitude, int32_t burstOn_us, int32_t burstOff_us);

/** @brief Debug flag for print throttling (used in simulator mode) */
uint32_t IsOkToPrint = 0;

/**
 * @brief 8kHz system tick ISR - clock and QCW control
 *
 * High-priority ISR called at MIDI_ISR_Hz (8000 Hz) by hardware timer.
 * Handles:
 * - Global clock tick (for uptime tracking)
 * - QCW mode ramping (if QCW_enable_Control active)
 *
 * Execution time: ~10µs (clock_tick) or ~50µs (qcw_handle)
 *
 * @note In QCW mode, bypasses normal signal generation to run qcw_handle()
 */
CY_ISR(isr_synth) {   
    clock_tick();
    if(QCW_enable_Control){
        qcw_handle();
        return;
    }
}
    
/**
 * @brief Hardware pulse timer ISR - consume pulses from ring buffer
 *
 * Called when interrupterTimebase timer expires (variable rate, depends on pulse periods).
 * Workflow:
 * 1. Command previous pulse to hardware via interrupter_oneshotRaw()
 * 2. Read next pulse from ring buffer
 * 3. Load next pulse period into timer compare register
 * 4. If buffer empty, stop timer
 *
 * Execution time: ~5-15µs depending on buffer state
 *
 * @note Uses ISR-safe RingBuffer_readFromISR() for thread safety
 * @note Zero-period pulses are rejected and retried (invalid state)
 * @note Timer stops automatically when buffer empty
 */
CY_ISR(SigGen_PulseTimerISR){
    interrupterTimebase_ReadStatusRegister();
    interrupterIRQ_ClearPending();
    
    //start the previous pulse
    if(!(readPulse.current == 0 || readPulse.onTime == 0)){
        if(configuration.is_qcw == 0 || synthMode == SYNTH_TR){ //Don't command a pulse in QCW mode... For now.
            interrupter_oneshotRaw(readPulse.onTime, readPulse.current);
        }
    }
    
    //try to read the next pulse
    while(1){
        if(RingBuffer_readFromISR(taskData->pulseBuffer, (void*)&readPulse, 1) == 1){
            //check if we got valid pulse and if not retry
            if(readPulse.period == 0){ 
                readPulse.period = 1;
                continue;
            }
            
            //and finally reduce the buffer size
            taskData->bufferLengthInCounts -= readPulse.period;
            
            if(readPulse.period < SIGGEN_MIN_PERIOD){ 
                //TODO evaluate occurance of this happening. Should be impossible and if it does happen it ruins the entire note timebase...
                readPulse.period = SIGGEN_MIN_PERIOD;
            }
            
            //load timer registers
            interrupterTimebase_WriteCompare(readPulse.period);
            
            //we got a valid time => exit loop
            break;

            //is the timer already running longer than the period? if so make it trigger as soon as possible
            //TODO evaluate if this is actually neccessary. After all the timer compare mode is set to ">=", so setting a compare value lower than the counter should trigger a pulse right away anyway
        }else{
            //no more pulses in the buffer or other error. Turn off the timer 
            SigGen_stopTimer();
            
            //also there is no way that there is still some time left in the buffer... clear it just in case
            taskData->bufferLengthInCounts = 0;
            
            //no more pulses could be read out => jsut exit from the loop
            break;
        }
        
    }
}

/**
 * @brief Parameter change callback for siggen configuration
 *
 * Recalculates derived parameters when siggen settings change:
 * - SigGen_minOt: minimum on-time threshold = max_tr_pw * SigGen_minOtOffset%
 *
 * @param params Parameter table (unused)
 * @param index Index of changed parameter (unused)
 * @param handle Terminal handle for error messages (unused)
 * @return pdPASS always (changes always accepted)
 */
uint8_t callback_siggen(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    //update minimum OT parameter
    SigGen_minOt = (configuration.max_tr_pw * configuration.SigGen_minOtOffset) / 100;
    
    return pdPASS;
}

/**
 * @brief Initialize signal generator subsystem
 *
 * Performs one-time setup:
 * - Initialize VoiceFlags[] bitmask array (VoiceFlags[i] = 1<<i)
 * - Allocate taskData structure (SigGen_taskData_t)
 * - Create pulse ring buffer (64 entries of SigGen_pulseData_t)
 * - Initialize hardware timers (interrupterTimebase for pulse timing)
 * - Register ISRs (SigGen_PulseTimerISR, isr_synth)
 * - Create SigGen_task FreeRTOS task (8kHz periodic execution)
 *
 * Must be called once during system initialization before any voice operations.
 */
void SigGen_init(){
    //initialize flags needed for masking voices
    for(uint32_t i = 0; i < SIGGEN_VOICECOUNT; i++){
        VoiceFlags[i] = 1<<i;
    }
    
    SigGen_taskData_t * data = pvPortMalloc(sizeof(SigGen_taskData_t));
    memset((void *)data, 0, sizeof(SigGen_taskData_t));
    taskData = data;
    
    //create pulse buffer
    data->pulseBuffer = RingBuffer_create(64, sizeof(SigGen_pulseData_t));
    
    //initialize timers
    
    //Timer 2&3 generate the signal period. 32Bit mode, no prescaler
    interrupterTimebase_Init();
    interrupterIRQ_StartEx(SigGen_PulseTimerISR);
    
    isr_midi_StartEx(isr_synth);
    
    xTaskCreate(SigGen_task, "SigGen", configMINIMAL_STACK_SIZE+128, (void*) data, tskIDLE_PRIORITY + 4, NULL);
}

/**
 * @brief Calculate current duty cycle across all active voices
 *
 * Iterates through all 6 voices, computing duty cycle contribution:
 * - Main pulse duty = (pulseWidth_us * frequencyTenths) / 100000
 * - Hypervoice duty added if hpvCount > 0 and noiseAmplitude == 0
 * - Total scaled by DutyCompressor gain if compressor active
 *
 * @return Duty cycle as tenths of percent (e.g., 125 = 12.5%)
 *
 * @note Hypervoice duty calculation approximates divider effect (TODO: improve accuracy)
 * @note Used for real-time limit enforcement and telemetry
 */
uint32_t SigGen_getCurrDuty(){
    uint32_t totalDuty = 0; //DER TOTAAALE TASTGRAD
    
    //calculate dutycycle of every voice and add it to the total dutycycle
    for(uint32_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
        if(!taskData->voice[currVoice].enabled) continue;
        
        //dutycycle = frequency * pulseWidth; pulsewidth calculation from volume already takes the master volume into account
        
        //scale to correct for the error from A: microseconds to seconds of OT, B: 10ths of a hertz to hertz in the frequency and C: a multiply by 100 for returning percent instead of 0-1
        uint32_t ourDuty = (taskData->voice[currVoice].pulseWidth_us * taskData->voice[currVoice].frequencyTenths) / 100000; 
        
        //is hypervoice on? if so add the duty from the second pulse
        if(taskData->voice[currVoice].hpvCount != 0 && taskData->voice[currVoice].noiseAmplitude == 0){ 
            //TODO actually respect the hpv divider in the duty calculation
            //scale to correct for the error from A: microseconds to seconds of OT, B: 10ths of a hertz to hertz in the frequency and C: a multiply by 100 for returning percent instead of 0-1       
            ourDuty += ((taskData->voice[currVoice].pulseWidth_us / taskData->voice[currVoice].hpvCount) * taskData->voice[currVoice].frequencyTenths) / 100000; 
        }
        totalDuty += ourDuty;
    }
    
    //scale current duty by the compressor volume
    if(Comp_getGain() != COMP_UNITYGAIN) totalDuty = (totalDuty * Comp_getGain()) >> 15;
    
    return totalDuty;
}

/**
 * @brief Set hypervoice parameters for a voice (internal helper)
 *
 * Configures sub-pulse generation for harmonic enrichment:
 * - Validates count > 0, noiseAmplitude == 0, masterVolume > 0, volume > 0
 * - Calculates hpvOffset = (phase * period) >> 10  (phase is 0-1024 fixed-point)
 * - Applies master volume scaling: hpvVolume = (volume * masterVolume) >> 15
 * - Updates hpvCount, currHPVDivider for period subdivision
 *
 * Disables hypervoice if:
 * - count == 0 (no hypervoice)
 * - noiseAmplitude > 0 (noise mode incompatible)
 * - masterVolume == 0 or volume == 0 (silent)
 *
 * @param voice Voice index (0-5)
 * @param count Number of hypervoice sub-pulses per period (0 = disabled)
 * @param pulseWidth Hypervoice pulse width in microseconds
 * @param volume Hypervoice volume (0-INT16_MAX)
 * @param phase Phase offset in 0-1024 fixed-point (1024 = 100% = full period)
 *
 * @note Called by SigGen_setHyperVoice*() variants
 * @note Phase provides harmonic tuning by offsetting sub-pulse timing
 */
//phase is 0-100% fixed point with a maximum of 1024=1
static void SigGen_setHyperVoiceParams(uint32_t voice, uint32_t count, uint32_t pulseWidth, uint32_t volume, uint32_t phase){
    if(count == 0 || taskData->voice[voice].noiseAmplitude || masterVolume == 0 || volume == 0){
        taskData->voice[voice].limitedHpvPulseWidth_us = 0;
        taskData->voice[voice].hpvPulseWidth_us = 0;
        
        taskData->voice[voice].limitedHpvCount = 0;
        taskData->voice[voice].hpvCount = 0;
        taskData->voice[voice].hpvVolume = 0;
        taskData->voice[voice].initialHpvVolume = 0;
        taskData->voice[voice].hpvPhase = 0;
        taskData->voice[voice].currHPVDivider = INT_MAX;
        return;
    }
    
    taskData->voice[voice].initialHpvVolume = volume;
    taskData->voice[voice].hpvVolume = (volume * masterVolume) >> 15;
    taskData->voice[voice].hpvPulseWidth_us = pulseWidth;
    taskData->voice[voice].hpvPhase = (count > 0) ? phase : 0;
    taskData->voice[voice].hpvCount = count;
    
    if(taskData->voice[voice].currHPVDivider > taskData->voice[voice].hpvCount) taskData->voice[voice].currHPVDivider = taskData->voice[voice].hpvCount;
    
    taskData->voice[voice].hpvOffset = (count > 0) ? (phase * taskData->voice[voice].period) >> 10 : 0;
}

/**
 * @brief Query if a voice is currently enabled
 * @param voice Voice index (0-5)
 * @return 1 if voice enabled, 0 if disabled
 */
uint32_t SigGen_isVoiceOn(uint32_t voice){
    return taskData->voice[voice].enabled;
}

/** @brief Debug divider for print throttling (unused in production) */
uint8_t divder = 0xff;

/**
 * @brief Set main voice parameters (internal, all voice setters delegate to this)
 *
 * Core voice parameter update function handling:
 * - Note-on/note-off detection (resets noteAge, counters on note-on)
 * - Frequency to period conversion: period = (10000000 / frequencyTenths) ticks at 8kHz
 * - Master volume scaling: pulseVolume = (volume * masterVolume) >> 15
 * - Burst mode timing: converts burstOn_us/burstOff_us to task ticks
 * - Voice eradication flagging for noise mode changes
 * - Hypervoice period calculation for sub-pulse timing
 *
 * Note-on conditions (all must be true):
 * - enabled == 1
 * - volume != 0
 * - frequencyTenths != 0
 *
 * @param voice Voice index (0-5)
 * @param enabled Enable (1) or disable (0) voice
 * @param pulseWidth Pulse width in microseconds
 * @param volume Volume (0-INT16_MAX), scaled by master volume
 * @param frequencyTenths Frequency in tenths of Hz (e.g., 4400 = 440.0 Hz)
 * @param noiseAmplitude White noise amplitude (0 = disabled, >0 = noise waveform)
 * @param burstOn_us Burst on-time in microseconds (0 = no burst)
 * @param burstOff_us Burst off-time in microseconds
 *
 * @note All public voice setters (VMS/SID/TR) call this function
 * @note Noise mode change triggers voicesToEradicate flag to clear stale pulses
 */
static void SigGen_setVoiceParams(uint32_t voice, uint32_t enabled, int32_t pulseWidth, int32_t volume, int32_t frequencyTenths, int32_t noiseAmplitude, int32_t burstOn_us, int32_t burstOff_us){
    IsOkToPrint = 1;
    
    //if(--divder == 0) TERM_printDebug(min_handle[1], "set voice freq=%d, ontime=%d\r\n", frequencyTenths, pulseWidth);
    
    //check if voice is switching on
    uint32_t willBeOn = enabled && (volume != 0) && (frequencyTenths != 0);
    
    //is the voice switching on?
    if(!taskData->voice[voice].enabled && willBeOn){
        //yes => reset the timebase for the note
        taskData->voice[voice].counter = 0;
        if(configuration.is_qcw && synthMode != SYNTH_TR){
            qcw_cmd_midi_pulse(volume, frequencyTenths);   
        }
    }
    
    taskData->voice[voice].enabled = willBeOn;
    
    if(!taskData->voice[voice].enabled || masterVolume == 0){ 
        taskData->voice[voice].enabled = 0;
        taskData->voice[voice].limitedEnabled = 0;
        
        //make sure to update the duty limiter
        SigGen_limit();
		return;
	}
    
    taskData->voice[voice].initialPulseVolume = volume;
    
    taskData->voice[voice].pulseVolume = (volume * masterVolume) >> 15;
    
    taskData->voice[voice].pulseWidth_us = pulseWidth;
    taskData->voice[voice].frequencyTenths = frequencyTenths;
    
    
    taskData->voice[voice].period = 10000000 / frequencyTenths;
    //TODO perhaps reset the counter if the period that was on it before was much longer than the current one? 
    
    if(noiseAmplitude > (taskData->voice[voice].period >> 1)){
        taskData->voice[voice].noiseAmplitude = taskData->voice[voice].period >> 1;
    }else{
        taskData->voice[voice].noiseAmplitude = noiseAmplitude;
    }
    
    //is burst on?
    if(burstOff_us > 0){
        //yes, setup burst counter
        int32_t burstPeriod_us = burstOn_us + burstOff_us;
        taskData->voice[voice].burstPeriod = burstPeriod_us;
        if(taskData->voice[voice].burstCounter > taskData->voice[voice].burstPeriod) taskData->voice[voice].burstCounter = burstPeriod_us;
        
        //the burst ontime is a comparison for > burstOt, so in order to get the correct ontime the output need to be on between burstOt and burstPeriod
        taskData->voice[voice].burstOt = burstPeriod_us - burstOn_us;
    }else{
        taskData->voice[voice].burstPeriod = 0;
        taskData->voice[voice].burstCounter = 0;
    }
    
    //re calculate hypervoice parameters
    if(taskData->voice[voice].hpvCount != 0) SigGen_setHyperVoiceParams(voice, taskData->voice[voice].hpvCount, taskData->voice[voice].hpvPulseWidth_us, taskData->voice[voice].hpvVolume, taskData->voice[voice].hpvPhase);
    
    SigGen_limit();
}

/**
 * @brief Calculate target on-time for fixed duty cycle mode
 *
 * Computes on-time to achieve target duty cycle based on:
 * - Target duty = max_tr_duty * (pulseWidth / max_tr_pw)
 * - Target OT = targetDuty / frequency
 * - Formula: OT_us = ((pulseWidth * max_tr_duty * 100 / max_tr_pw) * 100) / frequencyTenths
 *
 * Adjustments:
 * - Noise mode: OT *= 1.5 (compensate for duty reduction)
 * - Min clamp: SigGen_minOt (calculated from max_tr_pw * SigGen_minOtOffset%)
 * - Max clamp: max_tr_pw
 *
 * @param pulseWidth Input pulse width slider value (microseconds)
 * @param frequencyTenths Frequency in tenths of Hz (e.g., 4400 = 440.0 Hz)
 * @param noiseOn 1 if noise waveform active (increases OT by 50%), 0 otherwise
 * @return Calculated on-time in microseconds, or 0 if frequency out of range
 *
 * @note Frequency range: 0.1 Hz to 20 kHz (frequencyTenths 1-200000)
 * @note Used by VMS and SID modes to maintain consistent duty cycle across frequency range
 */
static uint32_t getFixedDutyOntime(int32_t pulseWidth, int32_t frequencyTenths, uint32_t noiseOn){
    //check for maximum frequency
    if(frequencyTenths > 200000 || frequencyTenths == 0) return 0;
    
    //uint32_t ot = ((SigGen_otCurveStart + ((frequencyTenths * SigGen_otDeriv) >> 16)) * pulseWidth) >> 13;
    
    //TERM_printDebug(min_handle[1], "Calc ot for freq=%ddHz val=%dus\r\n", frequencyTenths, ot);
    
    //return ot;
    
    //calculate the pulsewidth we need to achive the target dutycycle
    
    //target dutycycle is scaled 0-max_tr_duty with the ontime slider
    
    //targetDuty = max_duty * (pw/max_tr_pw)
    
    //target ontime = targetDuty / target frequency = max_duty * (pw/max_tr_pw) * 1/targetFrequency
    //target ontime_us = targetDuty / target frequency = max_duty * (pw/max_tr_pw) * 1/targetFrequency * 1e6
    
    //units:                 [us]        [thousands - 1/10%]                       [us]
    //                                   [         1/1000 %          ]
    //                     [                                  1/1000 %                            ]       range: 0-100000  
    //                     [                                  1/1000 %                            ]       range: 0-100000  
    //                     [                                  1/10000 %  aka 0.1ppm                         ]       range: 0-10000000  
    //                     [                                  1/10000 %  aka 0.1ppm                         ] /      [10/s]
    //                     [                                  1/10000 %  aka 0.1ppm                         ] *      [s/10]
    //                     [                                               us                                                  ]
    int32_t targetOt_us = ((((pulseWidth * configuration.max_tr_duty * 100) / (configuration.max_tr_pw)) * 100) / frequencyTenths);
    
    
    if(noiseOn) targetOt_us = ((targetOt_us * 3) >> 1);
    
    if(targetOt_us < SigGen_minOt) targetOt_us = SigGen_minOt;
    
    //TODO perhaps start reducing this as we approach a very low volume?
    
    //would that exceed the maximum pw?
    if(targetOt_us > configuration.max_tr_pw) targetOt_us = configuration.max_tr_pw;
    
    return targetOt_us;
}

/* ===== Public Voice Setter Wrappers ===== */

/**
 * @brief Set VMS (MIDI) voice parameters - public wrapper
 *
 * Wrapper for SYNTH_MIDI mode voice updates. Delegates to SigGen_setVoiceParams()
 * after converting pulseWidth to fixed-duty on-time via getFixedDutyOntime().
 *
 * @param voice Voice index (0-5)
 * @param enabled Enable (1) or disable (0) voice
 * @param pulseWidth Pulse width slider value (converted to fixed-duty OT)
 * @param volume Volume (0-INT16_MAX)
 * @param frequencyTenths Frequency in tenths of Hz
 * @param noiseAmplitude White noise amplitude (0 = disabled)
 * @param burstOn_us Burst on-time in microseconds (0 = no burst)
 * @param burstOff_us Burst off-time in microseconds
 *
 * @note Rejects calls if synthMode != SYNTH_MIDI
 * @note Called by MIDI processor (VMS engine)
 */
//wrappers for writing the voice configs from the different synthesizer modes
void SigGen_setVoiceVMS(uint32_t voice, uint32_t enabled, int32_t pulseWidth, int32_t volume, int32_t frequencyTenths, int32_t noiseAmplitude, int32_t burstOn_us, int32_t burstOff_us){
    if(synthMode != SYNTH_MIDI) return;
    
    SigGen_setVoiceParams(voice, enabled, getFixedDutyOntime(pulseWidth, frequencyTenths, noiseAmplitude > 0), volume, frequencyTenths, noiseAmplitude, burstOn_us, burstOff_us);
}
/**
 * @brief Set VMS hypervoice parameters - public wrapper
 * @param voice Voice index (0-5)
 * @param count Number of hypervoice sub-pulses per period
 * @param pulseWidth Hypervoice pulse width slider (converted to fixed-duty OT)
 * @param volume Hypervoice volume (0-INT16_MAX)
 * @param phase Phase offset (0-1024 fixed-point)
 *
 * @note Rejects calls if synthMode != SYNTH_MIDI
 */
void SigGen_setHyperVoiceVMS(uint32_t voice, uint32_t count, uint32_t pulseWidth, uint32_t volume, uint32_t phase){
    if(synthMode != SYNTH_MIDI) return;
    SigGen_setHyperVoiceParams(voice, count, getFixedDutyOntime(pulseWidth, taskData->voice[voice].frequencyTenths, 0), volume, phase);
}

/**
 * @brief Set SID voice parameters - public wrapper
 *
 * Wrapper for SYNTH_SID mode voice updates. No burst mode support.
 *
 * @param voice Voice index (0-5)
 * @param enabled Enable (1) or disable (0) voice
 * @param pulseWidth Pulse width slider value (converted to fixed-duty OT)
 * @param volume Volume (0-INT16_MAX)
 * @param frequencyTenths Frequency in tenths of Hz
 * @param noiseAmplitude White noise amplitude (0 = disabled)
 *
 * @note Rejects calls if synthMode != SYNTH_SID
 * @note Called by SID processor (6581/8580 emulation)
 */
void SigGen_setVoiceSID(uint32_t voice, uint32_t enabled, int32_t pulseWidth, int32_t volume, int32_t frequencyTenths, int32_t noiseAmplitude){
    if(synthMode != SYNTH_SID) return;
    SigGen_setVoiceParams(voice, enabled, getFixedDutyOntime(pulseWidth, frequencyTenths, noiseAmplitude > 0), volume, frequencyTenths, noiseAmplitude, 0, 0);
}

/**
 * @brief Set SID hypervoice parameters - public wrapper
 * @param voice Voice index (0-5)
 * @param count Number of hypervoice sub-pulses per period
 * @param pulseWidth Hypervoice pulse width slider (converted to fixed-duty OT)
 * @param volume Hypervoice volume (0-INT16_MAX)
 * @param phase Phase offset (0-1024 fixed-point)
 *
 * @note Rejects calls if synthMode != SYNTH_SID
 */
void SigGen_setHyperVoiceSID(uint32_t voice, uint32_t count, uint32_t pulseWidth, uint32_t volume, uint32_t phase){
    if(synthMode != SYNTH_SID) return;
    SigGen_setHyperVoiceParams(voice, count, getFixedDutyOntime(pulseWidth, taskData->voice[voice].frequencyTenths, 0), volume, phase);
}

/**
 * @brief Set TR (transient) mode voice parameters - public wrapper
 *
 * TR mode uses fixed voice 0, no hypervoice, no noise, direct pulse width control.
 *
 * @param enabled Enable (1) or disable (0) TR voice
 * @param pulseWidth Pulse width in microseconds (direct, not fixed-duty)
 * @param volume Volume (0-INT16_MAX)
 * @param frequencyTenths Frequency in tenths of Hz
 * @param burstOn_us Burst on-time in microseconds (0 = no burst)
 * @param burstOff_us Burst off-time in microseconds
 *
 * @note Rejects calls if synthMode != SYNTH_TR
 * @note TR mode always uses voice 0 (single-voice manual control)
 * @note Called by interrupter module for manual pulse control
 */
void SigGen_setVoiceTR(uint32_t enabled, int32_t pulseWidth, int32_t volume, int32_t frequencyTenths, int32_t burstOn_us, int32_t burstOff_us){
    if(synthMode != SYNTH_TR) return;
    
    //TR mode always uses voice 0 without noise
    SigGen_setVoiceParams(0, enabled, pulseWidth, volume, frequencyTenths, 0, burstOn_us, burstOff_us);
}

/**
 * @brief Apply safety limits to all voice parameters
 *
 * Multi-stage limiting process:
 * 1. Calculate current duty cycle via SigGen_getCurrDuty()
 * 2. Determine max duty (max_tr_duty/10, scaled by DutyCompressor maxDutyOffset if not TR mode)
 * 3. If currDuty > maxDuty, calculate scale-down factor: ontimeScale = (maxDuty * MAX_VOL) / currDuty
 * 4. For each voice:
 *    - Scale limitedPulseWidth_us by ontimeScale and Comp_getGain()
 *    - Scale limitedHpvPulseWidth_us similarly if hypervoice active
 *    - Clamp counter to limitedPeriod (prevent slow note artifacts)
 * 5. Update telemetry voice count (tt.n.midi_voices)
 *
 * Limiting behavior:
 * - TR mode: Hard duty limit (no maxDutyOffset scaling)
 * - MIDI/SID modes: Soft duty limit (DutyCompressor can increase headroom)
 * - Max duty clamped to 99%
 *
 * @note Called by SigGen_setVoiceParams() after parameter changes
 * @note Called by SigGen_task() each iteration for real-time limiting
 * @note Thread safety: TODO - evaluate if locking needed
 */
void SigGen_limit(){
    //get current dutycycle
    uint32_t currDuty = SigGen_getCurrDuty();
    
    //get maximum dutycycle
    uint32_t maxDuty = configuration.max_tr_duty / 10;
    
    uint32_t ontimeScale = MAX_VOL;
    
    //scale maxDuty with the hard limit override. Default maxDutyOffset=64 => maximum factor = 256/64 = 4 (>> 6 is / 64)
    //BUT only do so if we aren't in TR mode. If we are then the duty limit is a hard one
    if(synthMode != SYNTH_TR && Comp_getMaxDutyOffset() != 64) maxDuty = (maxDuty * Comp_getMaxDutyOffset()) >> 6; 
    
    //make sure the value isn't too high
    if(maxDuty > 99) maxDuty = 99;
    
    /* about the scaling (I admit this is weird):
     * 
     * The Dutycycle is a number between 0 and 1 that indicates for how long a signal is on in relation to its period.
     * 
     * In this code however the config parameters as well as the return from SigGen_getCurrDuty() is in percent! That means value = dutycycle * 100;
     * Earlier versions of the code also had some weirdness with the units. The Frequency in the function was in decihertz (10ths of a hertz) and the ontime was in microseconds, but only the frequency was corrected for. That means the overall error was value = dutycycle * 100 [percent correction] * 1000000 [microsecond to second correction]
     * 
     * This version now also corrects the microsecond error so the values returned are all in even integer percents.
     */
    
    //do we need to reduce the ontime of all of the notes?
    if(currDuty > maxDuty){
        //yes! Reduce the scaling factor
        
        ontimeScale = (maxDuty * MAX_VOL) / currDuty;
    }else{
        //TODO perhaps add duty limiter active flag?
    }
    
    //reset active voice count
    tt.n.midi_voices.value = 0;
    
    //TODO evaluate if this is thread safe, if not make it so (as requested by the captain)
    for(uint32_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
        //if a voice is disabled we don't need to change anything
        if(!taskData->voice[currVoice].enabled){ 
            taskData->voice[currVoice].limitedEnabled = 0;
            continue;
        }
        
        //linearly scale the ontime down
        uint32_t onTime = taskData->voice[currVoice].pulseWidth_us;
        onTime = (onTime * ontimeScale) >> 15;
        if(Comp_getGain() != COMP_UNITYGAIN) onTime = (onTime * Comp_getGain()) >> 15;
        taskData->voice[currVoice].limitedPulseWidth_us = onTime;
        taskData->voice[currVoice].limitedPeriod = taskData->voice[currVoice].period;
        
        //make sure to reset the counter to prevent a previously played lower note from slowing down any future ones
        if(taskData->voice[currVoice].counter > taskData->voice[currVoice].limitedPeriod) taskData->voice[currVoice].counter = taskData->voice[currVoice].limitedPeriod;
        
        //is HPV enabled? if so we need to scale that too
        if(taskData->voice[currVoice].hpvCount != 0 && taskData->voice[currVoice].noiseAmplitude == 0){
            onTime = taskData->voice[currVoice].hpvPulseWidth_us;
            onTime = (onTime * ontimeScale) >> 15;
            if(Comp_getGain() != COMP_UNITYGAIN) onTime = (onTime * Comp_getGain()) >> 15;
            taskData->voice[currVoice].limitedHpvPulseWidth_us = onTime;
            
            taskData->voice[currVoice].limitedHpvCount = taskData->voice[currVoice].hpvCount;
        }else{
            taskData->voice[currVoice].limitedHpvPulseWidth_us = 0;
        }
        
        taskData->voice[currVoice].limitedEnabled = taskData->voice[currVoice].enabled;
        
        //at this point the voice must be on => increment voice count
        tt.n.midi_voices.value ++;
    }
}

/**
 * @brief Switch synthesis mode and kill audio output
 *
 * Changes signal generator operating mode:
 * - SYNTH_OFF: No synthesis
 * - SYNTH_MIDI: MIDI polyphonic (VMS engine)
 * - SYNTH_SID: SID chip emulation
 * - SYNTH_TR: Manual transient mode
 * - SYNTH_MIDI_QCW / SYNTH_SID_QCW: QCW long pulse modes (TODO: not fully implemented)
 *
 * Always kills audio first to prevent glitches during mode transition.
 *
 * @param newMode New synthesis mode from enum SYNTH
 *
 * @note Called by interrupter module or CLI commands
 * @note QCW modes are partially implemented (placeholder switch cases)
 */
void SigGen_switchSynthMode(uint8_t newMode){
    //kill output when changing synth mode
    SigGen_killAudio();
    
    //set new mode
    synthMode = newMode;
    
    //what state are we changing to? TODO figure out is we actually need to change anything here :D
    switch(newMode){
        case SYNTH_MIDI:
            
            break;
        case SYNTH_SID:
        
            break;
        case SYNTH_MIDI_QCW:
        case SYNTH_SID_QCW:
        
            //TODO implement this... for now we just assume state was set to "off"
        
        case SYNTH_OFF:
        
            break;
        case SYNTH_TR:
        
            break;
    }
}

/**
 * @brief Set master volume and update all voices
 *
 * Updates global volume control (0-MAX_VOL, typically 0-100):
 * - If newVolume > 0: Rescale all voice volumes, enable output
 * - If newVolume == 0: Disable output (pause generation)
 *
 * Volume scaling (15-bit fixed-point):
 * - pulseVolume = (initialPulseVolume * masterVolume) >> 15
 * - hpvVolume = (initialHpvVolume * masterVolume) >> 15
 *
 * @param newVolume Master volume (0-MAX_VOL), values > MAX_VOL rejected
 *
 * @note All voice volumes pre-scaled by master volume for efficiency
 * @note Zero volume pauses generation without losing voice state
 */
void SigGen_setMasterVol(uint32_t newVolume){
    if(newVolume > MAX_VOL) return;
    masterVolume = newVolume;
    
    //did we just get muted?
    if(masterVolume > 0){
        //no => update all voices
        for(uint32_t i = 0; i < SIGGEN_VOICECOUNT; i++){
            taskData->voice[i].hpvVolume = (taskData->voice[i].initialHpvVolume * masterVolume) >> 15;
            taskData->voice[i].pulseVolume = (taskData->voice[i].initialPulseVolume * masterVolume) >> 15;
        }
        SigGen_setOutputEnabled(1);
    }else{
        //yes => pause generation
        SigGen_setOutputEnabled(0);
    }
}

/**
 * @brief Enable or disable signal output
 * @param en 1 to enable output, 0 to disable and kill audio
 *
 * @note Disabling output calls SigGen_killAudio() to flush buffer
 */
void SigGen_setOutputEnabled(uint32_t en){
	isEnabled = en;
	if(!en) SigGen_killAudio();
}

/**
 * @brief Emergency stop - kill all audio output immediately
 *
 * Comprehensive shutdown procedure:
 * 1. Disable all voices (enabled = 0, limitedEnabled = 0)
 * 2. Disable hypervoice (hpvCount = 0, limitedHpvCount = 0)
 * 3. Stop hardware timer (SigGen_stopTimer)
 * 4. Flush pulse ring buffer (RingBuffer_flush)
 * 5. Reset buffer length counter (bufferLengthInCounts = 0)
 * 6. Clear telemetry voice count
 *
 * Thread safety:
 * - Disables timer ISR before flushing buffer to prevent concurrent access
 * - Safe to call during UD3 startup (checks taskData != NULL)
 *
 * @note Called by interrupter_kill(), SigGen_setOutputEnabled(0), mode switches
 * @note Does not clear voice parameters (frequency, volume) - only disables output
 */
void SigGen_killAudio(){
    // This can only happen during UD3 startup
    if (!taskData) { return; }
    //TODO evaluate if this is actually thread safe
    
	//disable all voices
    for(uint32_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
        taskData->voice[currVoice].enabled = 0;
        taskData->voice[currVoice].limitedEnabled = 0;
        taskData->voice[currVoice].hpvCount = 0;
        taskData->voice[currVoice].limitedHpvCount = 0;
    }
    
    //kill the timer
    SigGen_stopTimer();
    
    //reset the buffer, which must be done with the timer interrupt disabled to prevent intereference with the bufferLengthInCounts write
    SigGen_disableTimerISR();
    RingBuffer_flush(taskData->pulseBuffer);
    taskData->bufferLengthInCounts = 0;
    SigGen_enableTimerISR();
    
    tt.n.midi_voices.value = 0;
}

/**
 * @brief Queue a pulse for hardware output (converts units and writes to ring buffer)
 *
 * Conversion process:
 * 1. Input pulse in microseconds and siggen volume (0-INT16_MAX)
 * 2. Convert period: µs → timer counts (32 counts/µs at 32kHz timer)
 * 3. Convert onTime: µs → timer counts (1:1 for hardware PWM)
 * 4. Convert current: siggen volume → DAC counts via linear scaling
 *    DAC value = min_tr_cl_dac_val + ((current * diff_tr_cl_dac_val) >> 15)
 * 5. Write to ring buffer, update bufferLengthInCounts
 *
 * Thread safety:
 * - Uses RingBuffer_write() (thread-safe)
 * - Disables timer ISR during bufferLengthInCounts update
 *
 * @param pulse Pointer to pulse descriptor in microseconds (period, onTime, current)
 * @return 1 if pulse queued successfully, 0 if buffer full
 *
 * @note Called by SigGen_task() to feed pulses to hardware ISR
 * @note Buffer capacity: 64 entries (SIGGEN_PULSEBUFFER_SIZE)
 */
uint8_t SigGen_queuePulse(SigGen_pulseData_t* pulse) {
    //convert the period, volume and ontime to the values that will need to be written into the hardware upon pulse execution
    SigGen_pulseData_t raw_pulse;
    raw_pulse.period = SIGGEN_US_TO_PERIOD_COUNT(pulse->period);
    raw_pulse.onTime = SIGGEN_US_TO_OT_COUNT(pulse->onTime);
    raw_pulse.current = SIGGEN_VOLUME_TO_CURRENT_DAC_VALUE(pulse->current);

    //write it into the buffer
    if(RingBuffer_write(taskData->pulseBuffer, (void*)&raw_pulse, 1, 0) != 1){
        //write failed, not enough space available...
        return 0;
    }else{
        //increase the buffersize
        SigGen_disableTimerISR();
        taskData->bufferLengthInCounts += raw_pulse.period;
        SigGen_enableTimerISR();
        return 1;
    }
}

/** @brief Debug flag for pulse addition tracking (unused) */
volatile uint32_t adding = 0;

/** @brief Debug value for compressor state (unused) */
volatile uint32_t compDebug = 0;

/**
 * @brief Overlay a new pulse onto an existing pulse descriptor
 *
 * Merging strategy:
 * - onTime: Maximum of (existing, new)
 * - current (volume): Sum of (existing + new)
 *
 * Called when multiple voices trigger simultaneously (same counter value).
 * Allows polyphonic mixing without separate pulse buffer entries.
 *
 * @param pulse Pointer to existing pulse descriptor (modified in-place)
 * @param newVolume Volume of new pulse to overlay (0-INT16_MAX)
 * @param newOntime On-time of new pulse in microseconds
 *
 * @note Alternative strategies considered: highest/lowest/average (see commented block)
 * @note Current strategy: loudest pulse on-time, summed volume
 */
static void SigGen_overlayPulse(SigGen_pulseData_t * pulse, int32_t newVolume, int32_t newOntime){
    //a pulse was triggered and we need to decide how to overlay it with any potentially already triggered pulses
    
    //ways in which we can scale this:
    /*
        
        Ontime:
            Highest
            Lowest
            Sum
            Average
            ->Loudest pulse
            Quitest pulse
            
            
        Volume:
            Highest
            Lowest
            Sum
            Average
            ->Loudest pulse
            Quitest pulse
    
    
    */
    
    
    if(newOntime > pulse->onTime) pulse->onTime = newOntime;
    
    pulse->current += newVolume;
    
    /*if(newVolume > pulse->current){
        pulse->onTime = newOntime; //if(pulseWidth < data->voice[currVoice].limitedPulseWidth_us)
        pulse->current = newVolume;
    }*/
}
    
/**
 * @brief Main signal generation task - converts voice states to pulse queue
 *
 * FreeRTOS task running at high priority, generates pulses from voice states:
 *
 * Main loop (8kHz via vTaskDelay(1)):
 * 1. Check if output enabled (skip if isEnabled == 0)
 * 2. Fill pulse buffer to 1ms worth of pulses (bufferLengthInCounts < 32000)
 * 3. For each buffer fill iteration:
 *    a. Find voice with smallest counter (next pulse to trigger)
 *    b. Check for hypervoice pulse vs main pulse
 *    c. Advance all voice counters by nextPulse.period (time to next pulse)
 *    d. Trigger pulses for all voices with counter == 0
 *    e. Overlay simultaneously-triggered pulses via SigGen_overlayPulse()
 *    f. Apply burst mode muting (if burstCounter < burstOt, zero pulse)
 *    g. Queue pulse via SigGen_queuePulse()
 * 4. Kickstart timer if stopped but pulses waiting
 *
 * Voice counter management:
 * - counter: Phase accumulator, wraps at period (resets to period when <= 0)
 * - currHPVCounter: Hypervoice sub-pulse accumulator
 * - currHPVDivider: Tracks which hypervoice sub-pulse in cycle
 *
 * Hypervoice logic:
 * - When currHPVDivider == 0 and currHPVCounter < counter: trigger HPV pulse
 * - currHPVDivider counts down from hpvCount to 0, then resets
 * - currHPVCounter = counter + hpvOffset (phase-shifted sub-pulse timing)
 *
 * Noise mode:
 * - Randomizes pulse timing: counter += rand() % noiseAmplitude
 * - Creates white noise waveform effect
 *
 * Burst mode:
 * - burstCounter increments each iteration
 * - If burstCounter < burstOt: mute pulse (onTime = 0, current = 0)
 * - Resets at burstPeriod
 *
 * TR mode:
 * - Only voice 0 active (single-voice manual control)
 * - TR burst state managed separately in data->trBurstState
 *
 * @param callData Pointer to SigGen_taskData_t structure
 *
 * @note Task priority: tskIDLE_PRIORITY + 4 (PRIO_MIDI from tsk_priority.h)
 * @note Stack size: configMINIMAL_STACK_SIZE + 128 words
 * @note Target buffer fill: 1ms (32000 timer counts at 32kHz)
 * @note Voice age incremented each cycle (for envelope effects)
 */
    
static void SigGen_task(void * callData){
    volatile SigGen_taskData_t * data = (SigGen_taskData_t *) callData;
    
    while(1){
        vTaskDelay(1);
        //uint32_t noVoicesEnabled = 1;
		
		if(!isEnabled) continue;
              
        //check if the buffer is running low
        while(data->bufferLengthInCounts < SIGGEN_MS_TO_PERIOD_COUNT(1)){
        
            //calculate time from current pulse to next pulse
        
            //find next pulse to be triggered
            int32_t nextVoice = 0xffff;
            SigGen_pulseData_t nextPulse = {.current = 0, . onTime = 0, .period = INT_MAX};
            
            for(uint32_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
                //if we are in TR mode then only voice 1 is active and all others can be ignored
                if(synthMode == SYNTH_TR && currVoice >= 1) break;

                //if no other enabled voice was found so far, then the current one is the one with the lowest remaining time. But only if it is enabled
                if(nextVoice == 0xffff){
                    if(data->voice[currVoice].limitedEnabled && data->voice[currVoice].limitedPulseWidth_us != 0){
                        nextVoice = currVoice;
                        
                        //will the next pulse be a hypervoice pulse or not?
                        if(data->voice[currVoice].currHPVDivider == 0 && data->voice[currVoice].currHPVCounter < data->voice[currVoice].counter){
                            //yes, set the timeToNextPulse to the hpv counter
                            nextPulse.period = data->voice[currVoice].currHPVCounter;
                        }else{
                            //no set it to the normal counter
                            nextPulse.period = data->voice[currVoice].counter;
                        }
                    }

                //is the current voice's timer expiring sooner than that of the current nextVoice? 
                }else{
                    if(data->voice[currVoice].limitedEnabled && data->voice[currVoice].limitedPulseWidth_us != 0){
                        if((data->voice[currVoice].counter < nextPulse.period) || (data->voice[currVoice].currHPVDivider == 0 && data->voice[currVoice].currHPVCounter < nextPulse.period)){
                            nextVoice = currVoice;

                            //will the next pulse be a hypervoice pulse or not?
                            if(data->voice[currVoice].currHPVDivider == 0 && data->voice[currVoice].currHPVCounter < data->voice[currVoice].counter){
                                //yes, set the timeToNextPulse to the hpv counter
                                nextPulse.period = data->voice[currVoice].currHPVCounter;
                            }else{
                                //no set it to the normal counter
                                nextPulse.period = data->voice[currVoice].counter;
                            }
                        }
                    }
                }
                
                //we also need to reset the alreadyTriggered flag for later
                data->voice[currVoice].alreadyTriggered = 0;
            }
            
            if(nextVoice == 0xffff){ 
                //if no voice is enabled we can just break out of the loop as no more data can be generated anyway
                break;
            }
            
            //figure out how long the maximum delay we can add to the buffer without it becoming laggy is
            int32_t maxPeriod_us = SIGGEN_PERIOD_COUNT_TO_US(((SIGGEN_MS_TO_PERIOD_COUNT(SIGGEN_LONG_DELAY_THRESHOLD_ms)) - data->bufferLengthInCounts));
            
            //is the time longer? If so just limit it to the maximum
            //if we do this then no voice will trigger a pulse and all pulse parameters will remain at zero except for the period
            //the timer interrupt will check for this and ignore the pulse as if it was one that had its pulse muted due to burst mode
            if(nextPulse.period > maxPeriod_us) nextPulse.period = maxPeriod_us;
            
            //noVoicesEnabled = 0;

            //now the voice of which the timer expires next is indexed by nextVoice, propagate the current time to that point (aka decrease all other time counters by the delay to the next pulse)
            for(uint32_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
                //if we are in TR mode then only voice 1 is active and all others can be ignored
                if(synthMode == SYNTH_TR && currVoice >= 1) break;
                
                if(data->voice[currVoice].limitedEnabled){ 
                    //decrement frequency timer
                    data->voice[currVoice].counter -= nextPulse.period;
                    
                    //decrement burst timer if burst is active
                    if(data->voice[currVoice].burstPeriod != 0) data->voice[currVoice].burstCounter -= nextPulse.period;
                    
                    //decrement hpv timer if hpv is active
                    if(data->voice[currVoice].currHPVDivider == 0) data->voice[currVoice].currHPVCounter -= nextPulse.period;

                    //check if a pulse needs to be sent because this timer expired (turned to 0)
                    if(data->voice[currVoice].counter <= 0){ 
                        //reset the counter. If noise is enabled we add a random offset to this
                        
                        //first make sure that the period is actually large enough to get the counter positive again
                        if(data->voice[currVoice].limitedPeriod < -data->voice[currVoice].counter){
                            //no its not... just reset the counter to zero. Best we can do :(
                            data->voice[currVoice].counter = 0;
                        }else{
                            //yep data seems valid. Add the period to the counter
                            data->voice[currVoice].counter += data->voice[currVoice].limitedPeriod;
                        }
                        
                        if(data->voice[currVoice].noiseAmplitude){
                            //generate 32bit random number
                            int32_t randomizer = rand()*rand();
                            //limit frequency change to 0.5-2.0x fBase or whatever noise Amplitude is set to
                            int32_t noiseLimit = (data->voice[currVoice].noiseAmplitude > data->voice[currVoice].limitedPeriod) ? data->voice[currVoice].limitedPeriod : data->voice[currVoice].noiseAmplitude;
                            randomizer = (randomizer & noiseLimit) - (noiseLimit>>1);
                            if(abs(randomizer) < data->voice[currVoice].counter) data->voice[currVoice].counter += randomizer;
                        }  //TODO verify use of "&" as randomizer scaling

                        //overlay current pulse
                        SigGen_overlayPulse(&nextPulse, data->voice[currVoice].pulseVolume, data->voice[currVoice].limitedPulseWidth_us);

                        //block timer for this iteration
                        data->voice[currVoice].alreadyTriggered = 1;
                        
                        //update HPV divider if it is enabled
                        if(data->voice[currVoice].limitedHpvCount != 0 && !data->voice[currVoice].noiseAmplitude){
                            
                            //it is enabled, reduce divider count if its not already zero
                            if(data->voice[currVoice].currHPVDivider > 0)  data->voice[currVoice].currHPVDivider--;

                            //reached the pulse to have HPV
                            if(data->voice[currVoice].currHPVDivider == 0){
                                //reset counter to the correct offset
                                data->voice[currVoice].currHPVCounter = data->voice[currVoice].hpvOffset;
                            }
                        }
                        
                    }
                    
                    if(data->voice[currVoice].currHPVDivider == 0 && data->voice[currVoice].currHPVCounter <= 0){
                        //HPV pulse was just triggered. Process it like a normal one, but also update the divider count
                        
                        //unlike a normal pulse a HPV pulse isn't continuous, it will disable itself until the next main pulse writes the proper count value
                        data->voice[currVoice].currHPVCounter = INT_MAX;
                        
                        //is HPV still supposed to be on?
                        if(data->voice[currVoice].limitedHpvCount != 0 && !data->voice[currVoice].noiseAmplitude){
                            //yes :)
                            data->voice[currVoice].currHPVDivider = data->voice[currVoice].limitedHpvCount;
                        }else{
                            data->voice[currVoice].currHPVDivider = INT_MAX;
                        }
                                
                        //overlay current pulse
                        SigGen_overlayPulse(&nextPulse, data->voice[currVoice].hpvVolume, data->voice[currVoice].limitedHpvPulseWidth_us);

                        //block timer for this iteration
                        data->voice[currVoice].alreadyTriggered = 1;
                    }
                    
                    if(data->voice[currVoice].burstPeriod != 0 && data->voice[currVoice].burstCounter <= 0){
                        //burst period timer just rolled over. Reset it
                        data->voice[currVoice].burstCounter += data->voice[currVoice].burstPeriod;
                    }

                    configASSERT(data->voice[currVoice].counter >= 0);
                }
            }
            
            //find timers that would trigger within the ontime or holdoff time of the current pulse
            
			//check if any other timers would trigger within the ontime and or the holdoff time
            for(uint32_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
                //if we are in TR mode then only voice 1 is active and all others can be ignored
                if(synthMode == SYNTH_TR && currVoice >= 1) break;
                
                int32_t currentMinimumPeriod = (nextPulse.onTime + param.offtime);
                
                if(data->voice[currVoice].limitedEnabled){ 
                    //would the note pulse occur within the pulse or during the holdoff time after it?
                    if(data->voice[currVoice].counter < currentMinimumPeriod){ 
                        
                        data->voice[currVoice].counter += data->voice[currVoice].limitedPeriod;
                        
                        if(data->voice[currVoice].noiseAmplitude){
                            //generate 32bit random number
                            int32_t randomizer = rand()*rand();
                            //limit frequency change to 0.5-2.0x fBase or whatever noise Amplitude is set to
                            int32_t noiseLimit = (data->voice[currVoice].noiseAmplitude > data->voice[currVoice].limitedPeriod) ? data->voice[currVoice].limitedPeriod : data->voice[currVoice].noiseAmplitude;
                            randomizer = (randomizer & noiseLimit) - (noiseLimit>>1);
                            if(abs(randomizer) < data->voice[currVoice].counter) data->voice[currVoice].counter += randomizer;
                        }  //TODO verify use of "&" as randomizer scaling
                        
                        //did this timer already trigger during this pulse length? If so we just skip it
                        if(!data->voice[currVoice].alreadyTriggered){
                            //overlay current pulse
                            SigGen_overlayPulse(&nextPulse, data->voice[currVoice].pulseVolume, data->voice[currVoice].limitedPulseWidth_us);

                            //block timer for this iteration
                            data->voice[currVoice].alreadyTriggered = 1;

                            //and finally go back to the start of the list
                            currVoice = 0;
                        }
                    }
                    
                    //would the voices HPV pulse trigger during the deadtime?
                    if(data->voice[currVoice].currHPVDivider == 0 && data->voice[currVoice].currHPVCounter <= currentMinimumPeriod){ 
                        //disable HPV counter
                        data->voice[currVoice].currHPVCounter = INT_MAX;
                        
                        //is HPV still supposed to be on?
                        if(data->voice[currVoice].limitedHpvCount != 0 && !data->voice[currVoice].noiseAmplitude){
                            //yes :)
                            data->voice[currVoice].currHPVDivider = data->voice[currVoice].limitedHpvCount;
                        }else{
                            //no :(
                            data->voice[currVoice].currHPVDivider = INT_MAX;
                        }
                        
                        //did this timer already trigger during this pulse length? If so we just skip it
                        if(!data->voice[currVoice].alreadyTriggered){
                            //no => add the pulse volume to the current one. Check if the width is wider than the current one, if so use it instead
                            
                            //overlay current pulse
                            SigGen_overlayPulse(&nextPulse, data->voice[currVoice].hpvVolume, data->voice[currVoice].limitedHpvPulseWidth_us);

                            //block timer for this iteration
                            data->voice[currVoice].alreadyTriggered = 1;

                            //and finally go back to the start of the list
                            currVoice = 0;
                        }
                    }
                }
            }
            
            //limit volume of pulse
            if(nextPulse.onTime > MAX_VOL) nextPulse.onTime = MAX_VOL;
            
            //check if the voice fron which the pulse originates is muted due to burst off time. If it is then we just set the pulse current and ontime to zero.
            //we still need to add it however to prevent the buffer from running dry and the load exceeding 100%
            if(data->voice[nextVoice].burstPeriod != 0 && data->voice[nextVoice].burstCounter < data->voice[nextVoice].burstOt){
                nextPulse.onTime = 0;
                nextPulse.current = 0;
            }
            
            //we were here
            
            //write it into the buffer
            if (!SigGen_queuePulse(&nextPulse)) {
                //TODO maybe send an alarm?
                
                //theoretically it might be cleverer to continue; here, but that could create a situation where we get stuck in this loop, so break instead
                break;
            }
        }
        
        //it is possible that the timer is turned of at this point in the code but a pulse is waiting. If that is the case the timer needs to be kickstarted so it can begin reading out more pulses by itself
        if(!SigGen_isTimerRunning() && RingBuffer_getDataCount(data->pulseBuffer) > 0){
            //time is off but pulses are waiting. Load the first pulse and start the timer
            
            //get the next pulse
            if(RingBuffer_read(data->pulseBuffer, (void*)&readPulse, 1) == 1){
                taskData->bufferLengthInCounts -= readPulse.period;
                
                if(readPulse.period < SIGGEN_MIN_PERIOD) readPulse.period = SIGGEN_MIN_PERIOD;
                
                //reset counter to trigger asap
                interrupterTimebase_WriteCounter(0);
                
                //load timer registers
                interrupterTimebase_WriteCompare(readPulse.period);
                
                //clear the irq incase its still pending
                interrupterIRQ_ClearPending();
                
                //and finally re-enable the timer
                SigGen_startTimer();
            }
            
            //wait what? Read failed although there is supposedly data in the buffer... anyway, forget what we are doing and just carry on the loop
        }
#ifdef SIMULATOR
        simulator_process_audio(data, &readPulse);
#endif
    }
}
