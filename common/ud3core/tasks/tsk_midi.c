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

#include "cyapicallbacks.h"
#include <cytypes.h>

#include "tsk_midi.h"

#include "helper/fifo.h"

xTaskHandle tsk_midi_TaskHandle;
uint8 tsk_midi_initVar = 0u;

#if (1 == 1)
xSemaphoreHandle tsk_midi_Mutex;
#endif

/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */
#include "cli_common.h"
#include "interrupter.h"
#include "tsk_midi.h"
#include "tsk_priority.h"
#include "telemetry.h"
#include <device.h>
#include <math.h>
#include <stdio.h>

#define PITCHBEND_ZEROBIAS (0x2000)
#define PITCHBEND_DIVIDER ((float)0x1fff)

#define N_CHANNEL 4
#define N_BUFFER 8
#define N_DISPCHANNEL 4

#define N_MIDICHANNEL 16

#define COMMAND_NOTEONOFF 1
#define COMMAND_NOTEOFF 4
#define COMMAND_CONTROLCHANGE 2
#define COMMAND_PITCHBEND 3

// MIDI Find the frequency from the tone number
#define MIDITONENUM_FREQ(n) (440.0 * pow(2.0, ((n)-69.0) / 12.0))
// Calculate 1/2 cycle count number from frequency
// SG_Timer Is divided by this to judge the plus or minus of the square wave by odd number or even number
// 2 counts = 1 / 150000 sec, n counts = 1 / f = n / (150000 * 2)
// n = 300000 / f, n / 2 = 150000 / f
#define FREQ_HALFCOUNT(f) ((int16)floor(150000.0 / (f)))

typedef struct __note__ {
	uint8 command;
	uint8 midich;
	union {
		struct {
			uint8 tone, vol;
		} noteonoff;
		struct {
			uint8 n, value;
		} controlchange;
		struct {
			int16 value;
		} pitchbend;
	} data;
} NOTE;

typedef struct __midich__ {
	uint8 expression; // Expression: Control change (Bxh) 0 bH
	uint8 rpn_lsb;	// RPN (LSB): Control change (Bxh) 64 H
	uint8 rpn_msb;	// RPN (MSB): Control change (Bxh) 65 H
	uint8 bendrange;  // Pitch Bend Sensitivity (0 - ffh)
	int16 pitchbend;  // Pitch Bend (0-3fffh)
	uint8 updated;	// Was it updated (whether BentRange or PitchBent was rewritten)
} MIDICH;

typedef struct __channel__ {
	uint8 midich;	// Channel of midi (0 - 15)
	uint8 miditone;  // Midi's tone number (0-127)
	uint8 volume;	// Volume (0 - 127) Not immediately reflected in port
	uint8 updated;   // Was it updated?
	uint8 displayed; // Displayed 0 ... Displayed, 1 ... Note On, 2 ... Note Off
} CHANNEL;

typedef struct __port__ {
	uint16 halfcount; // A count that determines the frequency of port (for 1/2 period)
	uint8 volume;	 // Volume of port (0 - 127)
} PORT;

PORT *isr_port_ptr;

// Note on & off

NOTE *v_NOTE_NOTEONOFF(NOTE *v, uint8 midich, uint8 tone, uint8 vol) {
	v->command = COMMAND_NOTEONOFF;
	v->midich = midich;
	v->data.noteonoff.tone = tone;
	v->data.noteonoff.vol = vol;
	return (v);
}
// Control Change
NOTE *v_NOTE_CONTROLCHANGE(NOTE *v, uint8 midich, uint8 n, uint8 value) {
	v->command = COMMAND_CONTROLCHANGE;
	v->midich = midich;
	v->data.controlchange.n = n;
	v->data.controlchange.value = value;
	return (v);
}
// Pitchbend
NOTE *v_NOTE_PITCHBEND(NOTE *v, uint8 midich, uint8 lsb, uint8 msb) {
	v->command = COMMAND_PITCHBEND;
	v->midich = midich;
	v->data.pitchbend.value = (((int16)msb & 0x7f) << 7) + (lsb & 0x7f) - PITCHBEND_ZEROBIAS;
	return (v);
}

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */

