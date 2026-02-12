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

/**
 * @file tsk_midi.h
 * @brief MIDI processing task
 *
 * Receives MIDI messages from queue and processes them into signal generator
 * commands. Handles pitch bend, ADSR envelopes, and polyphonic voices.
 */

#if !defined(tsk_midi_TASK_H)
#define tsk_midi_TASK_H
    
#define N_MIDICHANNEL 16  //!< MIDI channels (1-16)
#define MIDI_MSG_SIZE 4   //!< MIDI message buffer size

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
    
/**
 * @brief MIDI channel state structure
 *
 * Tracks per-channel pitch bend and ADSR settings
 */
typedef struct __midich__ {
	uint8_t rpn_lsb;	//!< RPN LSB (control change 0x64)
	uint8_t rpn_msb;	//!< RPN MSB (control change 0x65)
	uint8_t bendrange;  //!< Pitch bend sensitivity (semitones)
	int16_t pitchbend;  //!< Pitch bend value (0-0x3FFF, center 0x2000)
    uint8_t attack;     //!< Attack time (custom parameter)
    uint8_t decay;      //!< Decay time (custom parameter)
    uint8_t release;    //!< Release time (custom parameter)
	uint8_t updated;	//!< Update flag for pitch bend changes
} MIDICH;

extern xQueueHandle qMIDI_rx; //!< MIDI receive queue

/* `#END` */

/**
 * @brief Start MIDI processing task
 */
void tsk_midi_Start(void);

/**
 * @brief Reset MIDI frame skip counter
 */
void tsk_midi_reset_skip();

/**
 * @brief Update MIDI duty cycle limits
 */
void update_midi_duty();

/**
 * @brief Queue MIDI message for processing
 * @param midiMsg Pointer to 4-byte MIDI message buffer
 */
void queue_midi_message(uint8 *midiMsg);

/**
 * @brief Kill all active MIDI voices
 */
void tsk_midi_kill();

/**
 * @brief MIDI inject CLI command (debug)
 * @param handle Terminal handle
 * @param argCount Argument count
 * @param args Argument array
 * @return Command status
 */
uint8_t CMD_midi_inject(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);

extern MIDICH midich[N_MIDICHANNEL]; //!< Per-channel MIDI state

/*
 * Add user function prototypes in the below merge region to add user
 * functionality to the task definition.
 */
/* `#START USER_TASK_PROTOS` */

/* `#END` */

/* ------------------------------------------------------------------------ */
#endif
/* [] END OF FILE */
