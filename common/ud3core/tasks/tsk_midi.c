/*
 * UD3
 *
 * Copyright (c) 2018 Jens Kerrinnes
 * Copyright (c) 2015 Steve Ward
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software withoutrestriction, including without limitation the rights to
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
#include "tsk_fault.h"
#include "clock.h"
#include "SignalGenerator.h"

xTaskHandle tsk_midi_TaskHandle;
uint8 tsk_midi_initVar = 0u;


/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */
#include "cli_common.h"
#include "interrupter.h"
#include "hardware.h"
#include "tsk_midi.h"
#include "tsk_priority.h"
#include "telemetry.h"
#include "alarmevent.h"
#include "qcw.h"
#include "helper/printf.h"
#include <device.h>
#include <stdlib.h>

#define PITCHBEND_ZEROBIAS (0x2000)
#define PITCHBEND_DIVIDER ((uint32_t)0x1fff)

enum midi_commands{
    COMMAND_NOTEONOFF,
    COMMAND_NOTEOFF,
    COMMAND_CONTROLCHANGE,
    COMMAND_PITCHBEND
};


void reflect();

const uint8_t kill_msg[3] = {0xb0, 0x77, 0x00};
xQueueHandle qMIDI_rx;

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


uint8_t skip_flag = 0; // Skipping system exclusive messages

void tsk_midi_reset_skip(){
    skip_flag = 0;
}


// MIDI channel status
MIDICH midich[N_MIDICHANNEL];

// Note on & off

