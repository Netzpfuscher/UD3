/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

/* [] END OF FILE */
#include "tsk_eth_common.h"
#include "cyapicallbacks.h"
#include <cytypes.h>
#include <device.h>
#include "telemetry.h"
#include "alarmevent.h"
#include "cli_common.h"
#include "tasks/tsk_midi.h"
#include "tasks/tsk_fault.h"
#include "clock.h"
#include "SignalGeneratorSID.h"

void process_midi(uint8_t* ptr, uint16_t len) {
	uint8_t c;
    static uint8_t midi_count = 0;
    static uint8_t midiMsg[3];
    
	while (len) {
		c = *ptr;
        len--;
        ptr++;
		if (c & 0x80) {
			midi_count = 1;
			midiMsg[0] = c;

			goto end;
		} else if (!midi_count) {
            goto end;
		}
		switch (midi_count) {
		case 1:
			midiMsg[1] = c;
			midi_count = 2;
            if((midiMsg[0] & 0xF0) == 0xC0){
                midi_count = 0;
                midiMsg[2]=0;
                USBMIDI_1_callbackLocalMidiEvent(0, midiMsg);
            }
			break;
		case 2:
			midiMsg[2] = c;
			midi_count = 0;
			if (midiMsg[0] == 0xF0) {
				if (midiMsg[1] == 0x0F) {
					WD_reset();
					goto end;
				}
			}
			USBMIDI_1_callbackLocalMidiEvent(0, midiMsg);
			break;
		}
	end:;
	}
}

#define SID_BYTES       29

void process_min_sid(uint8_t* ptr, uint16_t len) {
    uint8_t n_frames = *ptr;
    len--;
    ptr++;
    
    if(len!=(SID_BYTES*n_frames)){
        alarm_push(ALM_PRIO_WARN,"SID: Malformed SID frame", ALM_NO_VALUE);
        return;
    }
    
    struct sid_f SID_frame;

    while(n_frames){
        uint16_t dutycycle=0;
        uint32_t freq_temp;
        for(uint32_t i = 0;i<SID_CHANNELS;i++){
            //SID_FREQLO1
            SID_frame.freq_fp8[i] = *ptr;
            ptr++;
            //SID_FREQHI1
            SID_frame.freq_fp8[i] |= ((uint16_t)*ptr << 8);
            SID_frame.freq_fp8[i] = (SID_frame.freq_fp8[i]<<15)/2179;
            freq_temp = SID_frame.freq_fp8[i]>>8;                      
            ptr++;
            //SID_PWLO1
            SID_frame.pw[i] = (SID_frame.pw[i] & 0xff00) + *ptr;
            ptr++;
            //SID_PWHI1
            SID_frame.pw[i] = (SID_frame.pw[i] & 0xff) + ((uint16_t)*ptr << 8);
            ptr++;
            //SID_CR1
            SID_frame.wave[i] = bit_is_set(*ptr, 7);
            SID_frame.gate[i] = bit_is_set(*ptr, 0);
            
            //TODO reimplement this?
            /*if(filter.channel[i]==0 || freq_temp > filter.max || freq_temp < filter.min || freq_temp == 0){
                SID_frame.gate[i]=0;
            }
            if(SID_frame.wave[i] && filter.noise_disable){
                SID_frame.gate[i] = 0;
                SID_frame.freq_fp8[i] = 0;
            }*/
            if(*ptr & 0x08){
                SID_frame.test[i]=1;
            }else{
                SID_frame.test[i]=0;
            }
            ptr++;
            //SID_AD1
            SID_frame.attack[i] = (*ptr >> 4) & 0x0F;
            SID_frame.decay[i] = (*ptr) & 0x0F;
            ptr++;
            //SID_SR1
            SID_frame.sustain[i] = ((*ptr & 0xF0) >> 1);
            SID_frame.release[i] = (*ptr) & 0x0F;
            ptr++;
            
            //Calculate dutycycle
            if (SID_frame.gate[i]){
                if(SID_frame.wave[i]){
                    dutycycle+= (((uint32)127*(uint32)param.pw)/(127000ul/freq_temp)); //Noise
                }else{
                    dutycycle+= ((uint32)127*(uint32)param.pw)/(127000ul/freq_temp); //Normal wave
                }
            }
        }
        //SID_FCLO
        ptr++;
        //SID_FCHI
        ptr++;
        //SID_Res_Filt
        ptr++;
        //SID_Mode_Vol
        ptr++;
        //SID_UD_TIME0
        SID_frame.next_frame = *ptr<<24;
        ptr++;
        //SID_UD_TIME1
        SID_frame.next_frame |= *ptr<<16;
        ptr++;
        //SID_UD_TIME2
        SID_frame.next_frame |= *ptr<<8;
        ptr++;
        //SID_UD_TIME3
        SID_frame.next_frame |= *ptr;
        ptr++;
        n_frames--;
        
        if(dutycycle>configuration.max_tr_duty - param.temp_duty){
            SID_frame.master_pw = (param.pw * (configuration.max_tr_duty - param.temp_duty)) / dutycycle;
        }else{
            SID_frame.master_pw = param.pw;
        }  
        int32_t diff = SID_frame.next_frame - l_time;
        if(diff<-1000){
            alarm_push(ALM_PRIO_WARN,"SID: Old SID frame received", diff);
        }else{   
            xQueueSend(qSID,&SID_frame,0);
        }
        
    }
   
}
