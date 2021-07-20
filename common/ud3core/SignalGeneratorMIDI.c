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

#include "SignalGenerator.h"
#include "SignalGeneratorMIDI.h"
#include <device.h>
#include "tasks/tsk_midi.h"
#include "telemetry.h"
#include "interrupter.h"
#include "qcw.h"

static inline void compute_adsr_midi(uint8_t ch){
	switch (channel[ch].adsr_state){
        case ADSR_ATTACK:
            if(midich[channel[ch].midich].attack){
                channel[ch].volume+=(128-midich[channel[ch].midich].attack)<<8;
            }else{
                channel[ch].adsr_state=ADSR_SUSTAIN;
            }
            if(channel[ch].volume>=MAX_VOL){
                channel[ch].volume=MAX_VOL;
                channel[ch].adsr_state=ADSR_DECAY;
            }
        break;
        case ADSR_DECAY:
            if(midich[channel[ch].midich].attack){
                channel[ch].volume-=(128-midich[channel[ch].midich].decay)<<8;
            }else{
                channel[ch].adsr_state=ADSR_SUSTAIN;
            }
            if(channel[ch].volume<=((int32_t)channel[ch].sustain<<16)){
                channel[ch].volume=(uint32_t)channel[ch].sustain<<16;
                channel[ch].adsr_state=ADSR_SUSTAIN;
            }
        break;
        case ADSR_SUSTAIN:
            channel[ch].volume = channel[ch].sustain<<16;
        break;
        case ADSR_RELEASE:
            if(midich[channel[ch].midich].release){
                channel[ch].volume-=(128-midich[channel[ch].midich].release)<<8;
            }else{
                channel[ch].volume=0;
                channel[ch].adsr_state=ADSR_IDLE;
            }
            if(channel[ch].volume<=0){
                channel[ch].volume=0;
                channel[ch].adsr_state=ADSR_IDLE;
            }
        break;
    }
}

void synthcode_MIDI(){
    tt.n.midi_voices.value=0;
    
	for (uint8_t ch = 0; ch < N_CHANNEL; ch++) {
        compute_adsr_midi(ch);
        if(channel[ch].volume>0){
            tt.n.midi_voices.value++;
            interrupter_set_pw_vol(ch,interrupter.pw,channel[ch].volume);
            SigGen_channel_enable(ch,1);
        }else{
            SigGen_channel_enable(ch,0); 
        }
    }    
}

void synthcode_QMIDI(uint32_t r){
    qcw_handle_synth();
    int32_t vol=0;
    tt.n.midi_voices.value=0;
	for (uint8_t ch = 0; ch < N_CHANNEL; ch++) {

        compute_adsr_midi(ch);

		if (channel[ch].volume > 0) {
            tt.n.midi_voices.value++;
			if ((r / channel[ch].halfcount) % 2 > 0) {
                vol +=channel[ch].volume;
			}else{
                vol -=channel[ch].volume;
            }
		}
	}
    vol>>=16;
    if(vol>127)vol=127;
    if(vol<-128)vol=-128;
    qcw_modulate(vol+0x80);  
}