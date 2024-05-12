/*
 * UD3 - NVM
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

#include "SignalGeneratorSID.h"

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


static SigGen_taskData_t * taskData;

static uint32_t isOutputInErrorState = 0;

static void SigGen_task(void * params);

static uint32_t masterVolume = MAX_VOL;
static uint8_t synthMode = SYNTH_OFF;

//current pulse the timer is waiting to run
static SigGen_pulseData_t readPulse;

uint32_t VoiceFlags[SIGGEN_VOICECOUNT];

static uint32_t isEnabled = 1;

static volatile uint32_t voicesToEradicate = 0;

#define SIGGEN_MS_TO_PERIOD_COUNT(X) X * 32000
#define SIGGEN_US_TO_PERIOD_COUNT(X) X * 32
#define SIGGEN_US_TO_OT_COUNT(X) X
#define SIGGEN_VOLUME_TO_CURRENT_DAC_VALUE(X) (params.min_tr_cl_dac_val + ((X * params.diff_tr_cl_dac_val) >> 15))


#define SigGen_isTimerRunning() (interrupterTimebase_ReadControlRegister() & interrupterTimebase_CTRL_ENABLE)
#define SigGen_startTimer() interrupterTimebase_WriteControlRegister(interrupterTimebase_ReadControlRegister() | interrupterTimebase_CTRL_ENABLE); SigGen_enableTimerISR();
#define SigGen_stopTimer() interrupterTimebase_WriteControlRegister(interrupterTimebase_ReadControlRegister() & ~interrupterTimebase_CTRL_ENABLE); SigGen_disableTimerISR();

#define SigGen_isTimerISREnabled() interrupterIRQ_GetState()
#define SigGen_disableTimerISR() interrupterIRQ_Disable()
#define SigGen_enableTimerISR() interrupterIRQ_Enable()

static void SigGen_setHyperVoiceParams(uint32_t voice, uint32_t count, uint32_t pulseWidth, uint32_t volume, uint32_t phase);
static void SigGen_setVoiceParams(uint32_t voice, uint32_t enabled, int32_t pulseWidth, int32_t volume, int32_t frequencyTenths, int32_t noiseAmplitude, int32_t burstOn_us, int32_t burstOff_us);

uint32_t IsOkToPrint = 0;

CY_ISR(isr_synth) {
    uint32_t r = SG_Timer_ReadCounter();
    
    
    clock_tick();
    if(qcw_reg){
        qcw_handle();
        return;
    }
    switch(param.synth){
        case SYNTH_SID:
            synthcode_SID(r);
            break;
        case SYNTH_SID_QCW:
            synthcode_QSID(r);
            break;
        default:
            break;
    }
    
}
    
CY_ISR(SigGen_PulseTimerISR){
    interrupterTimebase_ReadStatusRegister();
    interrupterIRQ_ClearPending();
    
    //start the previous pulse
    if(!(readPulse.current == 0 || readPulse.onTime == 0)) interrupter_oneshotRaw(readPulse.onTime, readPulse.current);
    
    //try to read the next pulse
    while(1){
        if(RingBuffer_readFromISR(taskData->pulseBuffer, &readPulse, 1) == 1){
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

void SigGen_init(){
    //initialize flags needed for masking voices
    for(uint32_t i = 0; i < SIGGEN_VOICECOUNT; i++){
        VoiceFlags[i] = 1<<i;
    }
    
    SigGen_taskData_t * data = pvPortMalloc(sizeof(SigGen_taskData_t));
    memset(data, 0, sizeof(SigGen_taskData_t));
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
 /*

//TODO figure out how to implement the compressor... we aren't scaling the ontime anymore really... not sure what to do :(

//calculate ontime in microseconds from a target volume
uint32_t SigGen_scalePulseWidth_us(uint32_t pulseWidth_us){
    //TODO add non-linear volume to ontime conversion
	
    if(pulseWidth_us > configuration.max_tr_pw) pulseWidth_us = configuration.max_tr_pw;
    
    //scale if master is lower than max
    if(masterVolume < VOLUME_MAX) volume = (volume * masterVolume) >> 16;
    
    //how about the duty compressor
    if(Comp_getGain() < VOLUME_MAX) volume = (volume * Comp_getGain()) >> 16;
    
    //scale 0-0x10000 (vol_max) to minOnTime-maxOnTime
    uint32_t ontimeDifference = currentCCI->maxOnTime - currentCCI->minOnTime;
    return ((volume * ontimeDifference) >> 16) + currentCCI->minOnTime;
}*/

//calculate the dutycycle and return it in percent
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
    
    return totalDuty;
}

