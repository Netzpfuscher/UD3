/*
 * VMS - Versatile Modulation System
 *
 * Copyright (c) 2020 Thorben Zethoff
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

#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#if PIC32
#include <xc.h>
#include <sys/attribs.h>
#endif
#include <string.h>
#include "FreeRTOS.h"
//#include <GenericTypeDefs.h>

//#include "usb lib/usb.h"
#include "MidiController.h"
//#include "ConfigManager.h"
//#include "UART32.h"
#include "ADSREngine.h"
#include "SignalGenerator.h"
#include "VMSRoutines.h"
#include "helper/vms_types.h"
#include "mapper.h"
#include "cli_basic.h"
#include "cli_common.h"
//#include "HIDController.h"

//Note frequency lookup table
const uint16_t Midi_NoteFreq[128] = {8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 19, 21, 22, 23, 24, 26, 28, 29, 31, 33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62, 65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123, 131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988, 1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976, 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951, 4186, 4435, 4699, 4978, 5274, 5588, 5920, 5920, 6645, 7040, 7459, 7902, 8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544};

ChannelInfo channelData[16] = {[0 ... 15].bendFactor = 200000, [0 ... 15].bendRange = 2.0};

//voice parameters and variables
SynthVoice Midi_voice[MIDI_VOICECOUNT] = {[0 ... 3].currNote = 0xff, [0].id = 0, [1].id = 1, [2].id = 2, [3].id = 3};

//Programm and coil configuration
//CoilConfig * Midi_currCoil;
uint8_t Midi_currCoilIndex = 0xff;          //this is used by the pc software to figure out which configuration is active
//uint16_t Midi_minOnTimeVal = 0;
//uint16_t Midi_maxOnTimeVal = 0;



//Global midi settings

//LED Time variables
unsigned Midi_blinkLED = 0;
uint16_t Midi_blinkScaler = 0;
uint16_t Midi_currComsLEDTime = 0;

unsigned Midi_enabled = 1;
uint32_t FWUpdate_currPLOffset = 0;
uint8_t * payload = 0;
uint16_t currPage = 0;

uint16_t Midi_currRPN = 0xffff;
unsigned Midi_initialized = 0;

void Midi_init(){
    if(Midi_initialized) return;
    Midi_initialized = 1;
    //allocate ram for programm and coild configurations
    //Midi_currProgramm = malloc(sizeof(MidiProgramm));
    //Midi_currCoil = malloc(sizeof(CoilConfig));
    
    VMS_init();
    
    uint8_t currChannel = 0;
    for(;currChannel < 16; currChannel++){ 
        channelData[currChannel].currVolume = 127;
        channelData[currChannel].currStereoVolume = 0xff;
        channelData[currChannel].currStereoPosition = 64;
        channelData[currChannel].currVolumeModifier = 32385;
        MAPPER_programChangeHandler(currChannel, 0);
    }
       
    //random s(1)ee(7), used for the noise source
    srand(1337);
    
    //get note timers ready
    SigGen_init();
    
}

//As long as the USB device is not suspended this function will be called every 1 ms
void Midi_SOFHandler(){
    if(!Midi_initialized) return;
    
    uint8_t currVoice = 0;
    for(;currVoice < MIDI_VOICECOUNT; currVoice++){
        Midi_voice[currVoice].noteAge ++;
    }
    
}

void Midi_run(uint8_t* ReceivedDataBuffer){
    if(!Midi_initialized) return;
    
    /* If the device is not configured yet, or the device is suspended, then
     * we don't need to run the demo since we can't send any data.
     */
    
    //a new Midi data packet was received
        if(Midi_enabled){

            uint8_t channel = ReceivedDataBuffer[1] & 0xf;
            uint8_t cmd = ReceivedDataBuffer[1] & 0xf0;

            if(cmd == MIDI_CMD_NOTE_OFF){
                
                //step through all voices and check if the note that was just switched off is playing on any, if it is and the channel matches, we switch it off
                uint8_t currVoice = 0;
                for(;currVoice < MIDI_VOICECOUNT; currVoice++){
                    if(Midi_voice[currVoice].currNote == ReceivedDataBuffer[2] && Midi_voice[currVoice].currNoteOrigin == channel){
                        Midi_NoteOff(currVoice, channel);
                    }
                }
                
            }else if(cmd == MIDI_CMD_NOTE_ON){
                if(ReceivedDataBuffer[3] > 0){  //velocity is > 0 -> turn note on && channelData[channel].currStereoVolume > 0
                    MAPPER_map(ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                    
                }else{  //velocity is == 0 -> turn note off (some software uses this instead of the note off command)
                    uint8_t currVoice = 0;
                    for(;currVoice < MIDI_VOICECOUNT; currVoice++){
                        if(Midi_voice[currVoice].currNote == ReceivedDataBuffer[2] && Midi_voice[currVoice].currNoteOrigin == channel){
                            Midi_NoteOff(currVoice, channel);
                        }
                    }
                }

            }else if(cmd == MIDI_CMD_CONTROLLER_CHANGE){
                if(ReceivedDataBuffer[2] == MIDI_CC_ALL_SOUND_OFF || ReceivedDataBuffer[2] == MIDI_CC_ALL_NOTES_OFF || ReceivedDataBuffer[2] == MIDI_CC_RESET_ALL_CONTROLLERS){ //Midi panic, and sometimes used by programms when you press the "stop" button

                    VMS_clear();
                    
                    //step through all voices and switch them off
                    uint8_t currVoice = 0;
                    for(;currVoice < MIDI_VOICECOUNT; currVoice++){
                        //Midi_voice[currVoice].currNote = 0xff;
                        Midi_voice[currVoice].otTarget = 0;
                        Midi_voice[currVoice].otCurrent = 0;
                        Midi_voice[currVoice].on = 0;
                    }
                    
                    //stop the timers
                    SigGen_kill();
                    
                    //reset all bend factors
                    uint8_t currChannel = 0;
                    for(;currChannel < 16; currChannel++){
                        channelData[currChannel].bendFactor = 200000;
                        channelData[currChannel].bendRange = 2.0;
                        channelData[currChannel].currVolume = 127;
                        channelData[currChannel].currStereoPosition = 64;
                        Midi_calculateStereoVolume(currChannel);
                        channelData[currChannel].currVolumeModifier = 32385;
                        channelData[currChannel].sustainPedal = 0;
                    }
                    
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_PTIME_COARSE){
                    channelData[channel].portamentoTime &= 0xff;
                    channelData[channel].portamentoTime |= (ReceivedDataBuffer[3] << 7);
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_VOLUME){
                    channelData[channel].currVolume = ReceivedDataBuffer[3];
                    MAPPER_volumeHandler(channel);
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_PAN){             //we are getting the address for the next RPN, remember it until the data comes in 
                    channelData[channel].currStereoPosition = ReceivedDataBuffer[3];
                    Midi_calculateStereoVolume(channel);
                    MAPPER_volumeHandler(channel);
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_RPN_LSB){             //we are getting the address for the next RPN, remember it until the data comes in
                    Midi_currRPN &= 0xff00;
                    Midi_currRPN |= ReceivedDataBuffer[3];
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_RPN_MSB){
                    Midi_currRPN &= 0x00ff;
                    Midi_currRPN |= ReceivedDataBuffer[3] << 8;
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_DATA_FINE){           //RPN data -> check the address we received before, to select which parameter to change
                    if(Midi_currRPN == MIDI_RPN_BENDRANGE){     //get the 100ths semitones for the bendrange
                        channelData[channel].bendLSB = ReceivedDataBuffer[3];
                        //channelData[channel].currProgramm->bendRange = (float) channelData[channel].bendMSB + (float) channelData[channel].bendLSB / 100.0;
                    }
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_DATA_COARSE){         //RPN data -> check the address we received before, to select which parameter to change
                    if(Midi_currRPN == MIDI_RPN_BENDRANGE){     //get the full semitones for the bendrange
                        channelData[channel].bendMSB = ReceivedDataBuffer[3];
                        //channelData[channel].currProgramm->bendRange = (float) channelData[channel].bendMSB + (float) channelData[channel].bendLSB / 100.0;
                    }
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_NRPN_LSB || ReceivedDataBuffer[2] == MIDI_CC_NRPN_MSB){   //we don't currently support any NRPNs, but we need to make sure, that we don't confuse the data sent for them as being a RPN, so reset the current RPN address
                    Midi_currRPN = 0xffff;
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_SUSTAIN_PEDAL){   //enable/disable the sustain pedal
                    channelData[channel].sustainPedal = ReceivedDataBuffer[3] > 63;
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_DAMPER_PEDAL){    //enable/disable the damper pedal
                    channelData[channel].damperPedal = ReceivedDataBuffer[3] > 63;
                }else if(ReceivedDataBuffer[2] >= 102 && ReceivedDataBuffer[2] <= 119){
                    channelData[channel].parameters[ReceivedDataBuffer[2] - 102] = ReceivedDataBuffer[3];
                    //UART_print("Assigned %d to CC %d\r\n", ReceivedDataBuffer[3], ReceivedDataBuffer[2]);
                }


            }else if(cmd == MIDI_CMD_PITCH_BEND){
                //calculate the factor, by which we will bend the note
                int16_t bendParameter = (((ReceivedDataBuffer[3] << 7) | ReceivedDataBuffer[2]) - 8192);
                float bendOffset = (float) bendParameter / 8192.0;
                channelData[channel].bendFactor = (uint32_t) (powf(2, (bendOffset * (float) channelData[channel].bendRange) / 12.0) * 200000.0);
                  
                MAPPER_bendHandler(channel);
                
            }else if(cmd == MIDI_CMD_PROGRAM_CHANGE){
                //load the program from NVM
                MAPPER_programChangeHandler(channel, ReceivedDataBuffer[2]);
            }
        }
}

