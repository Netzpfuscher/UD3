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
#include "MidiController.h"
#include <stdlib.h>

void synthcode_MIDI(uint32_t r){
    uint32_t rnd = rand();
    uint32_t flag;
	for (uint32_t ch = 0; ch < N_CHANNEL; ch++) {
        flag=0;
        if(channel[ch].volume > 0 && channel[ch].freq){
            if (Midi_voice[ch].noiseCurrent && (r / channel[ch].halfcount) % 2 > 0) {
                flag=1;
			} 
            if(flag > channel[ch].old_flag) SigGen_noise(ch, Midi_voice[ch].noiseCurrent,rnd);
            channel[ch].old_flag = flag;
        }
	}
}
