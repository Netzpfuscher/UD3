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
#include "clock.h"

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
#include "ntlibc.h"
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

enum ADSR{
    ADSR_IDLE = 0,
    ADSR_PENDING,
    ADSR_ATTACK,
    ADSR_DECAY,
    ADSR_SUSTAIN,
    ADSR_RELEASE,
};

struct _filter filter;

/*****************************************************************************
* Switches the synthesizer
******************************************************************************/
uint8_t callback_SynthFunction(parameter_entry * params, uint8_t index, port_str *ptr){
    switch_synth(param.synth);
    return 1;
}

uint8_t callback_synthFilter(parameter_entry * params, uint8_t index, port_str *ptr){
    uint8_t cnt=0;
    uint16_t number=0;
    char substring[20];
    uint8_t str_size = ntlibc_strlen(configuration.synth_filter);
    uint8_t flag=0;
    
    filter.min=0;
    filter.max=20000;
    if(configuration.synth_filter[0]=='\0'){  //No filter
        for(uint8_t i=0;i<16;i++){
            filter.channel[i]=pdTRUE;
        }
        return 1;
    }else{
        for(uint8_t i=0;i<16;i++){
            filter.channel[i]=pdFALSE;
        }
    }
    
    for(uint8_t i=0;i<str_size;i++){
        if(configuration.synth_filter[i]=='\0') break;
        
        if(configuration.synth_filter[i]=='c'){
            cnt=0;
            substring[0]='\0';
            for(uint8_t w=i+1;w<str_size;w++){
                if(ntlibc_isdigit(configuration.synth_filter[w])){
                    substring[cnt]=configuration.synth_filter[w];
                    cnt++;
                }else{
                    substring[cnt]='\0';
                    break;
                }
            }
            if(substring[0]){
                number = ntlibc_atoi(substring);
                if(number<16){
                    filter.channel[number]=pdTRUE;
                    flag=1;
                }
            }  
        }else if(configuration.synth_filter[i]=='f'){
            if(configuration.synth_filter[i+1]=='>' || configuration.synth_filter[i+1]=='<'){
                cnt=0;
                for(uint8_t w=i+2;w<str_size;w++){
                    if(ntlibc_isdigit(configuration.synth_filter[w])){
                        substring[cnt]=configuration.synth_filter[w];
                        cnt++;
                    }else{
                        substring[cnt]='\0';
                        break;
                    }
                }
                if(cnt){
                    number = ntlibc_atoi(substring);
                    if(number<20000){
                        if(configuration.synth_filter[i+1]=='>') filter.max = number;
                        if(configuration.synth_filter[i+1]=='<') filter.min = number;
                    }
                } 
            }
        }     
    }
    if(filter.min > filter.max){
        SEND_CONST_STRING("Error: Min frequency ist greater than max\r\n",ptr);
    }
    if(!flag){
        for(uint8_t i=0;i<16;i++){
            filter.channel[i]=pdTRUE;
        }   
    }
    uint8_t ret;
    char buf[50];
    for(uint8_t i=0;i<sizeof(filter.channel);i++){
        ret= snprintf(buf,sizeof(buf),"Channel %u: %u\r\n",i,filter.channel[i]);
        send_buffer(buf,ret,ptr);
    }
    ret= snprintf(buf,sizeof(buf),"Min frequency: %u\r\n",filter.min);
    send_buffer(buf,ret,ptr);
    ret= snprintf(buf,sizeof(buf),"Max frequency: %u\r\n",filter.max);
    send_buffer(buf,ret,ptr);

    return 1;
}

void reflect();

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


// Tone generator channel status (updated according to MIDI messages)
CHANNEL channel[N_CHANNEL];

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


uint8_t old_flag[N_CHANNEL];


static const uint8_t envelope[16] = {0,0,1,2,3,4,5,6,8,20,41,67,83,251,255,255};
volatile uint8_t pulse_active=pdFALSE;

