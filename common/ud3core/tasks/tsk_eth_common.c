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
#include "tasks/tsk_sid.h"
#include "tasks/tsk_fault.h"
#include "clock.h"
#include "SidFilter.h"

typedef struct{
    uint8_t cmd;
    int8_t message_size;
    uint8_t bytes_received;
    uint8_t msg[3];
} midi_cmd;

#define MIDI_NOTE_OFF           0x80
#define MIDI_NOTE_ON            0x90
#define MIDI_KEY_PRESSURE       0xA0
#define MIDI_CC                 0xB0
#define MIDI_PC                 0xC0
#define MIDI_CP                 0xD0
#define MIDI_PB                 0xE0
#define MIDI_FUNCTION           0xF0

#define MIDI_SYSEX_START        0xF0
#define MIDI_SONG_POSITION      0xF2
#define MIDI_SONG_SELECT        0xF3
#define MIDI_BUS_SELECT         0xF5
#define MIDI_TUNE_REQUEST       0xF6
#define MIDI_SYSEX_END          0xF7
#define MIDI_START_SONG         0xFA
#define MIDI_CONTINUE_SONG      0xFB
#define MIDI_STOP_SONG          0xFC
#define MIDI_ACTIVE_SENSING     0xFE
#define MIDI_SYSTEM_RESET       0xFF

#define MAX_SYSEX_SIZE          32

#define DATA_NOT_READY          -1


void process_midi(uint8_t* ptr, uint16_t len) {
    static midi_cmd state;
 
	while (len) {
		uint8_t c = *ptr;
        uint8_t c_masked = c & 0xF0;
        len--;
        ptr++;

		if (c & 0x80) {  //MSB set  Reset
            state.bytes_received = 0;
            switch(c_masked){
                case MIDI_PC:
                    state.cmd = c;
                    state.message_size = 1;
                break;
                case MIDI_CP:
                    state.cmd = c;
                    state.message_size = 1;
                break;
                case MIDI_FUNCTION:
                
                    switch(c){
                        case MIDI_SYSEX_START:
                            state.cmd = c;
                            state.bytes_received = 0;
                            state.message_size = MAX_SYSEX_SIZE;
                        break;
                        case MIDI_SYSEX_END:
                            state.message_size = state.bytes_received;
                        break;
                        case MIDI_SONG_POSITION:
                            state.cmd = c;
                            state.message_size = 2;
                        break;
                        case MIDI_SONG_SELECT:
                            state.cmd = c;
                            state.message_size = 1;
                        break;
                        case MIDI_BUS_SELECT:
                            state.cmd = c;
                            state.message_size = 1;
                        break;
                        default:
                            state.cmd = c;
                            state.message_size = 0;
                        break;
                    }
                break;
                default:
                    state.cmd = c;
                    state.message_size = 2;
                break;
            }
        }else{
                if(state.bytes_received < state.message_size && state.cmd != MIDI_SYSEX_START){
                    state.msg[state.bytes_received + 1] = c;
                    state.bytes_received++;
                } else if(state.bytes_received < state.message_size && state.cmd == MIDI_SYSEX_START){
                    state.bytes_received++;
                }
 
                if(state.message_size != DATA_NOT_READY && state.bytes_received == state.message_size){   
                    state.msg[0] = state.cmd;
                    USBMIDI_1_callbackLocalMidiEvent(0, state.msg);
                    state.bytes_received = 0;
                    if((state.cmd & 0xF0) == MIDI_FUNCTION){
                        state.message_size = DATA_NOT_READY;
                    }
                }
        }
    }
}


#define SID_BYTES       29

#define SID_COMMAND_VOLUME       0
#define SID_COMMAND_NOISE_VOLUME 1
#define SID_COMMAND_HPV_ENABLE   2

