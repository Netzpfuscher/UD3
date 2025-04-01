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
    
#define N_MIDICHANNEL 16
#define MIDI_MSG_SIZE 4

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
#include "TTerm.h"
#include "SignalGenerator.h"
    
typedef struct __midich__ {
	//uint8 expression; // Expression: Control change (Bxh) 0 bH
	uint8_t rpn_lsb;	// RPN (LSB): Control change (Bxh) 64 H
	uint8_t rpn_msb;	// RPN (MSB): Control change (Bxh) 65 H
	uint8_t bendrange;  // Pitch Bend Sensitivity (0 - ffh)
	int16_t pitchbend;  // Pitch Bend (0-3fffh)
    uint8_t attack;
    uint8_t decay;
    uint8_t release;
	uint8_t updated;	// Was it updated (whether BentRange or PitchBent was rewritten)
} MIDICH;

extern xQueueHandle qMIDI_rx;

/* `#END` */

void tsk_midi_Start(void);

void tsk_midi_reset_skip();

void update_midi_duty();

void queue_midi_message(uint8 *midiMsg);

void tsk_midi_kill();
uint8_t CMD_midi_inject(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);

extern MIDICH midich[N_MIDICHANNEL];

/*
 * Add user function prototypes in the below merge region to add user
 * functionality to the task definition.
 */
/* `#START USER_TASK_PROTOS` */

/* `#END` */

/* ------------------------------------------------------------------------ */
#endif
/* [] END OF FILE */
