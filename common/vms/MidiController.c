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
#include "NoteManager.h"
//#include "UART32.h"
#include "ADSREngine.h"
#include "SignalGenerator.h"
#include "VMSRoutines.h"
#include "helper/vms_types.h"
#include "mapper.h"
//#include "HIDController.h"

//Note frequency lookup table
const uint16_t Midi_NoteFreq[128] = {8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 19, 21, 22, 23, 24, 26, 28, 29, 31, 33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62, 65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123, 131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988, 1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976, 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951, 4186, 4435, 4699, 4978, 5274, 5588, 5920, 5920, 6645, 7040, 7459, 7902, 8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544};

ChannelInfo channelData[16] = {[0 ... 15].bendFactor = 200000, [0 ... 15].bendRange = 2.0};

//voice parameters and variables
SynthVoice Midi_voice[MIDI_VOICECOUNT] = {[0 ... 3].currNote = 0xff, [0].id = 0, [1].id = 1, [2].id = 2, [3].id = 3};

//Programm and coil configuration
CoilConfig * Midi_currCoil;
uint8_t Midi_currCoilIndex = 0xff;          //this is used by the pc software to figure out which configuration is active
//uint16_t Midi_minOnTimeVal = 0;
//uint16_t Midi_maxOnTimeVal = 0;


//USB comms stuff
static uint8_t ReceivedDataBuffer[64];          //data received from the midi endpoint
static uint8_t ConfigReceivedDataBuffer[64];    //data received from the config endpoint
static uint8_t ToSendDataBuffer[64];            //data to send to the config endpoint
static USB_HANDLE Midi_dataHandle;              //Micochip usb library handles
static USB_HANDLE Midi_configTxHandle;
static USB_HANDLE Midi_configRxHandle;

//Global midi settings

//variables used for the limitation of signal parameters
unsigned noteHoldoff = 0;
uint16_t halfLimit = 0;
uint16_t otLimit = 0;
int32_t accumulatedOnTime = 0;
uint16_t resetCounter = 0;
uint16_t dutyLimiter = 0;
uint32_t noteOffTime = 1;
uint16_t holdOff = 0;

//LED Time variables
unsigned Midi_blinkLED = 0;
uint16_t Midi_blinkScaler = 0;
uint16_t Midi_currComsLEDTime = 0;

unsigned Midi_enabled = 1;
unsigned progMode = 0;
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
    Midi_currCoil = pvPortMalloc(sizeof(CoilConfig));
    
    VMS_init();
    
    uint8_t currChannel = 0;
    for(;currChannel < 16; currChannel++){ 
        channelData[currChannel].currVolume = 127;
        channelData[currChannel].currStereoVolume = 0xff;
        channelData[currChannel].currStereoPosition = 64;
        channelData[currChannel].currVolumeModifier = 32385;
        MAPPER_programChangeHandler(currChannel, 0);
    }
    NVM_readCoilConfig(Midi_currCoil, 0xff);
    
    //calculate required coefficients
    //Midi_setNoteOnTime(Midi_currCoil->minOnTime + ((Midi_currCoil->maxOnTime * Midi_currVolume) / 127), 0);
    holdOff = (Midi_currCoil->holdoffTime * 100) / 133;
    halfLimit = (Midi_currCoil->ontimeLimit * 100) / 266;
    otLimit = (Midi_currCoil->ontimeLimit * 100) / 133;
    
    //random s(1)ee(7), used for the noise source
    srand(1337);
    
    //get note timers ready
    SigGen_init();
    
    //Do usb stuff (code mostly from the Midi device example code)
    Midi_dataHandle = NULL;
    Midi_configRxHandle = NULL;
    Midi_configTxHandle = NULL;
    USBEnableEndpoint(USB_DEVICE_AUDIO_MIDI_ENDPOINT,USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
    USBEnableEndpoint(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
    Midi_dataHandle = USBRxOnePacket(USB_DEVICE_AUDIO_MIDI_ENDPOINT,(uint8_t*)&ReceivedDataBuffer,64);
    Midi_configRxHandle = USBRxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT,(uint8_t*)&ConfigReceivedDataBuffer,64);
    
    //enable the power LED
    Midi_LED(LED_POWER, LED_ON);
}