void process_min_sid(uint8_t* ptr, uint16_t len) {
    uint8_t n_frames = *ptr;
    len--;
    ptr++;
    if (n_frames == 0 && len == 4) {
        uint8_t command = ptr[0];
        uint8_t channel = ptr[1];
        uint16_t value = (ptr[2] << 8) | ptr[3];
        switch (command) {
            case SID_COMMAND_VOLUME:
                SID_filterData.channelVolume[channel] = value;
                return;
            case SID_COMMAND_NOISE_VOLUME:
                SID_filterData.noiseVolume = value;
                return;
            case SID_COMMAND_HPV_ENABLE:
                SID_filterData.hpvEnabledGlobally = value != 0;
                return;
        }
    }
    
    if(len!=(SID_BYTES*n_frames)){
        alarm_push(ALM_PRIO_WARN,"SID: Malformed SID frame", ALM_NO_VALUE);
        return;
    }
    
    SIDFrame_t SID_frame;
    
    //TODO add filter stuff?
    
    //uint16_t master_pw; //TODO: whats this used for?

    while(n_frames){
        uint32_t freq_tempFp8;
        for(uint32_t i = 0;i<N_SIDCHANNEL;i++){
            //SID_FREQLO1
            freq_tempFp8 = *ptr;
            
            ptr++;
            
            //SID_FREQHI1
            freq_tempFp8 |= ((uint16_t)*ptr << 8);
            freq_tempFp8 = (freq_tempFp8<<15)/2179;
            
            SID_frame.frequency_dHz[i] = (freq_tempFp8 * 10) >> 8;
            
            ptr++;
            
            //SID_PWLO1
            SID_frame.pulsewidth[i] = (*ptr & 0xff);
            
            ptr++;
            
            //SID_PWHI1
            
            SID_frame.pulsewidth[i] = (SID_frame.pulsewidth[i] & 0xff) + (((uint16_t)*ptr << 8) & 0xff00);
            
            ptr++;
            
            //SID_CR1
            
            //start of with a clear flag set
            SID_frame.flags[i] = 0;
            
            //is noise bit set?
            if(*ptr & 0x80) SID_frame.flags[i] |= SID_FRAME_FLAG_NOISE;
            
            //is SQR bit set?
            if(*ptr & 0x40) SID_frame.flags[i] |= SID_FRAME_FLAG_SQUARE;
            
            //is SAW bit set?
            if(*ptr & 0x20) SID_frame.flags[i] |= SID_FRAME_FLAG_SAWTOOTH;
            
            //is TRI bit set?
            if(*ptr & 0x10) SID_frame.flags[i] |= SID_FRAME_FLAG_TRIANGLE;
            
            //is TEST bit set?
            if(*ptr & 0x08) SID_frame.flags[i] |= SID_FRAME_FLAG_TEST;
            
            //is RING bit set?
            if(*ptr & 0x04) SID_frame.flags[i] |= SID_FRAME_FLAG_RING;
            
            //is SYNC bit set?
            if(*ptr & 0x02) SID_frame.flags[i] |= SID_FRAME_FLAG_SYNC;
            
            //is GATE bit set?
            if(*ptr & 0x01) SID_frame.flags[i] |= SID_FRAME_FLAG_GATE;
            
            ptr++;
            
            //SID_AD1
            SID_frame.attack[i] = (*ptr >> 4) & 0x0F;
            SID_frame.decay[i] = (*ptr) & 0x0F;
            
            ptr++;
            
            //SID_SR1
            SID_frame.sustain[i] = ((*ptr & 0xF0) >> 1);
            SID_frame.release[i] = (*ptr) & 0x0F;
            
            ptr++;
        }
        //SID_FCLO
        
        ptr++;
        
        //SID_FCHI
        
        ptr++;
        
        //SID_Res_Filt
        
        ptr++;
        
        //SID_Mode_Vol
        if(*ptr & 0x80) SID_frame.flags[2] |= SID_FRAME_FLAG_CH3OFF;
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
        
        int32_t diff = SID_frame.next_frame - l_time;
        if(diff<-1000){
            alarm_push(ALM_PRIO_WARN,"SID: Old SID frame received", diff);
        }else{   
            xQueueSend(qSID,&SID_frame,0);
        }
        
    }
   
}
