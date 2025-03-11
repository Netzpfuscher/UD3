 /*
 * File:   midiProcessor.c
 * Author: Thorb
 *
 * Created on 10 January 2023, 23:16
 */

#include <math.h>

#include "MidiProcessor.h"
#include "SignalGenerator.h"
#include "VMS.h"
#include "VMSWrapper.h"
#include "MidiFilter.h"
#include "NoteMapper.h"

#define MIDIPROC_CH_CURR_RPN(CHANNEL) (((channelDescriptors[CHANNEL].parameters[MIDI_CC_RPN_MSB] << 8) & 0xff00) | (channelDescriptors[CHANNEL].parameters[MIDI_CC_RPN_LSB] & 0xff))
#define MIDIPROC_CH_CURR_DATA(CHANNEL) (((channelDescriptors[CHANNEL].parameters[MIDI_CC_DATA_COARSE] << 8) & 0xff00) | (channelDescriptors[CHANNEL].parameters[MIDI_CC_DATA_FINE] & 0xff))

//Note frequency lookup table
static const uint32_t Midi_NoteFreq_dHz[] = {82, 87, 92, 97, 103, 109, 116, 122, 130, 138, 146, 154, 164, 173, 184, 194, 206, 218, 231, 245, 260, 275, 291, 309, 327, 346, 367, 389, 412, 437, 462, 490, 519, 550, 583, 617, 654, 693, 734, 778, 824, 873, 925, 980, 1038, 1100, 1165, 1235, 1308, 1386, 1468, 1556, 1648, 1746, 1850, 1960, 2077, 2200, 2331, 2469, 2616, 2772, 2937, 3111, 3296, 3492, 3700, 3920, 4153, 4400, 4662, 4939, 5233, 5544, 5873, 6223, 6593, 6985, 7400, 7840, 8306, 8800, 9323, 9878, 10465, 11087, 11747, 12445, 13185, 13969, 14800, 15680, 16612, 17600, 18647, 19755, 20930, 22175, 23493, 24890, 26370, 27938, 29600, 31360, 33224, 35200, 37293, 39511, 41860, 44349, 46986, 49780, 52740, 55877, 59199, 62719, 66449, 70400, 74586, 79021, 83720, 88698, 93973, 99561, 105481, 111753, 118398, 125439};

static MidiChannelDescriptor_t * channelDescriptors;

static void MidiProcessor_DataHandler(uint32_t channel, uint32_t parameter);
static void MidiProcessor_updateStereoVolume(uint32_t channel);
static void resetChannelData(uint32_t channel);
static MidiFilterHandle_t * filters[SIGGEN_OUTPUTCOUNT];

void MidiProcessor_init(){
    //TODO add stereo parameters to the UD3
    
    //initialize midi filters
    for(uint32_t i = 0; i < SIGGEN_OUTPUTCOUNT; i++){
        filters[i] = MidiFilter_create(i);
    }
    
    channelDescriptors = pvPortMalloc(sizeof(MidiChannelDescriptor_t)*MIDI_CHANNELCOUNT);
    for(uint32_t i = 0; i < MIDI_CHANNELCOUNT; i++){
        resetChannelData(i);
    }
}

uint32_t MidiProcessor_noteToFrequency_dHz(uint32_t note){
    return Midi_NoteFreq_dHz[note];
}

uint32_t MidiProcessor_getLastFrequency(uint32_t channel){
    return channelDescriptors[channel].lastFrequency;
}

void MidiProcessor_setLastFrequency(uint32_t channel, uint32_t frequency){
    channelDescriptors[channel].lastFrequency = frequency;
}

uint32_t MidiProcessor_getBendFactor(uint32_t channel){
    return channelDescriptors[channel].bendFactor;
}

uint32_t MidiProcessor_getDamperVolume(uint32_t channel){
    return channelDescriptors[channel].parameters[MIDI_CC_DAMPER_PEDAL] ? 0xd3 : MIDI_VOLUME_MAX;
}