//As long as the USB device is not suspended this function will be called every 1 ms
void Midi_SOFHandler(){
    if(!Midi_initialized) return;
    
    unsigned anyNoteOn = 0;
    
    uint8_t currVoice = 0;
    for(;currVoice < MIDI_VOICECOUNT; currVoice++){
        anyNoteOn |= Midi_voice[currVoice].otFactor > 0;
        Midi_voice[currVoice].noteAge ++;
    }
    
    
    //Turn the note on LED on if any voices are active, and the E_STOP is closed (or disabled)
    Midi_LED(LED_OUT_ON, anyNoteOn & NVM_getConfig()->auxMode != AUX_E_STOP || (PORTB & _PORTB_RB5_MASK));
    
    //count down comms led on time, to make it turn off if no data was received for a while
    if(Midi_currComsLEDTime > 0){
        Midi_currComsLEDTime --;
        if(Midi_currComsLEDTime == 0){
            Midi_LED(LED_DATA, LED_OFF);
        }
    }
    
    if(Midi_blinkLED){
        if(++Midi_blinkScaler > 500){
            LATBINV = _LATB_LATB8_MASK;
            Midi_blinkScaler = 0;
        }
    }
}

void Midi_run(){
    if(!Midi_initialized) return;
    
    
    /* If the device is not configured yet, or the device is suspended, then
     * we don't need to run the demo since we can't send any data.
     */
    if( (USBGetDeviceState() < CONFIGURED_STATE) || USBIsDeviceSuspended() == true) {
        return;
    }
    
    //a new Midi data packet was received
    if(!USBHandleBusy(Midi_dataHandle)){
        if(!progMode && Midi_enabled){
            Midi_LED(LED_DATA, LED_BLINK);
            Midi_currComsLEDTime = 100;

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
                    //add note to the list with its velocity
                    
                    //UART_sendString("noteon: ", 0); UART_sendInt(ReceivedDataBuffer[2], 0); UART_sendString(" @ ", 0); UART_sendInt(channel, 1);
                    
                    //decide which voice should play the new note
                    uint8_t currVoice = 0;
                    unsigned skip = 0;
                    //if a voice is already playing the note, we can skip it. But we do need to reset its adsr
                    for(;currVoice < MIDI_VOICECOUNT; currVoice++){
                        if(Midi_voice[currVoice].currNote == ReceivedDataBuffer[2] && Midi_voice[currVoice].on && Midi_voice[currVoice].currNoteOrigin == channel){
                            MAPPER_map(currVoice, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                            //UART_sendString("\tskip", 1);
                            skip = 1;
                            break;
                        }
                    }
                    
                    //if we need to use a new voice, use the first off one we find. If there are none, we overwrite the oldest note
                    if(!skip){
                        if(Midi_isNoteOff(0)){
                            MAPPER_map(0, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                        }else if(Midi_isNoteOff(1)){ 
                            MAPPER_map(1, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                        }else if(Midi_isNoteOff(2)){          
                            MAPPER_map(2, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                        }else if(Midi_isNoteOff(3)){
                            MAPPER_map(3, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                            
                            
                        }else if(Midi_isNoteDecaying(0)){
                            MAPPER_map(0, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                        }else if(Midi_isNoteDecaying(1)){ 
                            MAPPER_map(1, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                        }else if(Midi_isNoteDecaying(2)){          
                            MAPPER_map(2, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                        }else if(Midi_isNoteDecaying(3)){
                            MAPPER_map(3, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                            
                            
                        }else{
                            
                            //UART_print("overwrite: ot[0]=%d, ot[1]=%d, ot[2]=%d, ot[3]=%d", Midi_voice[0].otCurrent, Midi_voice[1].otCurrent, Midi_voice[2].otCurrent, Midi_voice[3].otCurrent);
                            
                            uint32_t oldestAge = 0;
                            uint8_t i = 0;
                            for(; i < MIDI_VOICECOUNT; i++){
                                if(Midi_voice[i].noteAge > oldestAge) oldestAge = Midi_voice[i].noteAge;
                            }
                            
                            if(Midi_voice[0].noteAge == oldestAge){ 
                                MAPPER_map(0, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                                
                            }else if(Midi_voice[1].noteAge == oldestAge){ 
                                MAPPER_map(1, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                                
                            }else if(Midi_voice[2].noteAge == oldestAge){ 
                                MAPPER_map(2, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                                
                            }else if(Midi_voice[3].noteAge == oldestAge){ 
                                MAPPER_map(3, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                            }
                        }
                    }
                    

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
                    T2CONCLR = _T2CON_ON_MASK;
                    T3CONCLR = _T3CON_ON_MASK;
                    T4CONCLR = _T4CON_ON_MASK;
                    T5CONCLR = _T5CON_ON_MASK;
                    
                    //reset all bend factors
                    uint8_t currChannel = 0;
                    for(;currChannel < 16; currChannel++){
                        channelData[currChannel].bendFactor = 200000;
                        channelData[currChannel].bendRange = 2.0;
                        channelData[currChannel].currVolume = 127;
                        channelData[currChannel].currStereoVolume = 0xff;
                        channelData[currChannel].currStereoPosition = 64;
                        channelData[currChannel].currVolumeModifier = 32385;
                        channelData[currChannel].sustainPedal = 0;
                    }
                    
                    Midi_LED(LED_DUTY_LIMITER, LED_OFF);
                    
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

        //Get ready for next packet (this will overwrite the old data). thanks microchip, very cool
        Midi_dataHandle = USBRxOnePacket(USB_DEVICE_AUDIO_MIDI_ENDPOINT,(uint8_t*)&ReceivedDataBuffer,64);
    }
    
    if(!USBHandleBusy(Midi_configRxHandle))
    {
        if(!progMode){
            //New data is available at the configuration endpoint
            //TODO create structs for the data packets, so we don't have to do this ugly stuff
            //yeah this is still a TODO...
            
            if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_PROGRAMM){      //reads programm parameters
                //LEGACY!! only here to prevent old software from dying
                memset(ToSendDataBuffer, 0, 64);
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);

            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_COIL_CONFIG){   //read coil configurations
                memset(ToSendDataBuffer, 0, 64);
                CoilConfig requested;
                if(NVM_readCoilConfig(&requested, ConfigReceivedDataBuffer[1])){    //returns zero if config at index is invalid
                    ToSendDataBuffer[0] = 0x22;
                    ToSendDataBuffer[1] = ConfigReceivedDataBuffer[1];
                    ToSendDataBuffer[5] = (requested.maxOnTime >> 24);
                    ToSendDataBuffer[4] = (requested.maxOnTime >> 16);
                    ToSendDataBuffer[3] = (requested.maxOnTime >> 8);
                    ToSendDataBuffer[2] = requested.maxOnTime;
                    ToSendDataBuffer[9] = (requested.minOnTime >> 24);
                    ToSendDataBuffer[8] = (requested.minOnTime >> 16);
                    ToSendDataBuffer[7] = (requested.minOnTime >> 8);
                    ToSendDataBuffer[6] = (requested.minOnTime);
                    ToSendDataBuffer[10] = requested.maxDuty;
                    ToSendDataBuffer[12] = requested.ontimeLimit >> 8;
                    ToSendDataBuffer[11] = requested.ontimeLimit;
                    ToSendDataBuffer[14] = requested.holdoffTime >> 8;
                    ToSendDataBuffer[13] = requested.holdoffTime;
                    memcpy(&ToSendDataBuffer[32], requested.name, 24);
                }
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);

            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_SET_COIL_CONFIG){   //write coil configuration data
                Midi_setEnabled(0);
                CoilConfig * newData = pvPortMalloc(sizeof(CoilConfig));

                newData->maxOnTime = (ConfigReceivedDataBuffer[5] << 24) | (ConfigReceivedDataBuffer[4] << 16) | (ConfigReceivedDataBuffer[3] << 8) | ConfigReceivedDataBuffer[2];
                newData->minOnTime = (ConfigReceivedDataBuffer[9] << 24) | (ConfigReceivedDataBuffer[8] << 16) | (ConfigReceivedDataBuffer[7] << 8) | ConfigReceivedDataBuffer[6];
                newData->maxDuty = ConfigReceivedDataBuffer[10];
                newData->ontimeLimit = (ConfigReceivedDataBuffer[12] << 8) | ConfigReceivedDataBuffer[11];
                newData->holdoffTime = (ConfigReceivedDataBuffer[14] << 8) | ConfigReceivedDataBuffer[13];
                memcpy(newData->name, &ConfigReceivedDataBuffer[31], 22);
                NVM_writeCoilConfig(newData, (uint8_t) ConfigReceivedDataBuffer[1]);

                //if the config is the currently selected one we need to recalculate the current parameters and coefficients
                if(Midi_currCoilIndex == ConfigReceivedDataBuffer[1]){
                    Midi_currCoilIndex = ConfigReceivedDataBuffer[1];
                    NVM_readCoilConfig(Midi_currCoil, Midi_currCoilIndex);
                    halfLimit = (Midi_currCoil->ontimeLimit * 100) / 266;
                    otLimit = (Midi_currCoil->ontimeLimit * 100) / 133;
                    holdOff = (Midi_currCoil->holdoffTime * 100) / 133;
                    uint8_t currChannel = 0;
                    for(;currChannel < 16; currChannel++){                      //update the on time values, incase we have a different min/max from before
                        Midi_setNoteOnTime(Midi_currCoil->minOnTime + ((Midi_currCoil->maxOnTime * channelData[currChannel].currVolume * channelData[currChannel].currStereoVolume) / 127 / 0xff), currChannel);
                    }
                    UART_sendString("new CC: ot lim ", 0); UART_sendInt(Midi_currCoil->ontimeLimit, 0); UART_sendString(" OT Max ", 0); UART_sendInt(Midi_currCoil->maxOnTime, 0); UART_sendString(" duty Max ", 0); UART_sendInt(Midi_currCoil->maxDuty, 1);
                }
                vPortFree(newData);
                Midi_setEnabled(1);
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_DEVCFG){        //read device configuration parameters
                ToSendDataBuffer[0] = USB_CMD_GET_DEVCFG;
                memcpy(&ToSendDataBuffer[1], NVM_getConfig(), 28);  //28 = sizeof(CFGData), ignoring device specific stuff, the PC gets through other API calls
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);

            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_SET_DEVCFG){        //write device configuration parameters
                Midi_setEnabled(0);
                CFGData * newData = pvPortMalloc(sizeof(CFGData));
                memcpy(newData, &ConfigReceivedDataBuffer[1], sizeof(CFGData));
                NVM_writeCFG(newData);
                vPortFree(newData);
                LATBSET = _LATB_LATB7_MASK | _LATB_LATB8_MASK | _LATB_LATB9_MASK;
                Midi_LED(LED_POWER, LED_ON);    //turn on the power LED again, as it might be a different one from before
                Midi_setEnabled(1);

            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_SET_STERO_CONFIG){        //write device stereo config
                Midi_setEnabled(0);
                NVM_writeStereoParameters(ConfigReceivedDataBuffer[1], ConfigReceivedDataBuffer[2], ConfigReceivedDataBuffer[3]);
                
                uint8_t currChannel = 0;
                for(;currChannel < 16; currChannel ++){
                    Midi_calculateStereoVolume(currChannel);
                }
                Midi_setEnabled(1);

            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_STERO_CONFIG){        //write device stereo config
                ToSendDataBuffer[0] = USB_CMD_GET_STERO_CONFIG;
                ToSendDataBuffer[1] = NVM_getConfig()->stereoPosition;
                ToSendDataBuffer[2] = NVM_getConfig()->stereoWidth;
                ToSendDataBuffer[3] = NVM_getConfig()->stereoSlope;
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_COILCONFIG_INDEX){  //read which coil preset is in use
                ToSendDataBuffer[0] = USB_CMD_GET_COILCONFIG_INDEX;
                ToSendDataBuffer[1] = NVM_isCoilConfigValid(Midi_currCoilIndex) ? Midi_currCoilIndex : 0xff;
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);

            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_SET_COILCONFIG_INDEX){  //set which coil preset is in use
                if(Midi_areAllNotesOff){  //only allow for changing the coil if we are not playing any music
                    Midi_currCoilIndex = ConfigReceivedDataBuffer[1];
                    NVM_readCoilConfig(Midi_currCoil, Midi_currCoilIndex);
                    halfLimit = (Midi_currCoil->ontimeLimit * 100) / 266;
                    otLimit = (Midi_currCoil->ontimeLimit * 100) / 133;
                    holdOff = (Midi_currCoil->holdoffTime * 100) / 133;
                    uint8_t currChannel = 0;
                    for(;currChannel < 16; currChannel++){                      //update the on time values, incase we have a different min/max from before
                        Midi_setNoteOnTime(Midi_currCoil->minOnTime + ((Midi_currCoil->maxOnTime * channelData[currChannel].currVolume * channelData[currChannel].currStereoVolume) / 127 / 0xff), currChannel);
                    }
                    ToSendDataBuffer[0] = USB_CMD_SET_COILCONFIG_INDEX;
                    UART_sendString("new CC: ot lim ", 0); UART_sendInt(Midi_currCoil->ontimeLimit, 0); UART_sendString(" OT Max ", 0); UART_sendInt(Midi_currCoil->maxOnTime, 0); UART_sendString(" duty Max ", 0); UART_sendInt(Midi_currCoil->maxDuty, 1);
                    ToSendDataBuffer[1] = NVM_isCoilConfigValid(Midi_currCoilIndex) ? Midi_currCoilIndex : 0xff;
                }else{
                    ToSendDataBuffer[0] = USB_CMD_SET_COILCONFIG_INDEX;
                    ToSendDataBuffer[1] = USB_CMD_COILCONFIG_NOT_CHANGED;
                }
                
                //return either the new coil configuration index, or an error flag if we didn't change it, so the software can inform the user
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);

            }else if(ConfigReceivedDataBuffer[0] == USB_GET_CURR_NOTES){
                //LEGACY!! only there to make old program versions not crash
                memset(ToSendDataBuffer, 0, 64);
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_ENTER_PROGMODE){    //enter firmware update mode. In this mode only FWUPDATE commands will be processed, and any form of midi data ignored
                Midi_setEnabled(0);
                progMode = 1;
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_CLEAR_ALL_PROGRAMMS){   //batch erase of all program presets, used for bulk writing to limit the overall re-write count
                //LEGACY!!
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_CLEAR_ALL_COILS){   //batch erase of all coil configurations, used for bulk writing to limit the overall re-write count
                NVM_clearAllCoils();
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_FWVERSION){     //read the firmware version string
                memset(ToSendDataBuffer, 0, 64);
                memcpy(&ToSendDataBuffer[1], NVM_getFWVersionString(), 24);
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_BLVERSION){     //read the bootloader version string
                memset(ToSendDataBuffer, 0, 64);
                char * blVer = NVM_getBootloaderVersionString();
                memcpy(&ToSendDataBuffer[1], blVer, 24);
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                vPortFree(blVer);
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_SERIAL_NR){     //read the devices serial number. this is stored in the boot flash, and will not be changed when updating the firmware
                ToSendDataBuffer[0] = USB_CMD_GET_SERIAL_NR;
                uint32_t sn = NVM_getSerialNumber();
                ToSendDataBuffer[4] = (sn >> 24);
                ToSendDataBuffer[3] = (sn >> 16);
                ToSendDataBuffer[2] = (sn >> 8);
                ToSendDataBuffer[1] = (sn);
                
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_SET_USBPID){     //read the devices serial number. this is stored in the boot flash, and will not be changed when updating the firmware
                Midi_setEnabled(0);
                uint16_t newPID = (ConfigReceivedDataBuffer[2] << 8) | ConfigReceivedDataBuffer[1];
                NVM_updateDevicePID(newPID);
                Midi_setEnabled(1);
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_IS_SAFE){     //read the devices serial number. this is stored in the boot flash, and will not be changed when updating the firmware
                ToSendDataBuffer[0] = USB_CMD_IS_SAFE;
                ToSendDataBuffer[1] = Midi_areAllNotesOff();
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_BLINK){
                //we are supposed to blink an led to identify us
                Midi_blinkLED = ConfigReceivedDataBuffer[1];
                if(!Midi_blinkLED) LATBSET =_LATB_LATB8_MASK;
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_FWUPDATE_REBOOT){   //to a software reset
                Midi_setEnabled(0);     //mainly there to disable the output. If this is not done we rely on the gate pulldown on the MOSFET to turn off the output, which would take quite long
                __pic32_software_reset();

            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_PING){
                //called by the software every 250ms to check for device presence... no response or action required
            }else{
                HID_parseCMD(ConfigReceivedDataBuffer, ToSendDataBuffer, Midi_configTxHandle, 64);
            }

        }else{
            if(payload != 0){ //we are in the process of receiving data packets for a firmware update
                memcpy(payload + FWUpdate_currPLOffset, ConfigReceivedDataBuffer, 64);  //copy the data into the buffer at the correct location and increase the offset
                FWUpdate_currPLOffset += 64;
                
                if(FWUpdate_currPLOffset >= PAGE_SIZE){ //we have received one page of data -> write it to the NVM at the offset given by the packetcount * packet Size
                    NVM_writeFWUpdate(payload, currPage);
                    vPortFree(payload);
                    
                    //return to normal parsing of packets
                    payload = 0;
                    FWUpdate_currPLOffset = 0;
                    
                    //inform the software that we are ready for another packet, this is necessary as flash write speed is slow and not consistent enough to just use a wait in the software after writing one packet
                    ToSendDataBuffer[0] = USB_CMD_FWUPDATE_BULK_WRITE_FINISHED;
                    Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64); 
                }
            }else{
                
                if(ConfigReceivedDataBuffer[0] == USB_CMD_EXIT_PROGMODE){       //go back to normal operation
                    progMode = 0;
                    
                }else if(ConfigReceivedDataBuffer[0] == USB_CMD_FWUPDATE_ERASE){//erase any possible previous firmware updates
                    NVM_eraseFWUpdate();
                    memset(ToSendDataBuffer, 0, 64);
                    //inform the software that the erase operation is complete, this is necessary as flash erase speed is relatively slow and not consistent enough to just use a wait in the software after writing one packet
                    ToSendDataBuffer[0] = USB_CMD_FWUPDATE_ERASE;
                    Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                    
                }else if(ConfigReceivedDataBuffer[0] == USB_CMD_FWUPDATE_COMMIT){   //write the appropriate value to the fwStatus byte, to tell the bootloader we mean business ;)
                    NVM_commitFWUpdate(ConfigReceivedDataBuffer[1]);
                    Midi_blinkLED = 1;
                    
                }else if(ConfigReceivedDataBuffer[0] == USB_CMD_FWUPDATE_REBOOT){   //to a software reset
                    __pic32_software_reset();
                    
                }else if(ConfigReceivedDataBuffer[0] == USB_CMD_FWUPDATE_GET_CRC){  //calculate the CRC of the update firmware in flash
                    uint32_t crc = NVM_getUpdateCRC();
                    ToSendDataBuffer[0] = USB_CMD_FWUPDATE_GET_CRC;
                    ToSendDataBuffer[1] = (crc >> 24);
                    ToSendDataBuffer[2] = (crc >> 16);
                    ToSendDataBuffer[3] = (crc >> 8);
                    ToSendDataBuffer[4] = (crc);
                    Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                    
                }else if(ConfigReceivedDataBuffer[0] == USB_CMD_FWUPDATE_START_BULK_WRITE){ //prepare to receive civil judgment, wait no we prepare to receive packet data
                    payload = pvPortMalloc(PAGE_SIZE);
                    if(payload == 0) UART_print("nope we need more ram :(\r\n");
                    FWUpdate_currPLOffset = 0;
                    currPage = (ConfigReceivedDataBuffer[1] << 8) | (ConfigReceivedDataBuffer[2]);
                    
                    memset(ToSendDataBuffer, 0, 64);                            //return the size of the page to be written
                    ToSendDataBuffer[0] = USB_CMD_FWUPDATE_COMMIT;
                    ToSendDataBuffer[1] = PAGE_SIZE >> 8;
                    ToSendDataBuffer[2] = PAGE_SIZE & 0xff;
                    Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64); 
                }
            }
        }
        
        //get ready to receive more data
        Midi_configRxHandle = USBRxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT,(uint8_t*)&ConfigReceivedDataBuffer,64);   
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
        T2CONCLR = _T2CON_ON_MASK;
        T3CONCLR = _T3CON_ON_MASK;
        T4CONCLR = _T4CON_ON_MASK;
        T5CONCLR = _T5CON_ON_MASK;
        
        LATBCLR = _LATB_LATB15_MASK | _LATB_LATB5_MASK; //turn off the output
                    
        Midi_LED(LED_DUTY_LIMITER, LED_OFF);
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
    if(Midi_voice[0].otCurrent > 0 && T2CONbits.ON) return 0;
    if(Midi_voice[1].otCurrent > 0 && T3CONbits.ON) return 0;
    if(Midi_voice[2].otCurrent > 0 && T4CONbits.ON) return 0;
    if(Midi_voice[3].otCurrent > 0 && T5CONbits.ON) return 0;
    return 1;
}

//stereo volume mapping
void Midi_calculateStereoVolume(uint8_t channel){
    uint8_t stereoPosition = NVM_getConfig()->stereoPosition;
    uint8_t stereoWidth = NVM_getConfig()->stereoWidth;
    uint8_t stereoSlope = NVM_getConfig()->stereoSlope;
    
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

//"HAL" for the leds, takes the settings for led usage into account
void Midi_LED(uint8_t type, uint8_t state){
    if(NVM_getConfig()->ledMode1 == type) {                     //blue
        if(state == LED_ON) LATBCLR = _LATB_LATB9_MASK; else if(state == LED_OFF) LATBSET = _LATB_LATB9_MASK; else if(state == LED_BLINK) LATBINV = _LATB_LATB9_MASK; //blink coms led
    }
    if(NVM_getConfig()->ledMode2 == type && !Midi_blinkLED) {   //white
        if(state == LED_ON) LATBCLR = _LATB_LATB8_MASK; else if(state == LED_OFF) LATBSET = _LATB_LATB8_MASK; else if(state == LED_BLINK) LATBINV = _LATB_LATB8_MASK; //blink coms led
    }
    if(NVM_getConfig()->ledMode3 == type) {                     //green
        if(state == LED_ON) LATBCLR = _LATB_LATB7_MASK; else if(state == LED_OFF) LATBSET = _LATB_LATB7_MASK; else if(state == LED_BLINK) LATBINV = _LATB_LATB7_MASK; //blink coms led
    }
}