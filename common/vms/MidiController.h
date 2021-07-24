/*
    Copyright (C) 2020,2021 TMax-Electronics.de
   
    This file is part of the MidiStick Firmware.

    the MidiStick Firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    the MidiStick Firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the MidiStick Firmware.  If not, see <https://www.gnu.org/licenses/>.
*/
    
#if !defined(MidiController_H)
#define MidiController_H
    
#if PIC32
#include <sys/attribs.h>
#include <xc.h>
#endif
#include <stdint.h>

//#include "ConfigManager.h"
#include "helper/vms_types.h"

//Midi command macros
#define MIDI_CMD_NOTE_OFF               0x80
#define MIDI_CLK                        0xF8
#define MIDI_CMD_NOTE_ON                0x90
#define MIDI_CMD_KEY_PRESSURE           0xA0
#define MIDI_CMD_CONTROLLER_CHANGE      0xB0
#define MIDI_CC_ALL_SOUND_OFF           0x78
#define MIDI_CC_RESET_ALL_CONTROLLERS   0x79
#define MIDI_CC_ALL_NOTES_OFF           0x7B
#define MIDI_CC_VOLUME                  0x07
#define MIDI_CC_PTIME_COARSE            0x05
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
#define MIDI_CMD_PROGRAM_CHANGE         0xC0
#define MIDI_CMD_CHANNEL_PRESSURE       0xD0
#define MIDI_CMD_PITCH_BEND             0xE0
#define MIDI_CMD_SYSEX                  0xF0

#define MIDI_RPN_BENDRANGE              0x0000
/*
//ADST states
#define NOTE_OFF    0
#define ATTAC       1
#define DECAY       2
#define SUSTAIN     3
#define RELEASE     4*/

#define LED_POWER          0
#define LED_DATA           1
#define LED_OUT_ON         2
#define LED_DUTY_LIMITER   3
#define LED_ADSR_PREVIEW   4

#define LED_TYPE_COUNT     5

#define LED_OFF            0
#define LED_ON             1
#define LED_BLINK          2

#define AUX_AUDIO          0
#define AUX_E_STOP         1

#define AUX_MODE_COUNT     2

#define MIDI_VOICECOUNT 4


extern SynthVoice Midi_voice[MIDI_VOICECOUNT];
extern const uint16_t Midi_NoteFreq[128];

//extern CoilConfig * Midi_currCoil;
extern uint16_t Midi_minOnTimeVal;
extern uint16_t Midi_maxOnTimeVal;

extern ChannelInfo channelData[16];
extern unsigned Midi_enabled;

//Initialise midi stuff
void Midi_init();
void Midi_initTimers();

//Run the 1ms tick routines
void Midi_SOFHandler();

//Midi&Config CMD interpreter routines
void Midi_run(uint8_t* ReceivedDataBuffer);

//On time calculation functions
void Midi_setNoteOnTime(uint16_t onTimeUs, uint8_t ch);
void Midi_calculateStereoVolume(uint8_t channel);

//note config routines
void Midi_NoteOff(uint8_t voice, uint8_t channel);
void Midi_setNoteTPR(uint8_t voice, uint16_t freq);

//Calculates the ADSR Coefficients
void Midi_calculateADSR(uint8_t channel);

//Checks if any voices are still outputting data
unsigned Midi_areAllNotesOff();
void Midi_setEnabled(unsigned ena);

//Timer ISRs
//inline void Midi_timerHandler(uint8_t voice);

unsigned Midi_isNoteOff(uint8_t voice);
unsigned Midi_isNoteDecaying(uint8_t voice);
void Midi_noteOff(uint8_t voice);

ChannelInfo * Midi_getChannelData(uint8_t channel);
void Midi_mapDrumChannel(SynthVoice * target, uint8_t voice, uint8_t note, uint8_t velocity);
void Midi_flashCommsLED();

#endif