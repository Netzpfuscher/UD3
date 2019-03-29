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
#include "cli_common.h"
#include "tasks/tsk_midi.h"

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
			break;
		case 2:
			midiMsg[2] = c;
			midi_count = 0;
			if (midiMsg[0] == 0xF0) {
				if (midiMsg[1] == 0x0F) {
					watchdog_reset_Control = 1;
					watchdog_reset_Control = 0;
					goto end;
				}
			}
			USBMIDI_1_callbackLocalMidiEvent(0, midiMsg);
			break;
		}
	end:;
	}
}

void process_sid(uint8_t* ptr, uint16_t len) {
    static uint8_t SID_frame_marker=0;
    static uint8_t SID_register=0;
    static uint16_t dutycycle=0;
    static struct sid_f SID_frame;
    static uint8_t start_frame=0;
    if(len){
        uint32_t temp;
	    while(len){
            
            if(start_frame){
                switch(SID_register){
                    case 0:
                        SID_frame.freq[0]=(SID_frame.freq[0] & 0xff00) + *ptr;
                        break;
                    case 1:
                        SID_frame.freq[0] = (SID_frame.freq[0] & 0xff) + ((uint16_t)*ptr << 8);
                        temp = ((uint32_t)SID_frame.freq[0]<<7)/2179;
                        SID_frame.freq[0]=temp;
                        SID_frame.half[0]= (150000<<14) / (temp<<14);
                        break;
                    case 2:
    		            SID_frame.pw[0] = (SID_frame.pw[0] & 0xff00) + *ptr;
    		            break;      
                    case 3:
    		            SID_frame.pw[0] = (SID_frame.pw[0] & 0xff) + ((uint16_t)*ptr << 8);
    		            break;
            		case 4:
                        SID_frame.gate[0] = *ptr & 0x01;
                        SID_frame.wave[0] = *ptr & 0x80;
            			break;
                    case 7:
                        SID_frame.freq[1]=(SID_frame.freq[1] & 0xff00) + *ptr;
                        break;
                    case 8:
                        SID_frame.freq[1] = (SID_frame.freq[1] & 0xff) + ((uint16_t)*ptr << 8);
                        temp = ((uint32_t)SID_frame.freq[1]<<7)/2179;
                        SID_frame.freq[1]=temp;
                        SID_frame.half[1]= (150000<<14) / (temp<<14);
                        break;
                    case 9:
    		            SID_frame.pw[1] = (SID_frame.pw[1] & 0xff00) + *ptr;
    		            break;      
                    case 10:
    		            SID_frame.pw[1] = (SID_frame.pw[1] & 0xff) + ((uint16_t)*ptr << 8);
    		            break;
            		case 11:
                        SID_frame.gate[1] = *ptr & 0x01;
                        SID_frame.wave[1] = *ptr & 0x80;
            			break;
                    case 14:
                        SID_frame.freq[2]=(SID_frame.freq[2] & 0xff00) + *ptr;
                        break;
                    case 15:
                        SID_frame.freq[2] = (SID_frame.freq[2] & 0xff) + ((uint16_t)*ptr << 8);
                        temp = ((uint32_t)SID_frame.freq[2]<<7)/2179;
                        SID_frame.freq[2]=temp;
                        SID_frame.half[2]= (150000<<14) / (temp<<14);
                        break;
                    case 16:
    		            SID_frame.pw[2] = (SID_frame.pw[2] & 0xff00) + *ptr;
    		            break;      
                    case 17:
    		            SID_frame.pw[2] = (SID_frame.pw[2] & 0xff) + ((uint16_t)*ptr << 8);
    		            break;
            		case 18:
                        SID_frame.gate[2] = *ptr & 0x01;
                        SID_frame.wave[2] = *ptr & 0x80;
            			break;
                }
                SID_register++;
                if(SID_register==25){
                    for(uint8_t i=0;i<3;i++){
                        if (SID_frame.gate[i]){
                            if(SID_frame.wave[i]){
                                dutycycle+= ((uint32)127*(uint32)param.pw)/(127000ul/(uint32)4000); //Noise
                            }else{
                                dutycycle+= ((uint32)127*(uint32)param.pw)/(127000ul/(uint32)SID_frame.freq[i]); //Normal wave
                            }
                        }
                    }
                    telemetry.duty = dutycycle;
                    if(dutycycle>configuration.max_tr_duty - param.temp_duty){
                        SID_frame.master_pw = (param.pw * (configuration.max_tr_duty - param.temp_duty)) / dutycycle;
                    }else{
                        SID_frame.master_pw = param.pw;
                    }  
                    interrupter.pw = SID_frame.master_pw;
                    xQueueSend(qSID,&SID_frame,0);
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
                dutycycle=0;
                SID_register=0;
                start_frame=1;
            }
            
            len--;
            ptr++;
            
        }
        
    }
}