void Midi_setEnabled(unsigned ena){
    Midi_enabled = ena;
    if(!ena){
        VMS_clear();

        //step through all voices and switch them off
        uint8_t currVoice = 0;
        for(;currVoice < MIDI_VOICECOUNT; currVoice++){
            //Midi_voice[currVoice].currNote = 0xff;
            Midi_voice[currVoice].otTarget = 0;
            Midi_voice[currVoice].on = 0;
        }

        //stop the timers
        SigGen_kill();
                   
    }
}

unsigned Midi_isNoteOff(uint8_t voice){
    return (Midi_voice[voice].otCurrent < 10) && !Midi_voice[voice].on;
}

unsigned Midi_isNoteDecaying(uint8_t voice){
    return !Midi_voice[voice].on;
}

void Midi_NoteOff(uint8_t voice, uint8_t channel){
    //Midi_voice[voice].currNote = 0xff;
    //UART_print("note off on voice %d\r\n", voice);
    Midi_voice[voice].on = 0;
}

unsigned Midi_areAllNotesOff(){
    if(Midi_voice[0].otCurrent > 0 && SigGen_get_channel(0)) return 0;
    if(Midi_voice[1].otCurrent > 0 && SigGen_get_channel(1)) return 0;
    if(Midi_voice[2].otCurrent > 0 && SigGen_get_channel(2)) return 0;
    if(Midi_voice[3].otCurrent > 0 && SigGen_get_channel(3)) return 0;
    return 1;
}

