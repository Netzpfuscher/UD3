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
#include "tsk_fault.h"

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
#include "alarmevent.h"
#include <device.h>
#include <stdlib.h>

#define PITCHBEND_ZEROBIAS (0x2000)
#define PITCHBEND_DIVIDER ((uint32_t)0x1fff)

#define N_CHANNEL 4

#define N_MIDICHANNEL 16

#define COMMAND_NOTEONOFF 1
#define COMMAND_NOTEOFF 4
#define COMMAND_CONTROLCHANGE 2
#define COMMAND_PITCHBEND 3


const uint8_t kill_msg[3] = {0xb0, 0x77, 0x00};

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

typedef struct __pulse__ {
	uint8_t  volume;
	uint16_t pw;
} PULSE;



uint8_t skip_flag = 0; // Skipping system exclusive messages

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
} CHANNEL;

typedef struct __port__ {
	uint16 halfcount; // A count that determines the frequency of port (for 1/2 period)
	uint8 volume;	 // Volume of port (0 - 127)
    uint32 freq;
} PORT;

PORT *isr_port_ptr;
CHANNEL *channel_ptr;

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

void handle_qcw(){
	if ((ramp.modulation_value < 255) && (ramp.modulation_value > 0)) {
		ramp.modulation_value += param.qcw_ramp;
		if (ramp.modulation_value > 255)
			ramp.modulation_value = 255;
		ramp_control();
	}
}


CY_ISR(isr_midi) {
    if(qcw_reg){
        handle_qcw();
        return;
    }
    PULSE pulse;
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
            pulse.volume = isr_port_ptr[ch].volume;
            pulse.pw = interrupter.pw;
            xQueueSendFromISR(qPulse,&pulse,0);;
		}
		old_flag[ch] = flag[ch];
   
	}

    if(xQueueReceiveFromISR(qPulse,&pulse,0)){
        interrupter_oneshot(pulse.pw, pulse.volume);
    }

}

#define SID_CHANNELS 3

#define ADSR_IDLE    0
#define ADSR_ATTACK  1
#define ADSR_DECAY   2
#define ADSR_SUSTAIN 3
#define ADSR_RELEASE 4

static const uint8_t envelope[16] = {
    0,
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    8,
    20,
    41,
    67,
    83,
    251,
    255,
    255};

