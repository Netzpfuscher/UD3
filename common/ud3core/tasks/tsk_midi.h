/*
 * UD3
 *
 * Copyright (c) 2018 Jens Kerrinnes
 * Copyright (c) 2015 Steve Ward
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

#if !defined(tsk_midi_TASK_H)
#define tsk_midi_TASK_H

/*
 * Add user task definitions, types, includes and other things in the below
 * merge region to customize the task.
 */
/* `#START USER_TYPES_AND_DEFINES` */
#include <device.h>
    
    /* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
    
#include "cli_basic.h"
    
typedef struct __midich__ {
	//uint8 expression; // Expression: Control change (Bxh) 0 bH
	uint8 rpn_lsb;	// RPN (LSB): Control change (Bxh) 64 H
	uint8 rpn_msb;	// RPN (MSB): Control change (Bxh) 65 H
	uint8 bendrange;  // Pitch Bend Sensitivity (0 - ffh)
	int16 pitchbend;  // Pitch Bend (0-3fffh)
    uint8 attack;
    uint8 decay;
    uint8 release;
	uint8 updated;	// Was it updated (whether BentRange or PitchBent was rewritten)
} MIDICH;

typedef struct __channel__ {
	uint8 midich;	// Channel of midi (0 - 15)
	uint8 miditone;  // Midi's tone number (0-127)
	uint8 volume;	// Volume (0 - 127) Not immediately reflected in port
	uint8 updated;   // Was it updated?
    uint16 halfcount;
    uint32 freq;
    uint8 adsr_state;
    uint8 adsr_count;
    uint8 sustain;
    uint8 old_gate;
} CHANNEL;
    
#define N_CHANNEL 8

#define N_MIDICHANNEL 16

#define SID_CHANNELS 3

xQueueHandle qMIDI_rx;

/* `#END` */

void tsk_midi_Start(void);

void update_midi_duty();

void switch_synth(uint8_t synth);

void kill_accu();

extern xQueueHandle qSID;

extern MIDICH midich[N_MIDICHANNEL];

extern CHANNEL channel[N_CHANNEL];

extern const uint8_t kill_msg[3];

extern volatile uint32_t next_frame;

struct sid_f{
    uint16_t freq[SID_CHANNELS];
    uint16_t half[SID_CHANNELS];
    uint16_t pw[SID_CHANNELS];
    uint8_t gate[SID_CHANNELS];
    uint8_t wave[SID_CHANNELS];
    uint8_t attack[SID_CHANNELS];
    uint8_t decay[SID_CHANNELS];
    uint8_t sustain[SID_CHANNELS];
    uint8_t release[SID_CHANNELS];
    uint16_t master_pw;
    uint32_t next_frame;
};

#define SYNTH_OFF  0
#define SYNTH_MIDI 1
#define SYNTH_SID  2

struct _filter{
    uint16_t min;
    uint16_t max;
    uint8_t channel[N_MIDICHANNEL];
};

extern struct _filter filter;

uint8_t command_SynthMon(char *commandline, port_str *ptr);

/*
 * Add user function prototypes in the below merge region to add user
 * functionality to the task definition.
 */
/* `#START USER_TASK_PROTOS` */

/* `#END` */

/* ------------------------------------------------------------------------ */
#endif
/* [] END OF FILE */
