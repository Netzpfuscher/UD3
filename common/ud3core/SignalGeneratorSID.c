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


#include "SignalGeneratorSID.h"
#include <device.h>
#include "queue.h"
#include "telemetry.h"
#include "clock.h"
#include "qcw.h"
#include <stdlib.h>

xQueueHandle qSID;
struct sid_f sid_frm;

//uint8_t old_flag[N_CHANNEL];

static const uint32_t attack_env[16] = {520192,130048,65024,43349,27378,18578,15299,13004,10403,4161,2080,1300,1040,346,208,130};
static const uint32_t decrel_env[16] = {173397,43349,21674,14449,9126,6192,5099,4334,3467,1387,693,433,346,115,69,43};

void SigGen_SID_init(){
    qSID = xQueueCreate(N_QUEUE_SID, sizeof(struct sid_f));
}

static inline void compute_adsr_sid(uint8_t ch){
    switch (channel[ch].adsr_state){
            case ADSR_ATTACK:
                SigGen_channel_enable(ch,1);
                channel[ch].volume+=attack_env[sid_frm.attack[ch]];
                if(channel[ch].volume>=MAX_VOL){
                    channel[ch].volume=MAX_VOL;
                    channel[ch].adsr_state=ADSR_DECAY;
                }
            break;
            case ADSR_DECAY:
                channel[ch].volume-=decrel_env[sid_frm.decay[ch]];
                if(channel[ch].volume<=((int32_t)sid_frm.sustain[ch]<<16)){
                    channel[ch].volume=(uint32_t)sid_frm.sustain[ch]<<16;
                    channel[ch].adsr_state=ADSR_SUSTAIN;
                }
            break;
            case ADSR_SUSTAIN:
                channel[ch].volume=(uint32_t)sid_frm.sustain[ch]<<16;
            break;
            case ADSR_RELEASE:
                channel[ch].volume-=decrel_env[sid_frm.release[ch]];
                if(channel[ch].volume<=0){
                    channel[ch].volume=0;
                    channel[ch].adsr_state=ADSR_IDLE;
                    SigGen_channel_enable(ch,0);
                }
            break;
        }
}

uint32_t volatile next_frame=4294967295;
uint32_t last_frame=4294967295;

void synthcode_SID(uint32_t r){
  
    tt.n.midi_voices.value=0;
    
    if (l_time > next_frame){
        if(xQueueReceiveFromISR(qSID,&sid_frm,0)){
            last_frame=l_time;
            for(uint8_t i=0;i<SID_CHANNELS;i++){
                if(sid_frm.gate[i] > channel[i].old_gate) {
                    channel[i].adsr_state=ADSR_ATTACK;  //Rising edge
                }
                if(sid_frm.gate[i] < channel[i].old_gate) channel[i].adsr_state=ADSR_RELEASE;  //Falling edge
                sid_frm.pw[i]=sid_frm.pw[i]>>4;
                channel[i].old_gate = sid_frm.gate[i];
                if(sid_frm.freq_fp8[i]){
                    channel[i].halfcount = (uint32_t)(SG_CLOCK_HALFCOUNT<<8)/sid_frm.freq_fp8[i];
                    channel[i].freq = SigGen_channel_freq_fp8(i,sid_frm.freq_fp8[i]); 
                }else{
                    channel[i].halfcount = 0; 
                    channel[i].freq = 0;
                    SigGen_channel_enable(i,0);
                }
                
            }
            next_frame = sid_frm.next_frame;
        }
    }
    
    if((l_time - last_frame)>100){
     next_frame = l_time;
     last_frame = l_time;
        for (uint8_t i = 0;i<SID_CHANNELS;++i) {
            channel[i].volume = 0;
            channel[i].adsr_state = ADSR_IDLE;
            channel[i].old_gate=0;
            SigGen_channel_enable(i,0);
        }   
    }
    
    uint32_t rnd = rand();
    uint32_t flag;

    for (uint32_t ch = 0; ch < SID_CHANNELS; ch++) {
        flag=0;
        compute_adsr_sid(ch);
        if(channel[ch].volume > 0 && channel[ch].freq){
            tt.n.midi_voices.value++;
            if(sid_frm.wave[ch]){
                interrupter_set_pw_vol(ch,sid_frm.master_pw,channel[ch].volume/configuration.noise_vol_div);
            }else{
                interrupter_set_pw_vol(ch,sid_frm.master_pw,channel[ch].volume);
            }

            if(sid_frm.test[ch]){
                SigGen_noise(ch, 1 ,0);  //Not for noise only to set accu to 0
            }

            if (sid_frm.wave[ch] && (r / channel[ch].halfcount) % 2 > 0) {
                flag=1;
			}  
            if(flag > channel[ch].old_flag) SigGen_noise(ch, sid_frm.wave[ch],rnd);
            channel[ch].old_flag = flag;
        }
	}
}

void synthcode_QSID(uint32_t r){
    qcw_handle_synth();
    
    tt.n.midi_voices.value=0;
    
    if (l_time > next_frame){
        if(xQueueReceiveFromISR(qSID,&sid_frm,0)){
            last_frame=l_time;
            for(uint8_t i=0;i<SID_CHANNELS;i++){
                channel[i].freq = sid_frm.freq_fp8[i]>>8;
                channel[i].halfcount = (uint32_t)(SG_CLOCK_HALFCOUNT<<8)/sid_frm.freq_fp8[i];
                if(sid_frm.gate[i] > channel[i].old_gate){
                    channel[i].adsr_state=ADSR_ATTACK;  //Rising edge
                    if(!QCW_enable_Control) qcw_start();
                }
                if(sid_frm.gate[i] < channel[i].old_gate) channel[i].adsr_state=ADSR_RELEASE;  //Falling edge
                sid_frm.pw[i]=sid_frm.pw[i]>>4;
                
                channel[i].old_gate = sid_frm.gate[i];
            }
            next_frame = sid_frm.next_frame;
        }
    }
    if((l_time - last_frame)>200){
     next_frame = l_time;
     last_frame = l_time;
        for (uint8_t i = 0;i<SID_CHANNELS;++i) {
            channel[i].volume = 0;
            channel[i].adsr_state = ADSR_IDLE;
            channel[i].old_gate=0;
        }   
    }
        
    
    int8_t random = ((uint8_t)rand())-127;
    
    int32_t vol=0;
	
	for (uint8_t ch = 0; ch < SID_CHANNELS; ch++) {
        compute_adsr_sid(ch);

		if (channel[ch].volume > 0) {
            tt.n.midi_voices.value++;
			if ((r / channel[ch].halfcount) % 2 > 0) {
                if(sid_frm.wave[ch]){
                    vol +=(((int)channel[ch].volume*random)/127);
                }else{
                    vol +=channel[ch].volume;
                }
			}else{
                if(!sid_frm.wave[ch]){
                    vol -=channel[ch].volume;
                }
            }
		}   
	}
    vol/=3;
    vol>>=16;
    if(vol>127)vol=127;
    if(vol<-128)vol=-128;
    qcw_modulate(vol+0x80);    
}