uint8_t pulse_buffer[N_BUFFER];
fifo_t pulse_fifo;

CY_ISR(isr_midi) {

	uint8_t ch;
	uint8_t flag[N_CHANNEL];
	static uint8_t old_flag[N_CHANNEL];
	uint32 r = SG_Timer_ReadCounter();
	for (ch = 0; ch < N_CHANNEL; ch++) {
		flag[ch] = 0;
		if (isr_port_ptr[ch].volume > 0) {
			if ((r / isr_port_ptr[ch].halfcount) % 2 > 0) {
				flag[ch] = 1;
			}
		}
		if (flag[ch] > old_flag[ch]) {
			fifo_put(&pulse_fifo, isr_port_ptr[ch].volume);
		}
		old_flag[ch] = flag[ch];
	}

	int16_t temp_pulse;
	temp_pulse = fifo_get_nowait(&pulse_fifo);
	if (temp_pulse != -1) {
		interrupter_oneshot(param.pw, temp_pulse);
	}

	if (qcw_reg) {
		if ((ramp.modulation_value < 255) && (ramp.modulation_value > 0)) {
			ramp.modulation_value += param.qcw_ramp;
			if (ramp.modulation_value > 255)
				ramp.modulation_value = 255;
			ramp_control();
		}
	}
}

CY_ISR(isr_interrupter) {
	int16_t temp_pulse;
	temp_pulse = fifo_get_nowait(&pulse_fifo);
	if (temp_pulse != -1) {
		interrupter_oneshot(param.pw, temp_pulse);
	}
	Offtime_ReadStatusRegister();
}

void USBMIDI_1_callbackLocalMidiEvent(uint8 cable, uint8 *midiMsg) {
	static uint8_t skip_flag = 0; // Skipping system exclusive messages
	NOTE note_struct;
	uint8_t message_filter = midiMsg[0] & 0xf0;

	if (skip_flag) {
		if (midiMsg[0] == 0xf7 || midiMsg[1] == 0xf7 || midiMsg[2] == 0xf7) {
			skip_flag = 0;
		}
	} else {
		switch (message_filter) {
		case 0x90:
			v_NOTE_NOTEONOFF(&note_struct, midiMsg[0] & 0x0f, midiMsg[1], midiMsg[2]);
			xQueueSendFromISR(qMIDI_rx, &note_struct, NULL);
			break;
		case 0x80:
			v_NOTE_NOTEONOFF(&note_struct, midiMsg[0] & 0x0f, midiMsg[1], 0);
			xQueueSendFromISR(qMIDI_rx, &note_struct, NULL);
			break;
		case 0xb0:
			v_NOTE_CONTROLCHANGE(&note_struct, midiMsg[0] & 0x0f, midiMsg[1], midiMsg[2]);
			xQueueSendFromISR(qMIDI_rx, &note_struct, NULL);
			break;
		case 0xe0:
			v_NOTE_PITCHBEND(&note_struct, midiMsg[0] & 0x0f, midiMsg[1], midiMsg[2]);
			xQueueSendFromISR(qMIDI_rx, &note_struct, NULL);
			break;
		case 0xf0:
			skip_flag = 1;
			break;
		default:
			break;
		}
		if (skip_flag && (midiMsg[1] == 0xf7 || midiMsg[2] == 0xf7)) {
			skip_flag = 0;
		}
	}
}

// Initialization of sound source channel
void ChInit(CHANNEL channel[]) {
	for (uint8_t ch = 0; ch < N_CHANNEL; ch++) {
		channel[ch].volume = 0;
		channel[ch].updated = 0;
		if (ch < N_DISPCHANNEL)
			channel[ch].displayed = 0;
	}
}

// All channels of the port volume off
void PortVolumeAllOff(PORT port[]) {
	for (uint8_t ch = 0; ch < N_CHANNEL; ch++) {
		port[ch].volume = 0;
	}
}