static inline void compute_adsr_midi(uint8_t ch){
	switch (channel[ch].adsr_state){
        case ADSR_ATTACK:
            if(channel[ch].adsr_count>=midich[channel[ch].midich].attack){
                channel[ch].volume++;
                if(channel[ch].volume>=127){
                    channel[ch].volume=127;
                    channel[ch].adsr_state=ADSR_DECAY;
                }
                channel[ch].adsr_count=0;
            }else{
                channel[ch].adsr_count++;
            }
        break;
        case ADSR_DECAY:
            if(channel[ch].adsr_count>=midich[channel[ch].midich].decay){
                channel[ch].volume--;
                if(channel[ch].volume<=channel[ch].sustain) channel[ch].adsr_state=ADSR_SUSTAIN;
                channel[ch].adsr_count=0;
            }else{
                channel[ch].adsr_count++;
            }
        break;
        case ADSR_SUSTAIN:
            channel[ch].volume = channel[ch].sustain;
        break;
        case ADSR_RELEASE:
            if(channel[ch].adsr_count>=midich[channel[ch].midich].release){
                channel[ch].volume--;
                if(channel[ch].volume==0) {
                    channel[ch].sustain=0;
                    channel[ch].adsr_state=ADSR_IDLE;
                }
                channel[ch].adsr_count=0;
            }else{
                channel[ch].adsr_count++;
            }
        break;
    }
}

static inline void compute_adsr_sid(uint8_t ch){
    switch (channel[ch].adsr_state){
            case ADSR_ATTACK:
                if(channel[ch].adsr_count>=envelope[sid_frm.attack[ch]]){
                    channel[ch].volume++;
                    if(channel[ch].volume>=127){
                        channel[ch].volume=127;
                        channel[ch].adsr_state=ADSR_DECAY;
                    }
                    channel[ch].adsr_count=0;
                }else{
                    channel[ch].adsr_count++;
                }
            break;
            case ADSR_DECAY:
                if(channel[ch].adsr_count>=envelope[sid_frm.decay[ch]]){
                    channel[ch].volume--;
                    if(channel[ch].volume<=sid_frm.sustain[ch]) channel[ch].adsr_state=ADSR_SUSTAIN;
                    channel[ch].adsr_count=0;
                }else{
                    channel[ch].adsr_count++;
                }
            break;
            case ADSR_SUSTAIN:
                channel[ch].volume = sid_frm.sustain[ch];
            break;
            case ADSR_RELEASE:
                if(channel[ch].adsr_count>=envelope[sid_frm.release[ch]]){
                    channel[ch].volume--;
                    if(channel[ch].volume==0) {
                        channel[ch].adsr_state=ADSR_IDLE;
                    }
                    channel[ch].adsr_count=0;
                }else{
                    channel[ch].adsr_count++;
                }
            break;
        }
}

static inline void synthcode_MIDI(uint32_t r){
    PULSE pulse;
	uint8_t flag[N_CHANNEL];
    tt.n.midi_voices.value=0;
    
	for (uint8_t ch = 0; ch < N_CHANNEL; ch++) {

        compute_adsr_midi(ch);
        
        flag[ch] = 0;
		if (channel[ch].volume > 0) {
            tt.n.midi_voices.value++;
			if ((r / channel[ch].halfcount) % 2 > 0) {
				flag[ch] = 1;
			}
		}
		if (flag[ch] > old_flag[ch]) {
            pulse.volume = channel[ch].volume;
            pulse.pw = interrupter.pw;
#if USE_DEBUG_PW           
            pulse.pw = channel[ch].volume;
#endif
            xQueueSendFromISR(qPulse,&pulse,0);
		}
		old_flag[ch] = flag[ch];
   
	}
    if(itr_end_Read()==0){
        if(xQueueReceiveFromISR(qPulse,&pulse,0)){
            interrupter_oneshot(pulse.pw, pulse.volume);
        }  
    }
}

uint32_t volatile next_frame=4294967295;
uint32_t last_frame=4294967295;