uint32_t MidiProcessor_getVolume(uint32_t channel){
    return channelDescriptors[channel].parameters[MIDI_CC_VOLUME];
}

uint32_t MidiProcessor_getStereoVolume(uint32_t channel){
    return channelDescriptors[channel].stereoVolume;
}

uint32_t MidiProcessor_getCCValue(uint32_t channel, uint32_t id){
    return channelDescriptors[channel].parameters[id];
}

static void resetChannelData(uint32_t channel){
            
    //we also want to reset other dynamic values incase they got messed up and the user hit stop because something sounded wrong
    channelDescriptors[channel].bendRange = 2.0;
    memset(channelDescriptors[channel].parameters, 0, 128);
    channelDescriptors[channel].parameters[MIDI_CC_VOLUME] = MIDI_VOLUME_MAX;

    channelDescriptors[channel].bendFactor = 1<<13;
    channelDescriptors[channel].bendRangeRaw = 0;
    channelDescriptors[channel].bendRange = 2.0;

    channelDescriptors[channel].stereoVolume = MIDI_VOLUME_MAX;
    //TODO: MidiProcessor_recalculateStereoVolume();

    //channelDescriptors[channel].programm = 0; 
}

void MidiProcessor_resetMidi(){
    //reset all midi data
    
    VMSW_panicHandler();
    SigGen_killAudio();
    
    //TODO evaluate if this should affect all channels:
    //reset all bend factors
    for(uint32_t killChannel = 0;killChannel < 16; killChannel++){
        resetChannelData(killChannel);
    }
}

void MidiProcessor_processCmd(uint32_t cable, uint32_t channel, uint32_t cmd, uint32_t param1, uint32_t param2){
    if(cmd == MIDI_CMD_NOTE_OFF){
        for(uint32_t i = 0; i < SIGGEN_OUTPUTCOUNT; i++){
            MidiFilter_eventHandler(filters[i], MIDI_EVT_NOTEOFF, channel, param1, (uint32_t)NULL, (uint32_t)NULL);
        }
        //send noteOff event to mapper
        
    }else if(cmd == MIDI_CMD_NOTE_ON){
        
        if(param2 > 0 && channelDescriptors[channel].parameters[MIDI_CC_VOLUME] > 0  && channelDescriptors[channel].stereoVolume > 0 ){  //is the note volume > 0?
            //yes, send noteOn event to the mapper
            for(uint32_t i = 0; i < SIGGEN_OUTPUTCOUNT; i++){
                MidiFilter_eventHandler(filters[i], MIDI_EVT_NOTEON, channel, param1, param2, channelDescriptors[channel].programm);
            }
        }else{
            //no, volume == 0. Either the volume modifiers are 0 or the note velocity is 0. A velocity of 0 is sometimes used instead of noteOff
            for(uint32_t i = 0; i < SIGGEN_OUTPUTCOUNT; i++){
                MidiFilter_eventHandler(filters[i], MIDI_EVT_NOTEOFF, channel, param1, (uint32_t)NULL, (uint32_t)NULL);
            }
            //send noteOff event to mapper
        }

    }else if(cmd == MIDI_CMD_CONTROLLER_CHANGE){
        //write value into buffer. Those control values that have no value will be written too, but never read
        channelDescriptors[channel].parameters[param1] = param2;
        
        //now check if we need to do anything with the data we received
        if(param1 == MIDI_CC_ALL_SOUND_OFF || param1 == MIDI_CC_ALL_NOTES_OFF || param1 == MIDI_CC_RESET_ALL_CONTROLLERS){ //Midi panic, and sometimes used by programms when you press the "stop" button
            //user pressed either the stop or midi panic button. Make sure to kill the output and reset anything else live midi related
            MidiProcessor_resetMidi();
            
        }else if(param1 == MIDI_CC_VOLUME){
            //send volume_change event to the mapper
            Mapper_volumeChangeHandler(channel);
            
        }else if(param1 == MIDI_CC_PAN){
            //recalculate stereo volume
            MidiProcessor_updateStereoVolume(channel);
            
            //send volume_change event to the mapper
            Mapper_volumeChangeHandler(channel);

        }else if(param1 == MIDI_CC_DATA_FINE || param1 == MIDI_CC_DATA_COARSE){
            //Data was received (might be a RPN change), go to the matching handler
            MidiProcessor_DataHandler(channel, param2);

        }else if(param1 == MIDI_CC_NRPN_LSB || param1 == MIDI_CC_NRPN_MSB){
            //NRPN change was requested, we don't support any. However a spurious NRPN message with not conforming RPN message before it would cause trouble, as a valid RPN might still be selected. make this invalid just to be sure
            channelDescriptors[channel].parameters[MIDI_CC_RPN_MSB] = 0x3f;
            channelDescriptors[channel].parameters[MIDI_CC_RPN_LSB] = 0x3f;
        }


    }else if(cmd == MIDI_CMD_PITCH_BEND){
        //convert the two parameters to one signed 14bit value
        int16_t bendParameter = (((param2 << 7) | param1) - 8192);
        
        //calculate the factor that frequencies need to be multiplied with and convert to fixed point. Resolution is 8/24. ( (float) 0x10000 is equal to << 16, for a maximum frequency of ~25.000)
        float bendOffset = (float) bendParameter / 8192.0;
        channelDescriptors[channel].bendFactor = (uint32_t) (powf(2, (bendOffset * (float) channelDescriptors[channel].bendRange) / 12.0) * (float) 8192);

        //call bendHandler to tweak already playing notes
        Mapper_bendHandler(channel);

    }else if(cmd == MIDI_CMD_PROGRAM_CHANGE){
        //programm was changed, call mapper and update the map data pointer
        channelDescriptors[channel].programm = param1;
        Mapper_programmChangeHandler(channel, param1);
    }
}

