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
#define SID_FREQLO1     0
#define SID_FREQHI1     1
#define SID_PWLO1       2
#define SID_PWHI1       3
#define SID_CR1         4
#define SID_AD1         5
#define SID_SR1         6
#define SID_FREQLO2     7
#define SID_FREQHI2     8
#define SID_PWLO2       9
#define SID_PWHI2       10
#define SID_CR2         11
#define SID_AD2         12
#define SID_SR2         13
#define SID_FREQLO3     14
#define SID_FREQHI3     15
#define SID_PWLO3       16
#define SID_PWHI3       17
#define SID_CR3         18
#define SID_AD3         19
#define SID_SR3         20
#define SID_FCLO        21
#define SID_FCHI        22
#define SID_Res_Filt    23
#define SID_Mode_Vol    24
#define SID_UD_TIME0    25
#define SID_UD_TIME1    26
#define SID_UD_TIME2    27
#define SID_UD_TIME3    28
#define SID_END         29

void process_sid(uint8_t* ptr, uint16_t len) {
    static uint8_t SID_frame_marker=0;
    static uint8_t SID_register=0;
    static struct sid_f SID_frame;
    static uint8_t start_frame=0;
    uint32_t freq_temp=0;
    while(len){
        if(start_frame){
            switch(SID_register){
                case SID_FREQLO1:
                    SID_frame.freq_fp8[0] = *ptr;
                    break;
                case SID_FREQHI1:
                    SID_frame.freq_fp8[0] |= ((uint16_t)*ptr << 8);
                    SID_frame.freq_fp8[0] = (SID_frame.freq_fp8[0]<<15)/2179;
                    freq_temp = SID_frame.freq_fp8[0]>>8;
                    break;
                case SID_PWLO1:
		            SID_frame.pw[0] = (SID_frame.pw[0] & 0xff00) + *ptr;
		            break;      
                case SID_PWHI1:
		            SID_frame.pw[0] = (SID_frame.pw[0] & 0xff) + ((uint16_t)*ptr << 8);
		            break;
        		case SID_CR1:
                    if(filter.channel[0]==0 || freq_temp > filter.max || freq_temp < filter.min){
                        SID_frame.gate[0]=0;
                    }else{
                        SID_frame.gate[0] = *ptr & 0x01;
                    }
                    SID_frame.wave[0] = *ptr & 0x80;
                    if(*ptr & 0x08){
                        SID_frame.test[0]=1;
                    }else{
                        SID_frame.test[0]=0;
                    }
        			break;
                case SID_AD1:
                    SID_frame.attack[0] = (*ptr >> 4) & 0x0F;
                    SID_frame.decay[0] = (*ptr) & 0x0F;
                    break;
                case SID_SR1:
                    SID_frame.sustain[0] = ((*ptr & 0xF0) >> 1);
                    SID_frame.release[0] = (*ptr) & 0x0F;
                    break;
                case SID_FREQLO2:
                    SID_frame.freq_fp8[1] = *ptr;
                    break;
                case SID_FREQHI2:
                    SID_frame.freq_fp8[1] |= ((uint16_t)*ptr << 8);
                    SID_frame.freq_fp8[1] = (SID_frame.freq_fp8[1]<<15)/2179;
                    freq_temp = SID_frame.freq_fp8[1]>>8;
                    break;
                case SID_PWLO2:
		            SID_frame.pw[1] = (SID_frame.pw[1] & 0xff00) + *ptr;
		            break;      
                case SID_PWHI2:
		            SID_frame.pw[1] = (SID_frame.pw[1] & 0xff) + ((uint16_t)*ptr << 8);
		            break;
        		case SID_CR2:
                    if(filter.channel[1]==0 || freq_temp > filter.max || freq_temp < filter.min){
                        SID_frame.gate[1]=0;
                    }else{
                        SID_frame.gate[1] = *ptr & 0x01;
                    }
                    SID_frame.wave[1] = *ptr & 0x80;
                    if(*ptr & 0x08){
                        SID_frame.test[1]=1;
                    }else{
                        SID_frame.test[1]=0;
                    }
        			break;
                case SID_AD2:
                    SID_frame.attack[1] = (*ptr >> 4) & 0x0F;
                    SID_frame.decay[1] = (*ptr) & 0x0F;
                    break;
                case SID_SR2:
                    SID_frame.sustain[1] = ((*ptr & 0xF0) >> 1);
                    SID_frame.release[1] = (*ptr) & 0x0F;
                    break;
                case SID_FREQLO3:
                    SID_frame.freq_fp8[2] = *ptr;
                    break;
                case SID_FREQHI3:
                    SID_frame.freq_fp8[2] |= ((uint16_t)*ptr << 8);
                    SID_frame.freq_fp8[2] = (SID_frame.freq_fp8[1]<<15)/2179;
                    freq_temp = SID_frame.freq_fp8[2]>>8;
                    break;
                case SID_PWLO3:
		            SID_frame.pw[2] = (SID_frame.pw[2] & 0xff00) + *ptr;
		            break;      
                case SID_PWHI3:
		            SID_frame.pw[2] = (SID_frame.pw[2] & 0xff) + ((uint16_t)*ptr << 8);
		            break;
        		case SID_CR3:
                    if(filter.channel[2]==0 || freq_temp > filter.max || freq_temp < filter.min){
                        SID_frame.gate[2]=0;
                    }else{
                        SID_frame.gate[2] = *ptr & 0x01;
                    }
                    SID_frame.wave[2] = *ptr & 0x80;
                    if(*ptr & 0x08){
                        SID_frame.test[2]=1;
                    }else{
                        SID_frame.test[2]=0;
                    }
        			break;
                case SID_AD3:
                    SID_frame.attack[2] = (*ptr >> 4) & 0x0F;
                    SID_frame.decay[2] = (*ptr) & 0x0F;
                    break;
                case SID_SR3:
                    SID_frame.sustain[2] = ((*ptr & 0xF0) >> 1);
                    SID_frame.release[2] = (*ptr) & 0x0F;
                    break;
                case SID_UD_TIME0:
                    SID_frame.next_frame = *ptr<<24;
                    break;
                case SID_UD_TIME1:
                    SID_frame.next_frame |= *ptr<<16;
                    break;
                case SID_UD_TIME2:
                    SID_frame.next_frame |= *ptr<<8;
                    break;
                case SID_UD_TIME3:
                    SID_frame.next_frame |= *ptr;
                    break;
                    
            }
            SID_register++;
            if(SID_register==SID_END+1){
                uint16_t dutycycle=0;
                for(uint8_t i=0;i<3;i++){
                    if (SID_frame.gate[i]){
                        if(SID_frame.wave[i]){
                            dutycycle+= (((uint32)127*(uint32)param.pw)/(127000ul/freq_temp)); //Noise
                        }else{
                            dutycycle+= ((uint32)127*(uint32)param.pw)/(127000ul/freq_temp); //Normal wave
                        }
                    }
                }
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
                SID_register=0;
                start_frame=0;
            }
        }
        
        if(*ptr==0xFF){
            SID_frame_marker++;
        }else{
            SID_frame_marker=0;
        }
        if(SID_frame_marker==4){
            SID_register=0;
            start_frame=1;
        }
        len--;
        ptr++;   
    }  
}

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
            SID_frame.wave[i] = *ptr & 0x80;
            if(filter.channel[i]==0 || freq_temp > filter.max || freq_temp < filter.min){
                SID_frame.gate[i]=0;
            }else{
                SID_frame.gate[i] = *ptr & 0x01;
            }
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