//phase is 0-100% fixed point with a maximum of 1024=1
static void SigGen_setHyperVoiceParams(uint32_t voice, uint32_t count, uint32_t pulseWidth, uint32_t volume, uint32_t phase){
    if(count == 0 || taskData->voice[voice].noiseAmplitude || masterVolume == 0){
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

uint32_t SigGen_isVoiceOn(uint32_t voice){
    return taskData->voice[voice].enabled;
}

uint8_t divder = 0xff;

static void SigGen_setVoiceParams(uint32_t voice, uint32_t enabled, int32_t pulseWidth, int32_t volume, int32_t frequencyTenths, int32_t noiseAmplitude, int32_t burstOn_us, int32_t burstOff_us){
    IsOkToPrint = 1;
    //if(--divder == 0) TERM_printDebug(min_handle[1], "set voice param: %d en=%d freq=%d pw=%d vol=%d burstOn=%d burstOff=%d\r\n", voice, enabled, frequencyTenths, pulseWidth, volume, burstOn_us, burstOff_us);
    
    //check if voice is switching on
    uint32_t willBeOn = enabled && (volume != 0) && (frequencyTenths != 0);
    
    //is the voice switching on?
    if(!taskData->voice[voice].enabled && willBeOn){
        //yes voice was off before and will be on after this. Check pulsebuffer length
        if(taskData->bufferLengthInCounts > SIGGEN_MS_TO_PERIOD_COUNT(10)){
            //pulsebuffer is massive at the moment and will cause a long delay, trigger print
            TERM_printDebug(min_handle[1], "lag! %d Count=%d\r\n", --divder, RingBuffer_getDataCount(taskData->pulseBuffer));
        }
        
        //also reset the timebase for the note
        taskData->voice[voice].counter = 0;
        
    //or are we switching it off?
    }else if(taskData->voice[voice].enabled && !willBeOn){
        //yes switching off => make sure to remove any remaining pulses from the buffer if its long
        //why only if its long? Well because a short buffer will mean a non-noticeable short lag which we can ignore to prevent unneccessary cpu usage :)
        if(taskData->bufferLengthInCounts > SIGGEN_MS_TO_PERIOD_COUNT(2)){
            taskENTER_CRITICAL();
            voicesToEradicate |= VoiceFlags[voice];
            taskEXIT_CRITICAL();
        }
    }
    
    taskData->voice[voice].enabled = willBeOn;
    
    if(!taskData->voice[voice].enabled || masterVolume == 0){ 
        taskData->voice[voice].limitedEnabled = 0;
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

//wrappers for writing the voice configs from the different synthesizer modes
void SigGen_setVoiceVMS(uint32_t voice, uint32_t enabled, int32_t pulseWidth, int32_t volume, int32_t frequencyTenths, int32_t noiseAmplitude, int32_t burstOn_us, int32_t burstOff_us){
    if(synthMode != SYNTH_MIDI) return;
    SigGen_setVoiceParams(voice, enabled, pulseWidth, volume, frequencyTenths, noiseAmplitude, burstOn_us, burstOff_us);
}
void SigGen_setHyperVoiceVMS(uint32_t voice, uint32_t count, uint32_t pulseWidth, uint32_t volume, uint32_t phase){
    if(synthMode != SYNTH_MIDI) return;
    SigGen_setHyperVoiceParams(voice, count, pulseWidth, volume, phase);
}

//TODO implement writing sid as previously used in stickfigurator sid write
/*private void generateAndWriteVoiceSettings(Synth device2) {

	int[] freq = new int[4];
	int[] volume = new int[4];
	int[] noise = new int[4];
	int[] hyperVCount = new int[4];
	int[] hyperVPhase = new int[4];

	freq[0] = device2.sidV1Enabled ? chip.ch1.getFreq() : 0;           
	volume[0] = device2.sidV1Enabled ? chip.ch1.getVolume() : 0;
	noise[0] = chip.ch1.getNoise();     //noise ? 80 : 0                          
	
	if(chip.ch1.square && hpvEn && chip.ch1.PulseWidth != 2048) { hyperVCount[0] = 2; hyperVPhase[0] = chip.ch1.PulseWidth >> 4; }
	if(chip.ch2.square && hpvEn && chip.ch2.PulseWidth != 2048) { hyperVCount[1] = 2; hyperVPhase[1] = chip.ch2.PulseWidth >> 4; }
	if(chip.ch3.square && hpvEn && chip.ch3.PulseWidth != 2048) { hyperVCount[2] = 2; hyperVPhase[2] = chip.ch3.PulseWidth >> 4; }
	
	device.commander.writeRaw(freq[0], volume[0], noise[0], freq[1], volume[1], noise[1], chip.ch3Off ? 0 : freq[2], chip.ch3Off ? 0 : volume[2], noise[2], 0, 0, 0, hyperVCount[0], hyperVPhase[0], hyperVCount[1], hyperVPhase[1], hyperVCount[2], hyperVPhase[2], 0, 0);
}*/

void SigGen_setVoiceSID(uint32_t voice, uint32_t enabled, int32_t pulseWidth, int32_t volume, int32_t frequencyTenths, int32_t noiseAmplitude){
    if(synthMode != SYNTH_SID) return;
    //SigGen_setVoiceParams(voice, enabled, pulseWidth, volume, frequencyTenths, noiseAmplitude);
}

void SigGen_setVoiceTR(uint32_t enabled, int32_t pulseWidth, int32_t volume, int32_t frequencyTenths, int32_t burstOn_us, int32_t burstOff_us){
    if(synthMode != SYNTH_TR) return;
    
    //TR mode always uses voice 0 without noise
    SigGen_setVoiceParams(0, enabled, pulseWidth, volume, frequencyTenths, 0, burstOn_us, burstOff_us);
}

//Function to clear the pulsebuffer of pulses from a voice thats no longer active
static void SigGen_cleanseBuffer(uint32_t voicesToRemove){
    //start by scanning until we find a pulse that will remain unchanged. During this the timer must be disabled and later re-enabled if it was on
    uint32_t isr = SigGen_isTimerISREnabled();
    SigGen_disableTimerISR();
        
        
    //go through all pulses currently waiting in the buffer and check if they originate from the voice thats switched off
    uint32_t pulsesToScan = RingBuffer_getDataCount(taskData->pulseBuffer);
    RingBuffer_startIndexedPeek(taskData->pulseBuffer);
    int32_t delayToAdd = 0;
    
    for(uint32_t i = 0; i < pulsesToScan; i++){
        
        //check pulses in the ringbuffer until we either find one that will remain in it or reach its end
        SigGen_pulseData_t * pulse = NULL;
        
        while((pulse = RingBuffer_indexedPeek(taskData->pulseBuffer, i)) != NULL){
            //pulse was read successfully => increment index
            
            //got a pulse, now check if its ONLY from the voice that will be deleted
            if(pulse->sourceVoices & voicesToRemove){
                //yes => we need to remove it and add its period to the next valid pulse
                delayToAdd += pulse->period;
                
                //invalidate pulse to make the timer skip it and load another
                pulse->period = 0;
                
                TERM_printDebug(min_handle[1], "Remove %d\r\n", i);
                
                i++;
            }else{
                //pulse we just read was not from the voice that will switch off => its the next valid one after however many ones we may skip
                
                //update its period to include any pulses we might have skipped
                pulse->period += delayToAdd;
                delayToAdd = 0;
                
                //and break out of the loop so the timer can trigger it it would've done so by now
                break;
            }
        }
        
        //loop finished. Did it do so because we reached the end of the buffer?
        if(pulse == NULL){
            //yes => break out of the loop as no more pulses can be scanned anyway
            break;
        }
        
        
        //give the timer a chance to trigger if an interrupt is pending
        if(isr){
            SigGen_enableTimerISR();
            SigGen_disableTimerISR();
        }
    }
    
    //loop finished and all pulses were iterated over. If the last pulse (or pulses) were one to be deleted then we shortened the buffer and need to update its length
    taskData->bufferLengthInCounts -= delayToAdd;
    
    //and finally reenable the timer
    if(isr) SigGen_enableTimerISR();
}

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
    
    //TODO evaluate if this is thread safe, if not make it so (as requested by the captain)
    for(uint32_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
        //if a voice is disabled we don't need to change anything
        if(!taskData->voice[currVoice].enabled) continue;
        
        //linearly scale the ontime down
        uint32_t onTime = taskData->voice[currVoice].pulseWidth_us;
        onTime = (onTime * ontimeScale) >> 15;
        taskData->voice[currVoice].limitedPulseWidth_us = onTime;
        taskData->voice[currVoice].limitedPeriod = taskData->voice[currVoice].period;
        
        //make sure to reset the counter to prevent a previously played lower note from slowing down any future ones
        if(taskData->voice[currVoice].counter > taskData->voice[currVoice].limitedPeriod) taskData->voice[currVoice].counter = taskData->voice[currVoice].limitedPeriod;
        
        //is HPV enabled? if so we need to scale that too
        if(taskData->voice[currVoice].hpvCount != 0 && taskData->voice[currVoice].noiseAmplitude == 0){
            onTime = taskData->voice[currVoice].hpvPulseWidth_us;
            onTime = (onTime * ontimeScale) >> 15;
            taskData->voice[currVoice].limitedHpvPulseWidth_us = onTime;
            
            taskData->voice[currVoice].limitedHpvCount = taskData->voice[currVoice].hpvCount;
        }else{
            taskData->voice[currVoice].limitedHpvPulseWidth_us = 0;
        }
        
        taskData->voice[currVoice].limitedEnabled = taskData->voice[currVoice].enabled;
    }
}

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

void SigGen_setOutputEnabled(uint32_t en){
	isEnabled = en;
	if(!en) SigGen_killAudio();
}

void SigGen_killAudio(){
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
}

volatile uint32_t adding = 0;

SigGen_pulseData_t currPulse;
    
volatile uint32_t compDebug = 0;
    
static void SigGen_task(void * callData){
    volatile SigGen_taskData_t * data = (SigGen_taskData_t *) callData;
    
    currPulse.onTime = 0;
    currPulse.period = 0xf;
    
    while(1){
        vTaskDelay(1);
        uint32_t noVoicesEnabled = 1;
		
		if(!isEnabled) continue;
        
        uint32_t retryCount = SIGGEN_VOICECOUNT;
        
        //check if any pulses need to be removed
        if(voicesToEradicate != 0){
            TERM_printDebug(min_handle[1], "eradicate=%01x\r\n", voicesToEradicate);
            taskENTER_CRITICAL();
            SigGen_cleanseBuffer(voicesToEradicate);
            voicesToEradicate = 0;
            taskEXIT_CRITICAL();
        }
        
        //check if we need to rewind time to prevent lags from a overly large pulsebuffer
        //TODO
        
        //check if the buffer is running low
        while(data->bufferLengthInCounts < SIGGEN_MS_TO_PERIOD_COUNT(1)){
        
            //calculate time from current pulse to next pulse
        
            //find next pulse to be triggered
            int32_t nextVoice = 0xffff;
            int32_t timeToNextPulse = INT_MAX;//data->voice[nextVoice].counter;
            
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
                            timeToNextPulse = data->voice[currVoice].currHPVCounter;
                        }else{
                            //no set it to the normal counter
                            timeToNextPulse = data->voice[currVoice].counter;
                        }
                    }

                //is the current voice's timer expiring sooner than that of the current nextVoice? 
                }else{
                    if(data->voice[currVoice].limitedEnabled && data->voice[currVoice].limitedPulseWidth_us != 0){
                        if((data->voice[currVoice].counter < timeToNextPulse) || (data->voice[currVoice].currHPVDivider == 0 && data->voice[currVoice].currHPVCounter < timeToNextPulse)){
                            nextVoice = currVoice;

                            //will the next pulse be a hypervoice pulse or not?
                            if(data->voice[currVoice].currHPVDivider == 0 && data->voice[currVoice].currHPVCounter < data->voice[currVoice].counter){
                                //yes, set the timeToNextPulse to the hpv counter
                                timeToNextPulse = data->voice[currVoice].currHPVCounter;
                            }else{
                                //no set it to the normal counter
                                timeToNextPulse = data->voice[currVoice].counter;
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
            
            noVoicesEnabled = 0;

            int32_t pulseVolume = 0;
            int32_t pulseWidth = 0;
            uint32_t pulseSource = 0;

            //now the voice of which the timer expires next is indexed by nextVoice, propagate the current time to that point (aka decrease all other time counters by the delay to the next pulse)
            for(uint32_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
                //if we are in TR mode then only voice 1 is active and all others can be ignored
                if(synthMode == SYNTH_TR && currVoice >= 1) break;
                
                if(data->voice[currVoice].limitedEnabled){ 
                    //decrement frequency timer
                    data->voice[currVoice].counter -= timeToNextPulse;
                    
                    //decrement burst timer if burst is active
                    if(data->voice[currVoice].burstPeriod != 0) data->voice[currVoice].burstCounter -= timeToNextPulse;
                    
                    //decrement hpv timer if hpv is active
                    if(data->voice[currVoice].currHPVDivider == 0) data->voice[currVoice].currHPVCounter -= timeToNextPulse;

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

                        //increase volume of current pulse
                        //TODO perhaps rework this. We need to decide what exactly we would scale, be that ontime, volume or a mix of both
                        pulseVolume += data->voice[currVoice].pulseVolume;
                        if(pulseWidth < data->voice[currVoice].limitedPulseWidth_us) pulseWidth = data->voice[currVoice].limitedPulseWidth_us;
                        pulseSource |= VoiceFlags[currVoice];

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
                                
                        //increase volume of current pulse
                        pulseVolume += data->voice[currVoice].hpvVolume;
                        if(pulseWidth < data->voice[currVoice].limitedHpvPulseWidth_us) pulseWidth = data->voice[currVoice].limitedHpvPulseWidth_us;
                        pulseSource |= VoiceFlags[currVoice];

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
            
            if(pulseVolume == 0 || pulseWidth == 0){
                //check if we retried too many times and break out of the loop then as something went wrong with the signal generation
                if(!--retryCount) break;
                
                //no pulse will be generated, try to run the loop again, if there still isn't enogh data we will break out of it after nextVoice == 0xffff
                continue;
            }
            
            //reset retry count as we actually succeeded in finding a valid pulse
            retryCount = SIGGEN_VOICECOUNT;

            //find timers that would trigger within the ontime or holdoff time of the current pulse
            
			//check if any other timers would trigger within the ontime and or the holdoff time
            for(uint32_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
                //if we are in TR mode then only voice 1 is active and all others can be ignored
                if(synthMode == SYNTH_TR && currVoice >= 1) break;
                
                int32_t currentMinimumPeriod = (pulseWidth + param.offtime);
                
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
                            //no => add the pulse volume to the current one
                            pulseVolume += data->voice[currVoice].pulseVolume;
                            if(pulseWidth < data->voice[currVoice].limitedPulseWidth_us) pulseWidth = data->voice[currVoice].limitedPulseWidth_us;
                            pulseSource |= VoiceFlags[currVoice];

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
                            
                            pulseVolume += data->voice[currVoice].hpvVolume;
                            
                            if(pulseWidth < data->voice[currVoice].limitedHpvPulseWidth_us) pulseWidth = data->voice[currVoice].limitedHpvPulseWidth_us;
                            pulseSource |= VoiceFlags[currVoice];

                            //block timer for this iteration
                            data->voice[currVoice].alreadyTriggered = 1;

                            //and finally go back to the start of the list
                            currVoice = 0;
                        }
                    }
                }
            }
            
            //limit volume of pulse
            if(pulseVolume > MAX_VOL) pulseVolume = MAX_VOL;
            
            //check if the voice fron which the pulse originates is muted due to burst off time. If it is then we just set the pulse current and ontime to zero.
            //we still need to add it however to prevent the buffer from running dry and the load exceeding 100%
            if(data->voice[nextVoice].burstPeriod != 0 && data->voice[nextVoice].burstCounter < data->voice[nextVoice].burstOt){
                pulseWidth = 0;
                pulseVolume = 0;
            }
            
            //we were here
            //convert the period, volume and ontime to the values that will need to be written into the hardware upon pulse execution
            currPulse.period = SIGGEN_US_TO_PERIOD_COUNT(timeToNextPulse);
            currPulse.onTime = SIGGEN_US_TO_OT_COUNT(pulseWidth);
            currPulse.current = SIGGEN_VOLUME_TO_CURRENT_DAC_VALUE(pulseVolume);
            currPulse.sourceVoices = pulseSource;
        
            
            //write it into the buffer
            if(RingBuffer_write(data->pulseBuffer, &currPulse, 1, 0) != 1){
                //write failed, not enough space available...
                //TODO maybe send an alarm?
                
                //theoretically it might be cleverer to continue; here, but that could create a situation where we get stuck in this loop, so break instead
                break;
            }else{
                //increase the buffersize
                SigGen_disableTimerISR();
                data->bufferLengthInCounts += currPulse.period;
                SigGen_enableTimerISR();
            }
        }
        
        //it is possible that the timer is turned of at this point in the code but a pulse is waiting. If that is the case the timer needs to be kickstarted so it can begin reading out more pulses by itself
        if(!SigGen_isTimerRunning() && RingBuffer_getDataCount(data->pulseBuffer) > 0){
            //time is off but pulses are waiting. Load the first pulse and start the timer
            
            //get the next pulse
            if(RingBuffer_read(data->pulseBuffer, &readPulse, 1) == 1){
                taskData->bufferLengthInCounts -= readPulse.period;
                
                if(readPulse.period < SIGGEN_MIN_PERIOD) readPulse.period = SIGGEN_MIN_PERIOD;
                
                //load timer registers
                interrupterTimebase_WriteCompare(readPulse.period);
    
                //reset counter to trigger asap
                interrupterTimebase_WriteCounter(readPulse.period-SIGGEN_MIN_PERIOD);
                
                //and finally re-enable the timer
                SigGen_startTimer();
            }
            
            //wait what? Read failed although there is supposedly data in the buffer... anyway, forget what we are doing and just carry on the loop
        }
    }
}