static void MidiProcessor_updateStereoVolume(uint32_t channel){
    channelDescriptors[channel].stereoVolume = MIDI_VOLUME_MAX;
    
    //TODO... we still don't have NVM storage of config parameters
    uint8_t stereoPosition = 64;
    uint8_t stereoWidth = 127;
    uint8_t stereoSlope = 255;
    
    int16_t sp = channelDescriptors[channel].parameters[MIDI_CC_PAN];
    int16_t rsStart = (stereoPosition - stereoWidth - MIDI_VOLUME_MAX/stereoSlope);
    int16_t fsEnd = (stereoPosition + stereoWidth + MIDI_VOLUME_MAX/stereoSlope);
    
    
    if(sp < rsStart){                                                   //span before volume > 0
        channelDescriptors[channel].stereoVolume = 0;
    }else if(sp > rsStart && sp < (stereoPosition - stereoWidth)){      //rising slope
        channelDescriptors[channel].stereoVolume = (sp - rsStart) * stereoSlope;
    }else if(sp < (stereoPosition + stereoWidth) && sp > (stereoPosition - stereoWidth)){   //span of 0xff
        channelDescriptors[channel].stereoVolume = MIDI_VOLUME_MAX;
    }else if(sp < fsEnd && sp > (stereoPosition - stereoWidth)){        //falling slope
        channelDescriptors[channel].stereoVolume = MIDI_VOLUME_MAX - (sp - stereoPosition - stereoWidth) * stereoSlope;
    }else if(sp > fsEnd){                                               //span after the volume curve
        channelDescriptors[channel].stereoVolume = 0;
    }
}

//Some midi data 
static void MidiProcessor_DataHandler(uint32_t channel, uint32_t parameter){
    if(MIDIPROC_CH_CURR_RPN(channel) == MIDI_RPN_BENDRANGE){     //get the 100ths semitones for the bendrange
        channelDescriptors[channel].bendRangeRaw = MIDIPROC_CH_CURR_DATA(channel);
        //channelDescriptors[channel].bendRange = (float) channelDescriptors[channel].bendRangeRaw / 100.0;
        
        //TODO shouldn't we reset or at least re-calculate the bend-factor here?
    }
}