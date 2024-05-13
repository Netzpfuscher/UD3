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

#if !defined(SignalGenerator_H)
#define SignalGenerator_H
    
    
    #include <stdint.h>
    #include "config.h"
    #include "FreeRTOS.h"
    #include "TTerm.h"
    #include "cli_basic.h"
    
    enum SYNTH{
        SYNTH_OFF=0,
        SYNTH_MIDI=1,
        SYNTH_SID=2,
        SYNTH_TR=3,
        SYNTH_MIDI_QCW=4,
        SYNTH_SID_QCW=5,
    };

    #include "RingBuffer/include/RingBuffer.h"

    #define SIGGEN_VERSION 1

    //maximum possible ontime with the current configuration in us. also important: the maximum value for overflow free calculations is 0xffff = 65535 !
    #define SIGGEN_MAXOT 10900

    #define SIGGEN_OUTPUTCOUNT 1
    #define SIGGEN_VOICECOUNT 6

    #define SIGGEN_PULSEBUFFER_SIZE 128
    #define SIGGEN_MIN_OT 10    //TODO evaluate this... maybe make it bigger
    #define SIGGEN_MIN_PERIOD 1000    //TODO evaluate this...

    #define SIGGEN_PARAM_AUX_R_MODE 0
    #define SIGGEN_DEFAULT_PARAM_AUX_R_MODE AUXMODE_AUDIO_OUT
    #define SIGGEN_PARAM_AUX_L_MODE 1
    #define SIGGEN_DEFAULT_PARAM_AUX_L_MODE AUXMODE_AUDIO_OUT

    typedef struct{
        //time references
        int32_t counter;
        int32_t burstCounter;
        int32_t currHPVCounter;
        int32_t currHPVDivider;
        
        int32_t period;
        int32_t limitedPeriod;
        
        int32_t noteAge;
        int32_t hpvOffset;
        int32_t hpvCount;
        int32_t limitedHpvCount;
        
        //flags
        int32_t enabled;
        int32_t limitedEnabled;
        int32_t noiseAmplitude;
        int32_t alreadyTriggered;
        
        //pulse parameters
        int32_t frequencyTenths;
        int32_t hpvPhase;
        
        int32_t hpvVolume;
        int32_t pulseVolume;
        
        int32_t initialPulseVolume;
        int32_t initialHpvVolume;
        
        int32_t pulseWidth_us;
        int32_t limitedPulseWidth_us;
        
        int32_t hpvPulseWidth_us;
        int32_t limitedHpvPulseWidth_us;
        
        //burst parameters
        int32_t burstPeriod;
        int32_t burstOt;
    } SigGen_voiceData_t;

    typedef struct{
        volatile uint32_t period;
        volatile uint32_t onTime;
        volatile uint32_t current;
        volatile uint32_t sourceVoices;
        volatile uint32_t editedVoices;
    } volatile SigGen_pulseData_t;

    typedef struct{
        SigGen_voiceData_t voice[SIGGEN_VOICECOUNT];
        
        //parameters
        volatile int32_t bufferLengthInCounts;
        
        volatile RingBuffer_t * pulseBuffer;
        
        volatile uint32_t pulseFlopFlip;
        
        //tr parameters
        uint32_t trBurstCounter;
        uint32_t trBurstState;
        uint32_t trBurstOnTime;
        uint32_t trBurstOffTime;
        
    } volatile SigGen_taskData_t;
    
    extern uint32_t VoiceFlags[];

    typedef enum {AUXMODE_AUDIO_OUT, AUXMODE_E_STOP} AuxMode_t;

    void SigGen_init();

    uint32_t SigGen_isVoiceOn(uint32_t voice);

    uint32_t SigGen_getCurrDuty();

    uint32_t SigGen_otToVolume(uint32_t ontime);
    uint32_t SigGen_volumeToOT(uint32_t volume);

    void SigGen_setMasterVol(uint32_t newVolume);
    
    void SigGen_switchSynthMode(uint8_t newMode);
    
    void SigGen_setVoiceTR(uint32_t enabled, int32_t pulseWidth, int32_t volume, int32_t frequencyTenths, int32_t burstOn_us, int32_t burstOff_us);
    void SigGen_setVoiceSID(uint32_t voice, uint32_t enabled, int32_t pulseWidth, int32_t volume, int32_t frequencyTenths, int32_t noiseAmplitude);
    void SigGen_setHyperVoiceVMS(uint32_t voice, uint32_t count, uint32_t pulseWidth, uint32_t volume, uint32_t phase);
    void SigGen_setVoiceVMS(uint32_t voice, uint32_t enabled, int32_t pulseWidth, int32_t volume, int32_t frequencyTenths, int32_t noiseAmplitude, int32_t burstOn_us, int32_t burstOff_us);
    
    void SigGen_limit();
    void SigGen_setOutputEnabled(uint32_t en);
    void SigGen_killAudio();
    uint32_t SigGen_isOutputOn();
    uint32_t SigGen_wasOutputOn();
    
#endif