static inline void synthcode_SID(uint32_t r){
  
    tt.n.midi_voices.value=0;
    
    if (l_time > next_frame){
        if(xQueueReceiveFromISR(qSID,&sid_frm,0)){
            last_frame=l_time;
            for(uint8_t i=0;i<SID_CHANNELS;i++){
                channel[i].halfcount = sid_frm.half[i];
                if(sid_frm.gate[i] > channel[i].old_gate) channel[i].adsr_state=ADSR_ATTACK;  //Rising edge
                if(sid_frm.gate[i] < channel[i].old_gate) channel[i].adsr_state=ADSR_RELEASE;  //Falling edge
                sid_frm.pw[i]=sid_frm.pw[i]>>4;
                channel[i].freq = sid_frm.freq[i];
                channel[i].old_gate = sid_frm.gate[i];
                channel[i].freq = sid_frm.freq[i];
            }
            next_frame = sid_frm.next_frame;
        }
    }
    if((l_time - last_frame)>200){
     next_frame = l_time;
     last_frame = l_time;
        for (uint8_t i = 0;i<SID_CHANNELS;++i) {
            channel[i].volume = 0;
            channel[i].adsr_state = ADSR_IDLE;
            channel[i].old_gate=0;
        }   
    }
        
    uint8_t random = rand();

    PULSE pulse;
	uint8_t flag[SID_CHANNELS];

	for (uint8_t ch = 0; ch < SID_CHANNELS; ch++) {
        compute_adsr_sid(ch);

		flag[ch] = 0;
		if (channel[ch].volume > 0) {
            tt.n.midi_voices.value++;
			if ((r / channel[ch].halfcount) % 2 > 0) {
				flag[ch] = 1; 
			}
		}
		if (flag[ch] > old_flag[ch]) {
            if(!(sid_frm.wave[ch] && (random & SID_NOISE_WEIGHT))){
                pulse.volume = channel[ch].volume;
                pulse.pw = sid_frm.master_pw;
#if USE_DEBUG_PW
                pulse.pw = channel[ch].volume;   //For DEBUG with speaker
#endif
                xQueueSendFromISR(qPulse,&pulse,0);
            }
		}   
		old_flag[ch] = flag[ch];   
	}
    if(itr_end_Read()==0){
        if(xQueueReceiveFromISR(qPulse,&pulse,0)){
            pulse_active=pdTRUE;
            interrupter_oneshot(pulse.pw, pulse.volume);
        } 
    }
}

static inline void synthcode_QMIDI(uint32_t r){
    qcw_handle_synth();
    int16_t vol=0;
    tt.n.midi_voices.value=0;
	for (uint8_t ch = 0; ch < N_CHANNEL; ch++) {

        compute_adsr_midi(ch);

		if (channel[ch].volume > 0) {
            tt.n.midi_voices.value++;
			if ((r / channel[ch].halfcount) % 2 > 0) {
                vol +=channel[ch].volume;
			}else{
                vol -=channel[ch].volume;
            }
		}
	}
    if(vol>127)vol=127;
    if(vol<-128)vol=-128;
    qcw_modulate(vol+0x80);  
}

static inline void synthcode_QSID(uint32_t r){
    qcw_handle_synth();
    
    tt.n.midi_voices.value=0;
    
    if (l_time > next_frame){
        if(xQueueReceiveFromISR(qSID,&sid_frm,0)){
            last_frame=l_time;
            for(uint8_t i=0;i<SID_CHANNELS;i++){
                channel[i].halfcount = sid_frm.half[i];
                if(sid_frm.gate[i] > channel[i].old_gate){
                    channel[i].adsr_state=ADSR_ATTACK;  //Rising edge
                    if(!QCW_enable_Control) qcw_start();
                }
                if(sid_frm.gate[i] < channel[i].old_gate) channel[i].adsr_state=ADSR_RELEASE;  //Falling edge
                sid_frm.pw[i]=sid_frm.pw[i]>>4;
                channel[i].freq = sid_frm.freq[i];
                channel[i].old_gate = sid_frm.gate[i];
                channel[i].freq = sid_frm.freq[i];
            }
            next_frame = sid_frm.next_frame;
        }
    }
    if((l_time - last_frame)>200){
     next_frame = l_time;
     last_frame = l_time;
        for (uint8_t i = 0;i<SID_CHANNELS;++i) {
            channel[i].volume = 0;
            channel[i].adsr_state = ADSR_IDLE;
            channel[i].old_gate=0;
        }   
    }
        
    
    int8_t random = ((uint8_t)rand())-127;
    
    int16_t vol=0;
	
	for (uint8_t ch = 0; ch < SID_CHANNELS; ch++) {
        compute_adsr_sid(ch);

		if (channel[ch].volume > 0) {
            tt.n.midi_voices.value++;
			if ((r / channel[ch].halfcount) % 2 > 0) {
                if(sid_frm.wave[ch]){
                    vol +=(((int)channel[ch].volume*random)/127);
                }else{
                    vol +=channel[ch].volume;
                }
			}else{
                if(!sid_frm.wave[ch]){
                    vol -=channel[ch].volume;
                }
            }
		}   
	}
    vol/=3;
    if(vol>127)vol=127;
    if(vol<-128)vol=-128;
    qcw_modulate(vol+0x80);    
}