void MidichInit(MIDICH ptr[]) {
	for (uint8_t cnt = 0; cnt < N_MIDICHANNEL; cnt++) {
		ptr[cnt].expression = 127; // Expression (0 - 127): Control change (Bxh) 0 BH xx

		// Keep RPN in reset state: In order to avoid malfunctioning when data entry comes in suddenly
		ptr[cnt].rpn_lsb = 127; // RPN (LSB): Control change (Bxh) 64H xx
		ptr[cnt].rpn_msb = 127; // RPN (MSB): Control change (Bxh) 65H xx
		// When RPN = (0, 0)
		// Data entry (MSB): Control change (Bxh) 05 H xx
		// Update Pitch Bend Sensitivity
		ptr[cnt].bendrange = 2; // Pitch Bend Sensitivity (0 - ffh)
								// Update pitch bend with Exh LSB MSB
		ptr[cnt].pitchbend = 0; // Pitch Bend (-8192 - 8191)
		ptr[cnt].updated = 0;
	}
}

void process(NOTE *v, CHANNEL channel[], MIDICH midich[]) {
	uint8_t ch;
	if (v->command == COMMAND_NOTEONOFF) {		 // Processing of note on / off
		if (v->data.noteonoff.vol > 0) {		 // Note ON
			for (ch = 0; ch < N_CHANNEL; ch++) { // Search for ports that are already ringing with the same MIDI channel & node number
				if (channel[ch].volume > 0 && channel[ch].midich == v->midich && channel[ch].miditone == v->data.noteonoff.tone)
					break;
			}
			if (ch == N_CHANNEL) { // When there is no already-sounding port
				// First time ringing: search for port of off
				for (ch = 0; ch < N_CHANNEL; ch++) {
					if (channel[ch].volume == 0)
						break;
				}
			}
			if (ch < N_CHANNEL) { // A port was found
				channel[ch].midich = v->midich;
				channel[ch].miditone = v->data.noteonoff.tone;
				channel[ch].volume = v->data.noteonoff.vol;
				channel[ch].updated = 1; // This port has been updated
				if (ch < N_DISPCHANNEL)
					channel[ch].displayed = 1; // This port is undrawn
			}
		} else {								 // Note OFF
			for (ch = 0; ch < N_CHANNEL; ch++) { // Search for ports that are already ringing with the same MIDI channel & node number
				if (channel[ch].volume > 0 && channel[ch].midich == v->midich && channel[ch].miditone == v->data.noteonoff.tone)
					break;
			}
			if (ch < N_CHANNEL) { // A port was found

				channel[ch].volume = 0;
				channel[ch].updated = 1; // This port has been updated
				if (ch < N_DISPCHANNEL)
					channel[ch].displayed = 2; // This port is undrawn
			}
		}
	} else if (v->command == COMMAND_CONTROLCHANGE) { // Control Change Processing

		switch (v->data.controlchange.n) {
		case 0x0b: // Expression
			midich[v->midich].expression = v->data.controlchange.value;
			midich[v->midich].updated = 1; // This MIDI channel has been updated
			break;
		case 0x62: // NRPN(LSB)
		case 0x63: // NRPN(MSB)
			// RPN Reset
			midich[v->midich].rpn_lsb = 127; // RPN (LSB): Control change (Bxh) 64 H
			midich[v->midich].rpn_msb = 127; // RPN (MSB): Control change (Bxh) 65 H
			break;
		case 0x64: // RPN(LSB)
			midich[v->midich].rpn_lsb = v->data.controlchange.value;
			break;
		case 0x65: // RPN(MSB)
			midich[v->midich].rpn_msb = v->data.controlchange.value;
			break;
		case 0x06: // Data Entry
			if (midich[v->midich].rpn_lsb == 0 && midich[v->midich].rpn_msb == 0) {
				// Update Pitch Bend Sensitivity
				midich[v->midich].bendrange = v->data.controlchange.value;
				midich[v->midich].updated = 1; // This MIDI channel has been updated
				// Once accepted RPN reset
				midich[v->midich].rpn_lsb = 127; // RPN (LSB): Control change (Bxh) 64 H
				midich[v->midich].rpn_msb = 127; // RPN (MSB): Control change (Bxh) 65 H
			}
			break;
		case 0x77: //Panic Message
			for (ch = 0; ch < N_CHANNEL; ch++) {
				channel[ch].volume = 0;
				channel[ch].updated = 1;
				channel[ch].displayed = 2;
			}
			break;
		case 0x7B: //Kill all notes on channel
			for (ch = 0; ch < N_CHANNEL; ch++) {
				if (channel[ch].volume > 0 && channel[ch].midich == v->midich) {
					channel[ch].volume = 0;
					channel[ch].updated = 1;
					channel[ch].displayed = 2;
				}
			}
			break;
		}
	} else if (v->command == COMMAND_PITCHBEND) { // Processing Pitch Bend

		midich[v->midich].pitchbend = v->data.pitchbend.value;
		midich[v->midich].updated = 1; // This MIDI channel has been updated
	}
}