NOTE *v_NOTE_NOTEONOFF(NOTE *v, uint8 midich, uint8 tone, uint8 vol) {
	v->command = COMMAND_NOTEONOFF;
	v->midich = midich;
    int16_t t_trans = tone + param.transpose;
    if(t_trans<0) t_trans=0;
    if(t_trans>127) t_trans=127;
	v->data.noteonoff.tone = t_trans;
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




void USBMIDI_1_callbackLocalMidiEvent(uint8 cable, uint8 *midiMsg) {
	
	NOTE note_struct;
	uint8_t message_filter = midiMsg[0] & 0xf0;
    uint8_t ret=pdTRUE;
	if (skip_flag) {
		if (midiMsg[0] == 0xf7 || midiMsg[1] == 0xf7 || midiMsg[2] == 0xf7) {
			skip_flag = 0;
		}
	} else {
		switch (message_filter) {
		case 0x90:
			v_NOTE_NOTEONOFF(&note_struct, midiMsg[0] & 0x0f, midiMsg[1], midiMsg[2]);
			ret = xQueueSendFromISR(qMIDI_rx, &note_struct, NULL);
			break;
		case 0x80:
			v_NOTE_NOTEONOFF(&note_struct, midiMsg[0] & 0x0f, midiMsg[1], 0);
			ret = xQueueSendFromISR(qMIDI_rx, &note_struct, NULL);
			break;
		case 0xb0:
			v_NOTE_CONTROLCHANGE(&note_struct, midiMsg[0] & 0x0f, midiMsg[1], midiMsg[2]);
			ret = xQueueSendFromISR(qMIDI_rx, &note_struct, NULL);
			break;
		case 0xe0:
			v_NOTE_PITCHBEND(&note_struct, midiMsg[0] & 0x0f, midiMsg[1], midiMsg[2]);
			ret = xQueueSendFromISR(qMIDI_rx, &note_struct, NULL);
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
        if(ret==errQUEUE_FULL){
             alarm_push(ALM_PRIO_WARN,warn_midi_overrun,ALM_NO_VALUE);
        }
	}
}

// Initialization of sound source channel
void ChInit(CHANNEL channel[]) {
	for (uint8_t ch = 0; ch < N_CHANNEL; ch++) {
		channel[ch].volume = 0;
		channel[ch].updated = 0;
	}
}

// All channels of the port volume off
void PortVolumeAllOff() {
	for (uint8_t ch = 0; ch < N_CHANNEL; ch++) {
		channel[ch].volume = 0;
	}
}

void MidichInit(MIDICH ptr[]) {
	for (uint8_t cnt = 0; cnt < N_MIDICHANNEL; cnt++) {
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

void process(NOTE *v) {
	uint8_t ch;
	if (v->command == COMMAND_NOTEONOFF) {		 // Processing of note on / off
		if (v->data.noteonoff.vol > 0) {		 // Note ON
			for (ch = 0; ch < N_CHANNEL; ch++) { // Search for ports that are already ringing with the same MIDI channel & node number
				if (channel[ch].adsr_state != ADSR_IDLE && channel[ch].midich == v->midich && channel[ch].miditone == v->data.noteonoff.tone){
					break;
                }
			}
			if (ch == N_CHANNEL) { // When there is no already-sounding port
				// First time ringing: search for port of off
				for (ch = 0; ch < N_CHANNEL; ch++) {
					if (channel[ch].adsr_state == ADSR_IDLE)
						break;
				}
			}
			if (ch < N_CHANNEL) { // A port was found
				channel[ch].midich = v->midich;
				channel[ch].miditone = v->data.noteonoff.tone;
                channel[ch].sustain = v->data.noteonoff.vol;
  
                if(v->data.noteonoff.vol>0){
                    if(param.synth == SYNTH_MIDI_QCW && !QCW_enable_Control) qcw_start();
                    channel[ch].adsr_state = ADSR_PENDING;
                }else{
                    channel[ch].adsr_state = ADSR_RELEASE;
                }
                
				channel[ch].updated = 1; // This port has been updated
                
			}
		} else {								 // Note OFF
			for (ch = 0; ch < N_CHANNEL; ch++) { // Search for ports that are already ringing with the same MIDI channel & node number
				if (channel[ch].volume > 0  && channel[ch].midich == v->midich && channel[ch].miditone == v->data.noteonoff.tone)
					break;
			}
			if (ch < N_CHANNEL) { // A port was found
                channel[ch].adsr_state=ADSR_RELEASE;
				channel[ch].updated = 1; // This port has been updated
			}
		}
	} else if (v->command == COMMAND_CONTROLCHANGE) { // Control Change Processing

		switch (v->data.controlchange.n) {
        case 0x48: //Release
            midich[v->midich].release = v->data.controlchange.value;
            break;
        case 0x49: //Attack
            midich[v->midich].attack = v->data.controlchange.value;
            break;
        case 0x4b: //Decay
            midich[v->midich].decay = v->data.controlchange.value;
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
                channel[ch].adsr_state = ADSR_IDLE;
                channel[ch].sustain = 0;
				channel[ch].updated = 1;
			}
			break;
		case 0x7B: //Kill all notes on channel
			for (ch = 0; ch < N_CHANNEL; ch++) {
				if (channel[ch].volume > 0 && channel[ch].midich == v->midich) {
					channel[ch].volume = 0;
                    channel[ch].adsr_state = ADSR_IDLE;
                    channel[ch].sustain = 0;
					channel[ch].updated = 1;
				}
			}
			break;
		}
	} else if (v->command == COMMAND_PITCHBEND) { // Processing Pitch Bend
		midich[v->midich].pitchbend = v->data.pitchbend.value;
		midich[v->midich].updated = 1; // This MIDI channel has been updated
	}
}

void update_midi_duty(){
    if(!tt.n.midi_voices.value) return;
    uint32_t dutycycle=0;

    for (uint8_t ch = 0; ch < N_CHANNEL; ch++) {    
        if (channel[ch].volume > 0){
            dutycycle+= ((uint32)channel[ch].sustain*(uint32)param.pw)/(127000ul/(uint32)channel[ch].freq);
        }
	}
  
    tt.n.duty.value = dutycycle;
    if(dutycycle>(configuration.max_tr_duty-param.temp_duty)){
        interrupter.pw = (param.pw * (configuration.max_tr_duty-param.temp_duty)) / dutycycle;
    }else{
        interrupter.pw = param.pw;
    }
}

void reflect() {
	uint8_t ch;
	uint8_t mch;
    uint32_t dutycycle=0;
    uint32_t pb;
	// Reflect the status of the updated tone generator channel & MIDI channel on the port
	for (ch = 0; ch < N_CHANNEL; ch++) {
		mch = channel[ch].midich;
		if (channel[ch].updated || midich[mch].updated) {
			if (channel[ch].adsr_state) {
                pb = ((((uint32_t)midich[mch].pitchbend*midich[mch].bendrange)<<10) / PITCHBEND_DIVIDER)<<6;
                channel[ch].freq = Q16n16_mtof((channel[ch].miditone<<16)+pb);
                channel[ch].halfcount = (SG_CLOCK_HALFCOUNT<<14) / (channel[ch].freq>>2);
                SigGen_channel_freq_fp8(ch,channel[ch].freq>>8);
                channel[ch].freq = channel[ch].freq >>16;
                
                if(channel[ch].adsr_state==ADSR_PENDING) channel[ch].adsr_state = ADSR_ATTACK;
                if(filter.channel[ch]==0 || channel[ch].freq < filter.min || channel[ch].freq > filter.max){
                    channel[ch].adsr_state = ADSR_IDLE;   
                    channel[ch].volume = 0;
                    channel[ch].sustain = 0;
                }
			}
			channel[ch].updated = 0;													// Mission channel update work done
		    midich[mch].updated = 0; // MIDI channel update work done
        }
        
        if (channel[ch].sustain > 0){
            dutycycle+= ((uint32)channel[ch].sustain*(uint32)param.pw)/(127000ul/(uint32)channel[ch].freq);
        }
	}
   
    tt.n.duty.value = dutycycle;
    if(dutycycle>(configuration.max_tr_duty-param.temp_duty)){
        interrupter.pw = (param.pw * (configuration.max_tr_duty-param.temp_duty)) / dutycycle;
    }else{
        interrupter.pw = param.pw;
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
	qMIDI_rx = xQueueCreate(N_QUEUE_MIDI, sizeof(NOTE));
    
	NOTE note_struct;
    
#if USE_DEBUG_DAC
    DEBUG_DAC_Start();
    Opamp_1_Start();
#endif

	/* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */

	// Initialization of sound source channel
	ChInit(channel);
	// All channels of the port volume off
	PortVolumeAllOff();
	// MIDI Channel initialization
	MidichInit(midich);
	// Sound source relation module initialization

    SigGen_init();

		/* `#END` */
    alarm_push(ALM_PRIO_INFO,warn_task_midi, ALM_NO_VALUE);
	for (;;) {
		/* `#START TASK_LOOP_CODE` */

        if(param.synth==SYNTH_MIDI ||param.synth==SYNTH_MIDI_QCW){
    		if (xQueueReceive(qMIDI_rx, &note_struct, portMAX_DELAY)) {
    			process(&note_struct);
    			while (xQueueReceive(qMIDI_rx, &note_struct, 0)) {
    				process(&note_struct);
    			}
    			reflect();
    		}
        }else{
        vTaskDelay(200 /portTICK_RATE_MS);
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

        /*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_midi_TaskProc, "MIDI-Svc", STACK_MIDI, NULL, PRIO_MIDI, &tsk_midi_TaskHandle);
		tsk_midi_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