CY_ISR(isr_sid) {
    
    if(qcw_reg){
        handle_qcw();
        return;
    }
    
    telemetry.midi_voices=0;
    static uint8_t cnt=0;
    static uint8_t adsr_state[3]={ADSR_IDLE,ADSR_IDLE,ADSR_IDLE};
    static uint8_t old_gate[3]={0,0,0};
    static uint8_t adsr_cnt[3]={0,0,0};
    
    if (cnt >212){  // 50 Hz
        cnt=0;
        if(xQueueReceiveFromISR(qSID,&sid_frm,0)){
            
            for(uint8_t i=0;i<SID_CHANNELS;i++){
                isr_port_ptr[i].halfcount = sid_frm.half[i];
                if(sid_frm.gate[i] > old_gate[i]) adsr_state[i]=ADSR_ATTACK;  //Rising edge
                if(sid_frm.gate[i] < old_gate[i]) adsr_state[i]=ADSR_RELEASE;  //Falling edge
                //isr_port_ptr[i].volume=127*sid_frm.gate[i];
                sid_frm.pw[i]=sid_frm.pw[i]>>4;
                old_gate[i] = sid_frm.gate[i];
            }
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
    
    PULSE pulse;
	uint8_t ch;
	uint8_t flag[SID_CHANNELS];
	static uint8_t old_flag[SID_CHANNELS];
	uint32 r = SG_Timer_ReadCounter();
	for (ch = 0; ch < SID_CHANNELS; ch++) {
        switch (adsr_state[ch]){
            case ADSR_ATTACK:
                if(adsr_cnt[ch]>=envelope[sid_frm.attack[ch]]){
                    isr_port_ptr[ch].volume++;
                    if(isr_port_ptr[ch].volume>=127) adsr_state[ch]=ADSR_DECAY;
                    adsr_cnt[ch]=0;
                }else{
                    adsr_cnt[ch]++;
                }
            break;
            case ADSR_DECAY:
                if(adsr_cnt[ch]>=envelope[sid_frm.decay[ch]]){
                    isr_port_ptr[ch].volume--;
                    if(isr_port_ptr[ch].volume<=sid_frm.sustain[ch]) adsr_state[ch]=ADSR_SUSTAIN;
                    adsr_cnt[ch]=0;
                }else{
                    adsr_cnt[ch]++;
                }
            break;
            case ADSR_SUSTAIN:
                isr_port_ptr[ch].volume = sid_frm.sustain[ch];
            break;
            case ADSR_RELEASE:
                if(adsr_cnt[ch]>=envelope[sid_frm.release[ch]]){
                    isr_port_ptr[ch].volume--;
                    if(isr_port_ptr[ch].volume==0 || isr_port_ptr[ch].volume>127) adsr_state[ch]=ADSR_IDLE;
                    adsr_cnt[ch]=0;
                }else{
                    adsr_cnt[ch]++;
                }
            break;
        }
        
        
        
		flag[ch] = 0;
		if (isr_port_ptr[ch].volume > 0) {
            telemetry.midi_voices++;
			if ((r / isr_port_ptr[ch].halfcount) % 2 > 0) {
				flag[ch] = 1;
			}
		}
		if (flag[ch] > old_flag[ch]) {
            pulse.volume = isr_port_ptr[ch].volume;
            pulse.pw = sid_frm.master_pw;
            //pulse.pw = isr_port_ptr[ch].volume;
            xQueueSendFromISR(qPulse,&pulse,0);
		}
		old_flag[ch] = flag[ch];
   
        
	}

    if(xQueueReceiveFromISR(qPulse,&pulse,0)){
        interrupter_oneshot(pulse.pw, pulse.volume);
    }
 
   
}

CY_ISR(isr_interrupter) {
    PULSE pulse;
    if(xQueueReceiveFromISR(qPulse,&pulse,0)){
        interrupter_oneshot(pulse.pw, pulse.volume);
    }
	Offtime_ReadStatusRegister();
}

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
			}
		} else {								 // Note OFF
			for (ch = 0; ch < N_CHANNEL; ch++) { // Search for ports that are already ringing with the same MIDI channel & node number
				if (channel[ch].volume > 0 && channel[ch].midich == v->midich && channel[ch].miditone == v->data.noteonoff.tone)
					break;
			}
			if (ch < N_CHANNEL) { // A port was found

				channel[ch].volume = 0;
				channel[ch].updated = 1; // This port has been updated
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
			}
			break;
		case 0x7B: //Kill all notes on channel
			for (ch = 0; ch < N_CHANNEL; ch++) {
				if (channel[ch].volume > 0 && channel[ch].midich == v->midich) {
					channel[ch].volume = 0;
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
    if(!telemetry.midi_voices) return;
    uint32_t dutycycle=0;

    for (uint8_t ch = 0; ch < N_CHANNEL; ch++) {    
        if (channel_ptr[ch].volume > 0){
            dutycycle+= ((uint32)channel_ptr[ch].volume*(uint32)param.pw)/(127000ul/(uint32)isr_port_ptr[ch].freq);
        }
	}
  
    telemetry.duty = dutycycle;
    if(dutycycle>(configuration.max_tr_duty-param.temp_duty)){
        interrupter.pw = (param.pw * (configuration.max_tr_duty-param.temp_duty)) / dutycycle;
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
    if(dutycycle>(configuration.max_tr_duty-param.temp_duty)){
        interrupter.pw = (param.pw * (configuration.max_tr_duty-param.temp_duty)) / dutycycle;
    }else{
        interrupter.pw = param.pw;
    }
     
}

void kill_accu(){
    if(isr_port_ptr==NULL) return;
    for (uint8_t ch = 0; ch < N_CHANNEL; ch++) {
                isr_port_ptr[ch].volume=0;
                isr_port_ptr[ch].freq=0;
                isr_port_ptr[ch].halfcount=0;

	}
}

void switch_synth(uint8_t synth){
    skip_flag=0;
    telemetry.midi_voices=0;
    switch(synth){
        case SYNTH_OFF:
            isr_midi_Stop();
            xQueueReset(qMIDI_rx);
            xQueueReset(qSID);
            kill_accu();
        break;
        case SYNTH_MIDI:
            isr_midi_Stop();
            xQueueReset(qMIDI_rx);
            xQueueReset(qSID);
            kill_accu();
            isr_midi_StartEx(isr_midi);
        break;
        case SYNTH_SID:
            isr_midi_Stop();
            xQueueReset(qMIDI_rx);
            xQueueReset(qSID);
            kill_accu();
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
    qSID = xQueueCreate(64, sizeof(struct sid_f));
    qPulse = xQueueCreate(8, sizeof(PULSE));

	NOTE note_struct;

	// Tone generator channel status (updated according to MIDI messages)
	CHANNEL channel[N_CHANNEL];
    channel_ptr=channel;

	// MIDI channel status
	MIDICH midich[N_MIDICHANNEL];

	// Port status (updated at regular time intervals according to the status of the sound source channel)
	PORT port[N_CHANNEL];
	isr_port_ptr = port;

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
    //isr_midi_StartEx(isr_sid);
    
	isr_interrupter_StartEx(isr_interrupter);

	/* `#END` */
    alarm_push(ALM_PRIO_INFO,warn_task_midi, ALM_NO_VALUE);
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
		xTaskCreate(tsk_midi_TaskProc, "MIDI-Svc", STACK_MIDI, NULL, PRIO_MIDI, &tsk_midi_TaskHandle);
		tsk_midi_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
