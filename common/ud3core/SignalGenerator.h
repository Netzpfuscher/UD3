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
    
    #define N_MIDICHANNEL 16
    #define SID_CHANNELS 3
    
    enum ADSR{
    ADSR_IDLE = 0,
    ADSR_PENDING,
    ADSR_ATTACK,
    ADSR_DECAY,
    ADSR_SUSTAIN,
    ADSR_RELEASE,
    };
    
    struct _filter{
        uint16_t min;
        uint16_t max;
        uint8_t channel[N_MIDICHANNEL];
    };
    
    typedef struct __channel__ {
	uint8_t midich;	// Channel of midi (0 - 15)
	uint8_t miditone;  // Midi's tone number (0-127)
	int32_t volume;	// Volume (0 - 127) 7bit.16bit
	uint8_t updated;   // Was it updated?
    uint16_t halfcount;
    uint32_t freq;
    uint8_t adsr_state;
    uint8_t sustain;
    uint8_t old_gate;
    uint8_t noise;
    } CHANNEL;
    
    extern CHANNEL channel[N_CHANNEL];

    extern struct _filter filter;

    typedef uint32_t Q16n16;
    
    extern xQueueHandle qSID;
    
    Q16n16  Q16n16_mtof(Q16n16 midival_fractional);
    
    void SigGen_channel_enable(uint8_t ch, uint32_t ena);
    uint16_t SigGen_channel_freq_fp8(uint8_t ch, uint32_t freq);
    void SigGen_noise(uint8_t ch, uint32_t ena, uint32_t rnd);
    void SigGen_init();
    void SigGen_switch_synth(uint8_t synth);
    void SigGen_kill();
    size_t SigGen_get_channel(uint8_t ch);
    void SigGen_setNoteTPR(uint8_t voice, uint32_t freqTenths);
    void SigGen_limit();
    void SigGen_calc_hyper(uint8_t voice, uint32_t word);
    
    uint8_t CMD_SynthMon(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
    uint8_t callback_synthFilter(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
    uint8_t callback_SynthFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
        
    
    enum SYNTH{
        SYNTH_OFF=0,
        SYNTH_MIDI=1,
        SYNTH_SID=2,
        SYNTH_MIDI_QCW=3,
        SYNTH_SID_QCW=4
    };
    
    
#endif