#define STEREO_POS 127;
#define STEREO_WIDTH 127;
#define STEREO_SLOPE 16;

//stereo volume mapping
void Midi_calculateStereoVolume(uint8_t channel){
    uint8_t stereoPosition = STEREO_POS;
    uint8_t stereoWidth = STEREO_WIDTH;
    uint8_t stereoSlope = STEREO_SLOPE;
    
    int16_t sp = channelData[channel].currStereoPosition;
    int16_t rsStart = (stereoPosition - stereoWidth - 255/stereoSlope);
    int16_t fsEnd = (stereoPosition + stereoWidth + 255/stereoSlope);
    
    
    if(sp < rsStart){                                                   //span before volume > 0
        channelData[channel].currStereoVolume = 0;
    }else if(sp > rsStart && sp < (stereoPosition - stereoWidth)){      //rising slope
        channelData[channel].currStereoVolume = (sp - rsStart) * stereoSlope;
    }else if(sp < (stereoPosition + stereoWidth) && sp > (stereoPosition - stereoWidth)){   //span of 0xff
        channelData[channel].currStereoVolume = 0xff;
    }else if(sp < fsEnd && sp > (stereoPosition - stereoWidth)){        //falling slope
        channelData[channel].currStereoVolume = 0xff - (sp - stereoPosition - stereoWidth) * stereoSlope;
    }else if(sp > fsEnd){                                               //span after the volume curve
        channelData[channel].currStereoVolume = 0;
    }
    
    //UART_sendString("Calculated stereo volume for CH", 0); UART_sendInt(channel, 0); UART_sendString(" with position ", 0); UART_sendInt(sp, 0); UART_sendString(" = ", 0); UART_sendInt(channelData[channel].currStereoVolume, 1);
}

void Midi_setNoteOnTime(uint16_t onTimeUs, uint8_t ch){
    channelData[ch].currOT = onTimeUs;
}

ChannelInfo * Midi_getChannelData(uint8_t channel){
    return &channelData[channel];
}
