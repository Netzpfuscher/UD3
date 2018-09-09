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
#include <stdlib.h>

#define PITCHBEND_ZEROBIAS (0x2000)
#define PITCHBEND_DIVIDER ((uint32_t)0x1fff)

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

struct pulse_str {
	uint8_t  volume;
	uint16_t pw;
};

struct pulse_str isr1_pulse;
struct pulse_str isr2_pulse;

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
    uint32 freq;
} PORT;

PORT *isr_port_ptr;
CHANNEL *channel_ptr;
MIDICH *midich_ptr;

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

typedef uint32_t Q16n16;

static const uint32_t midiToFreq[128] =
    {
     0, 567670, 601425, 637188, 675077, 715219, 757748, 802806, 850544, 901120,
     954703, 1011473, 1071618, 1135340, 1202851, 1274376, 1350154, 1430438, 1515497,
     1605613, 1701088, 1802240, 1909406, 2022946, 2143236, 2270680, 2405702, 2548752,
     2700309, 2860877, 3030994, 3211226, 3402176, 3604479, 3818813, 4045892, 4286472,
     4541359, 4811404, 5097504, 5400618, 5721756, 6061988, 6422452, 6804352, 7208959,
     7637627, 8091785, 8572945, 9082719, 9622808, 10195009, 10801235, 11443507,
     12123974, 12844905, 13608704, 14417917, 15275252, 16183563, 17145888, 18165438,
     19245616, 20390018, 21602470, 22887014, 24247948, 25689810, 27217408, 28835834,
     30550514, 32367136, 34291776, 36330876, 38491212, 40780036, 43204940, 45774028,
     48495912, 51379620, 54434816, 57671668, 61101028, 64734272, 68583552, 72661752,
     76982424, 81560072, 86409880, 91548056, 96991792, 102759240, 108869632,
     115343336, 122202056, 129468544, 137167104, 145323504, 153964848, 163120144,
     172819760, 183096224, 193983648, 205518336, 217739200, 230686576, 244403840,
     258937008, 274334112, 290647008, 307929696, 326240288, 345639520, 366192448,
     387967040, 411036672, 435478400, 461373152, 488807680, 517874016, 548668224,
     581294016, 615859392, 652480576, 691279040, 732384896, 775934592, 822073344
    };

Q16n16  Q16n16_mtof(Q16n16 midival_fractional)
 {
     Q16n16 diff_fraction;
     uint8_t index = midival_fractional >> 16;
     uint16_t fraction = (uint16_t) midival_fractional; // keeps low word
     Q16n16 freq1 = (Q16n16) midiToFreq[index];
     Q16n16 freq2 = (Q16n16) midiToFreq[index+1];
     Q16n16 difference = freq2 - freq1;
     if (difference>=65536)
     {
         diff_fraction = ((difference>>8) * fraction) >> 8;
     }
     else
     {
         diff_fraction = (difference * fraction) >> 16;
     }
     return (Q16n16) (freq1+ diff_fraction);
 }

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */

xQueueHandle qSID;
xQueueHandle qPulse;
struct sid_f sid_frm;

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
            isr1_pulse.volume = isr_port_ptr[ch].volume;
            isr1_pulse.pw = interrupter.pw;
            xQueueSendFromISR(qPulse,&isr1_pulse,0);;
		}
		old_flag[ch] = flag[ch];
   
	}

    if(xQueueReceiveFromISR(qPulse,&isr1_pulse,0)){
        interrupter_oneshot(isr1_pulse.pw, isr1_pulse.volume);
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

#define SID_CHANNELS 3

CY_ISR(isr_sid) {
    
    static uint8_t cnt=0;
    if (cnt >212){
        cnt=0;
        if(xQueueReceiveFromISR(qSID,&sid_frm,0)){
            isr_port_ptr[0].halfcount = sid_frm.half[0];
            isr_port_ptr[1].halfcount = sid_frm.half[1];
            isr_port_ptr[2].halfcount = sid_frm.half[2];
            //isr_port_ptr[0].volume = sid_frm.gate[0]*127;
            //isr_port_ptr[1].volume = sid_frm.gate[1]*127;
            //isr_port_ptr[2].volume = sid_frm.gate[2]*127;
            sid_frm.pw[0]=sid_frm.pw[0]>>4;
            sid_frm.pw[1]=sid_frm.pw[1]>>4;
            sid_frm.pw[2]=sid_frm.pw[2]>>4;
            isr_port_ptr[0].volume = sid_frm.gate[0]* sid_frm.pw[0];
            isr_port_ptr[1].volume = sid_frm.gate[1]* sid_frm.pw[1];
            isr_port_ptr[2].volume = sid_frm.gate[2]* sid_frm.pw[2];
        }else{
            isr_port_ptr[0].volume = 0;
            isr_port_ptr[1].volume = 0;
            isr_port_ptr[2].volume = 0;
        }
    }else{
        cnt++;
    }
    
    uint16_t random = rand();
    random = random >>8;
    if(sid_frm.wave[0]){
        isr_port_ptr[0].halfcount = random;
    }
    if(sid_frm.wave[1]){
        isr_port_ptr[1].halfcount = random;
    }
    if(sid_frm.wave[2]){
        isr_port_ptr[2].halfcount = random;
    }
    
    
	uint8_t ch;
	uint8_t flag[SID_CHANNELS];
	static uint8_t old_flag[SID_CHANNELS];
	uint32 r = SG_Timer_ReadCounter();
	for (ch = 0; ch < SID_CHANNELS; ch++) {
		flag[ch] = 0;
		if (isr_port_ptr[ch].volume > 0) {
			if ((r / isr_port_ptr[ch].halfcount) % 2 > 0) {
				flag[ch] = 1;
			}
		}
		if (flag[ch] > old_flag[ch]) {
            isr1_pulse.volume = isr_port_ptr[ch].volume;
            isr1_pulse.pw = sid_frm.master_pw;
            xQueueSendFromISR(qPulse,&isr1_pulse,0);
		}
		old_flag[ch] = flag[ch];
   
	}

    if(xQueueReceiveFromISR(qPulse,&isr1_pulse,0)){
        interrupter_oneshot(isr1_pulse.pw, isr1_pulse.volume);
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

    if(xQueueReceiveFromISR(qPulse,&isr2_pulse,0)){
        interrupter_oneshot(isr2_pulse.pw, isr2_pulse.volume);
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

void update_midi_duty(){
    if(!telemetry.midi_voices) return;
    uint32_t dutycycle=0;

    for (uint8_t ch = 0; ch < N_CHANNEL; ch++) {    
        if (channel_ptr[ch].volume > 0){
            dutycycle+= ((uint32)channel_ptr[ch].volume*(uint32)param.pw)/(127000ul/(uint32)isr_port_ptr[ch].freq);
        }
	}
  
    telemetry.duty = dutycycle;
    if(dutycycle>configuration.max_tr_duty){
        interrupter.pw = (param.pw * configuration.max_tr_duty) / dutycycle;
    }else{
        interrupter.pw = param.pw;
    }
}
void reflect(PORT port[], CHANNEL channel[], MIDICH midich[]) {
	uint8_t ch;
	uint8_t mch;
    uint32_t dutycycle=0;
    uint32_t pb;
    telemetry.midi_voices =0;
	// Reflect the status of the updated tone generator channel & MIDI channel on the port
	for (ch = 0; ch < N_CHANNEL; ch++) {
		mch = channel[ch].midich;
		if (channel[ch].updated || midich[mch].updated) {
			if (channel[ch].volume > 0) {
                pb = ((((uint32_t)midich[mch].pitchbend*midich[mch].bendrange)<<10) / PITCHBEND_DIVIDER)<<6;
                port[ch].freq=Q16n16_mtof((channel[ch].miditone<<16)+pb);
                port[ch].halfcount= (150000<<14) / (port[ch].freq>>2);
                port[ch].freq = port[ch].freq >>16;

                
				if (ch < N_DISPCHANNEL)
					channel[ch].displayed = 1; // Re-display required
			}
			port[ch].volume = (int32)channel[ch].volume * midich[mch].expression / 127; // Reflect Expression here
			channel[ch].updated = 0;													// Mission channel update work done
		}
        
        if (channel[ch].volume > 0){
            telemetry.midi_voices++;
            dutycycle+= ((uint32)channel[ch].volume*(uint32)param.pw)/(127000ul/(uint32)port[ch].freq);
        }
	}
	for (mch = 0; mch < N_MIDICHANNEL; mch++) {
		midich[mch].updated = 0; // MIDI channel update work done
	}
    
    
    telemetry.duty = dutycycle;
    if(dutycycle>configuration.max_tr_duty){
        interrupter.pw = (param.pw * configuration.max_tr_duty) / dutycycle;
    }else{
        interrupter.pw = param.pw;
    }
     
}

void switch_synth(uint8_t synth){
    switch(synth){
        case SYNTH_OFF:
            isr_midi_Stop();
        break;
        case SYNTH_MIDI:
            isr_midi_Stop();
            isr_midi_StartEx(isr_midi);
        break;
        case SYNTH_SID:
            isr_midi_Stop();
            isr_midi_StartEx(isr_sid);
        break;
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
    qSID = xQueueCreate(32, sizeof(struct sid_f));
    qPulse = xQueueCreate(8, sizeof(struct pulse_str));

	NOTE note_struct;

	// Tone generator channel status (updated according to MIDI messages)
	CHANNEL channel[N_CHANNEL];
    channel_ptr=channel;

	// MIDI channel status
	MIDICH midich[N_MIDICHANNEL];
    midich_ptr=midich_ptr;

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

	//isr_midi_StartEx(isr_midi);
    isr_midi_StartEx(isr_sid);
    
	isr_interrupter_StartEx(isr_interrupter);

	/* `#END` */

	for (;;) {
		/* `#START TASK_LOOP_CODE` */

        if(param.synth==SYNTH_MIDI){
    		if (xQueueReceive(qMIDI_rx, &note_struct, portMAX_DELAY)) {
    			process(&note_struct, channel, midich);
    			while (xQueueReceive(qMIDI_rx, &note_struct, 0)) {
    				process(&note_struct, channel, midich);
    			}
    			reflect(port, channel, midich);
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
		xTaskCreate(tsk_midi_TaskProc, "MIDI-Svc", 160, NULL, PRIO_MIDI, &tsk_midi_TaskHandle);
		tsk_midi_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
