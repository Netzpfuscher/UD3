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

#if !defined(SignalGeneratorSID_H)
#define SignalGeneratorSID_H
    #include <stdint.h>
    #include "SignalGenerator.h"
    
    #define SID_CHANNELS 3
    
    void synthcode_SID(uint32_t r);
    void synthcode_QSID(uint32_t r);
    void SigGen_SID_init();
    
    
    enum ADSR{
    ADSR_IDLE = 0,
    ADSR_PENDING,
    ADSR_ATTACK,
    ADSR_DECAY,
    ADSR_SUSTAIN,
    ADSR_RELEASE,
    };
    
    typedef struct __channel__ {
	int32_t volume;	// Volume (0 - 127) 7bit.16bit
    uint16_t halfcount;
    uint32_t freq;
    uint8_t adsr_state;
    uint8_t old_gate;
    uint8_t noise;
    uint8_t  old_flag;
    } CHANNEL;
    
    extern CHANNEL channel[N_CHANNEL];
    
    struct sid_f{
    uint32_t freq_fp8[SID_CHANNELS];
    uint16_t pw[SID_CHANNELS];
    uint8_t gate[SID_CHANNELS];
    uint8_t test[SID_CHANNELS];
    uint8_t wave[SID_CHANNELS];
    uint8_t attack[SID_CHANNELS];
    uint8_t decay[SID_CHANNELS];
    uint8_t sustain[SID_CHANNELS];
    uint8_t release[SID_CHANNELS];
    uint16_t master_pw;
    uint32_t next_frame;
};
        
#endif