void reflect(PORT port[], CHANNEL channel[], MIDICH midich[]) {
	uint8_t ch;
	uint8_t mch;
    telemetry.midi_voices =0;
	// Reflect the status of the updated tone generator channel & MIDI channel on the port
	for (ch = 0; ch < N_CHANNEL; ch++) {
		mch = channel[ch].midich;
        if (channel[ch].volume > 0) telemetry.midi_voices++;
		if (channel[ch].updated || midich[mch].updated) {
			if (channel[ch].volume > 0) {
				port[ch].halfcount = FREQ_HALFCOUNT(MIDITONENUM_FREQ(channel[ch].miditone + ((float)midich[mch].pitchbend * midich[mch].bendrange) / PITCHBEND_DIVIDER));
				if (ch < N_DISPCHANNEL)
					channel[ch].displayed = 1; // Re-display required
			}
			port[ch].volume = (int32)channel[ch].volume * midich[mch].expression / 127; // Reflect Expression here
			channel[ch].updated = 0;													// Mission channel update work done
		}
	}
	for (mch = 0; mch < N_MIDICHANNEL; mch++) {
		midich[mch].updated = 0; // MIDI channel update work done
	}
}

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * This is the main procedure that comprises the task.  Place the code required
 * to preform the desired function within the merge regions of the task procedure
 * to add functionality to the task.
 */
void tsk_midi_TaskProc(void *pvParameters) {
	/*
	 * Add and initialize local variables that are allocated on the Task stack
	 * the the section below.
	 */
	/* `#START TASK_VARIABLES` */
	qMIDI_rx = xQueueCreate(256, sizeof(NOTE));

	fifo_init(&pulse_fifo, pulse_buffer, N_BUFFER);

	NOTE note_struct;

	// Tone generator channel status (updated according to MIDI messages)
	CHANNEL channel[N_CHANNEL];

	// MIDI channel status
	MIDICH midich[N_MIDICHANNEL];

	// Port status (updated at regular time intervals according to the status of the sound source channel)
	PORT port[N_CHANNEL];
	isr_port_ptr = port;

	new_midi_data = xSemaphoreCreateBinary();

	/* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */

	// Initialization of sound source channel
	ChInit(channel);
	// All channels of the port volume off
	PortVolumeAllOff(port);
	// MIDI Channel initialization
	MidichInit(midich);
	// Sound source relation module initialization
	SG_Timer_Start();

	isr_midi_StartEx(isr_midi);
	isr_interrupter_StartEx(isr_interrupter);

	/* `#END` */

	for (;;) {
		/* `#START TASK_LOOP_CODE` */

		if (xQueueReceive(qMIDI_rx, &note_struct, portMAX_DELAY)) {
			process(&note_struct, channel, midich);
			while (xQueueReceive(qMIDI_rx, &note_struct, 0)) {
				process(&note_struct, channel, midich);
			}
			reflect(port, channel, midich);
		}

		/* `#END` */
	}
}
/* ------------------------------------------------------------------------ */
void tsk_midi_Start(void) {
	/*
	 * Insert task global memeory initialization here. Since the OS does not
	 * initialize ANY global memory, execute the initialization here to make
	 * sure that your task data is properly 
	 */
	/* `#START TASK_GLOBAL_INIT` */

	/* `#END` */

	if (tsk_midi_initVar != 1) {
#if (1 == 1)
		tsk_midi_Mutex = xSemaphoreCreateMutex();
#endif

		/*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_midi_TaskProc, "MIDI-Svc", 512, NULL, PRIO_MIDI, &tsk_midi_TaskHandle);
		tsk_midi_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