CY_ISR(isr_synth) {
    uint32 r = SG_Timer_ReadCounter();
    clock_tick();
    if(qcw_reg){
        qcw_handle();
        return;
    }
    switch(param.synth){
        case SYNTH_MIDI:
            synthcode_MIDI(r);
            break;
        case SYNTH_SID:
            synthcode_SID(r);
            break;
        case SYNTH_MIDI_QCW:
            synthcode_QMIDI(r);
            break;
        case SYNTH_SID_QCW:
            synthcode_QSID(r);
            break;
        default:
            break;
    }
}


CY_ISR(isr_interrupter) { 
    PULSE pulse;
    if(xQueueReceiveFromISR(qPulse,&pulse,0)){
        interrupter_oneshot(pulse.pw, pulse.volume);
    }
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
            dutycycle+= ((uint32)channel[ch].volume*(uint32)param.pw)/(127000ul/(uint32)channel[ch].freq);
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

void kill_accu(){
    for (uint8_t ch = 0; ch < N_CHANNEL; ch++) {
                channel[ch].volume=0;
                channel[ch].freq=0;
                channel[ch].halfcount=0;

	}
}

void switch_synth(uint8_t synth){
    skip_flag=0;
    tt.n.midi_voices.value=0;
    xQueueReset(qMIDI_rx);
    xQueueReset(qSID);
    kill_accu();   
}

uint8_t CMD_SynthMon(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    char buf[80];
    uint8_t ret;
    uint32_t freq=0;
    uint8_t channels=N_CHANNEL;
    port_str * ptr = handle->port;
    
    Term_Disable_Cursor(ptr);
    Term_Erase_Screen(ptr);
    SEND_CONST_STRING("Synthesizer monitor    [CTRL+C] for quit\r\n",ptr);
    SEND_CONST_STRING("-----------------------------------------------------------\r\n",ptr);
    while(Term_check_break(ptr,100)){
        Term_Move_Cursor(3,1,ptr);
        if(param.synth == SYNTH_SID || param.synth == SYNTH_SID_QCW) channels=SID_CHANNELS;
        if(param.synth == SYNTH_MIDI || param.synth == SYNTH_MIDI_QCW) channels=N_CHANNEL;
        
        for(uint8_t i=0;i<channels;i++){
            ret=sprintf(buf,"Ch:   Freq:      \r");
            send_buffer(buf,ret,ptr);   
            if(channel[i].volume>0){
                freq=channel[i].freq;
            }else{
                freq=0;
            }
            ret=sprintf(buf,"Ch: %u Freq: %u",i+1,freq);
            send_buffer(buf,ret,ptr);                 
            Term_Move_Cursor_right(20,ptr);
            ret=sprintf(buf,"Vol: ",i+1);
            send_buffer(buf,ret,ptr);
            uint8_t cnt = channel[i].volume/12;

            for(uint8_t w=0;w<10;w++){
                if(w<cnt){
                    send_char('o',ptr);
                }else{
                    send_char(' ',ptr);
                }
            }
            SEND_CONST_STRING("\r\n",ptr);
        }
    }
    SEND_CONST_STRING("\r\n",ptr);
    Term_Enable_Cursor(ptr);
    return TERM_CMD_EXIT_SUCCESS;
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
    qSID = xQueueCreate(N_QUEUE_SID, sizeof(struct sid_f));
    qPulse = xQueueCreate(N_QUEUE_PULSE, sizeof(PULSE));

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


	isr_midi_StartEx(isr_synth);
    //isr_midi_StartEx(isr_sid);
    
	isr_interrupter_StartEx(isr_interrupter);
    isr_interrupter_Enable();

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
