#ifndef MIDI_PROC_INC
#define MIDI_PROC_INC
    
#include <stdint.h>

//Midi command macros
#define MIDI_CMD_NOTE_OFF               0x80
#define MIDI_CLK                        0xF8
#define MIDI_CMD_NOTE_ON                0x90
#define MIDI_CMD_KEY_PRESSURE           0xA0
#define MIDI_CMD_CONTROLLER_CHANGE      0xB0
#define MIDI_CMD_PROGRAM_CHANGE         0xC0
#define MIDI_CMD_CHANNEL_PRESSURE       0xD0
#define MIDI_CMD_PITCH_BEND             0xE0
#define MIDI_CMD_SYSEX                  0xF0

#define MIDI_CC_ALL_SOUND_OFF           0x78
#define MIDI_CC_RESET_ALL_CONTROLLERS   0x79
#define MIDI_CC_ALL_NOTES_OFF           0x7B
#define MIDI_CC_VOLUME                  0x07
#define MIDI_CC_PORTAMENTO_TIME         0x05
#define MIDI_CC_PAN                     0x0A
#define MIDI_CC_RPN_LSB                 0x64
#define MIDI_CC_RPN_MSB                 0x65
#define MIDI_CC_NRPN_LSB                0x62
#define MIDI_CC_SUSTAIN_PEDAL           0x40
#define MIDI_CC_DAMPER_PEDAL            0x43
#define MIDI_CC_NRPN_MSB                0x63
#define MIDI_CC_DATA_FINE               0x26
#define MIDI_CC_DATA_COARSE             0x06
#define MIDI_CC_RPN_MSB                 0x65

#define MIDI_RPN_BENDRANGE              0x0000

#define MIDI_CHANNELCOUNT 16

#define MIDI_VOLUME_MAX 0x7f

#define MIDIPROC_VERSION 1

//frequency scaler macros
#define MIDIPROC_SCALE_PITCHBEND(FREQ, CHANNEL) ((FREQ * MidiProcessor_getBendFactor(CHANNEL)) >> 13)

//volume scaler macros
#define MIDIPROC_SCALE_VOLUME(VOLUME, CHANNEL) (((VOLUME * MidiProcessor_getVolume(CHANNEL)) / 0x7f)
#define MIDIPROC_SCALE_VOLUME_STEREO(VOLUME, CHANNEL) (((VOLUME * MidiProcessor_getStereoVolume(CHANNEL)) / 0x7f)

typedef struct{
    uint8_t parameters[128];    //all midi parameters
    
    //pitchbend parameters
    uint32_t        bendFactor;     //current pitch factor, fixed point
    uint16_t        bendRangeRaw;   //value written by setRPN command
    float           bendRange;      //value calculated from bendRangeRaw, buffered for speed
    
    uint32_t        lastFrequency;
    
    //volume effect variables
    uint32_t        stereoVolume; // buffered instead of calculated live for speed
    
    uint8_t         programm; 
} MidiChannelDescriptor_t;

void MidiProcessor_init();
void MidiProc_setStereoConfig(uint32_t stereoPosition, uint32_t stereoWidth, uint32_t stereoSlope);
uint32_t MidiProcessor_noteToFrequency(uint32_t note);
void MidiProcessor_processCmd(uint32_t cable, uint32_t channel, uint32_t cmd, uint32_t param1, uint32_t param2);
uint32_t MidiProcessor_getStereoVolume(uint32_t channel);
uint32_t MidiProcessor_getCCValue(uint32_t channel, uint32_t id);
uint32_t MidiProcessor_getVolume(uint32_t channel);
uint32_t MidiProcessor_getDamperVolume(uint32_t channel);
uint32_t MidiProcessor_getBendFactor(uint32_t channel);
void MidiProcessor_setLastFrequency(uint32_t channel, uint32_t frequency);
uint32_t MidiProcessor_getLastFrequency(uint32_t channel);

#endif