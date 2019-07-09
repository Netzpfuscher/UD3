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
#include "tasks/tsk_fault.h"

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


void process_sid(uint8_t* ptr, uint16_t len) {
    static uint8_t SID_frame_marker=0;
    static uint8_t SID_register=0;
    static struct sid_f SID_frame;
    static uint8_t start_frame=0;
    if(len){
        uint32_t temp;
	    while(len){
            
            if(start_frame){
                switch(SID_register){
                    case SID_FREQLO1:
                        SID_frame.freq[0]=(SID_frame.freq[0] & 0xff00) + *ptr;
                        break;
                    case SID_FREQHI1:
                        SID_frame.freq[0] = (SID_frame.freq[0] & 0xff) + ((uint16_t)*ptr << 8);
                        temp = ((uint32_t)SID_frame.freq[0]<<7)/2179;
                        SID_frame.freq[0]=temp;
                        SID_frame.half[0]= (150000<<14) / (temp<<14);
                        break;
                    case SID_PWLO1:
    		            SID_frame.pw[0] = (SID_frame.pw[0] & 0xff00) + *ptr;
    		            break;      
                    case SID_PWHI1:
    		            SID_frame.pw[0] = (SID_frame.pw[0] & 0xff) + ((uint16_t)*ptr << 8);
    		            break;
            		case SID_CR1:
                        SID_frame.gate[0] = *ptr & 0x01;
                        SID_frame.wave[0] = *ptr & 0x80;
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
                        SID_frame.freq[1]=(SID_frame.freq[1] & 0xff00) + *ptr;
                        break;
                    case SID_FREQHI2:
                        SID_frame.freq[1] = (SID_frame.freq[1] & 0xff) + ((uint16_t)*ptr << 8);
                        temp = ((uint32_t)SID_frame.freq[1]<<7)/2179;
                        SID_frame.freq[1]=temp;
                        SID_frame.half[1]= (150000<<14) / (temp<<14);
                        break;
                    case SID_PWLO2:
    		            SID_frame.pw[1] = (SID_frame.pw[1] & 0xff00) + *ptr;
    		            break;      
                    case SID_PWHI2:
    		            SID_frame.pw[1] = (SID_frame.pw[1] & 0xff) + ((uint16_t)*ptr << 8);
    		            break;
            		case SID_CR2:
                        SID_frame.gate[1] = *ptr & 0x01;
                        SID_frame.wave[1] = *ptr & 0x80;
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
                        SID_frame.freq[2]=(SID_frame.freq[2] & 0xff00) + *ptr;
                        break;
                    case SID_FREQHI3:
                        SID_frame.freq[2] = (SID_frame.freq[2] & 0xff) + ((uint16_t)*ptr << 8);
                        temp = ((uint32_t)SID_frame.freq[2]<<7)/2179;
                        SID_frame.freq[2]=temp;
                        SID_frame.half[2]= (150000<<14) / (temp<<14);
                        break;
                    case SID_PWLO3:
    		            SID_frame.pw[2] = (SID_frame.pw[2] & 0xff00) + *ptr;
    		            break;      
                    case SID_PWHI3:
    		            SID_frame.pw[2] = (SID_frame.pw[2] & 0xff) + ((uint16_t)*ptr << 8);
    		            break;
            		case SID_CR3:
                        SID_frame.gate[2] = *ptr & 0x01;
                        SID_frame.wave[2] = *ptr & 0x80;
            			break;
                    case SID_AD3:
                        SID_frame.attack[2] = (*ptr >> 4) & 0x0F;
                        SID_frame.decay[2] = (*ptr) & 0x0F;
                        break;
                    case SID_SR3:
                        SID_frame.sustain[2] = ((*ptr & 0xF0) >> 1);
                        SID_frame.release[2] = (*ptr) & 0x0F;
                        break;
                }
                SID_register++;
                if(SID_register==25){
                    uint16_t dutycycle=0;
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
                SID_register=0;
                start_frame=1;
            }
            len--;
            ptr++;   
        }
    }
}


