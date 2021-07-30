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
#include "MidiController.h"
#include "ADSREngine.h"

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


void reflect();

const uint8_t kill_msg[3] = {0xb0, 0x77, 0x00};
xQueueHandle qMIDI_rx;


/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */




void USBMIDI_1_callbackLocalMidiEvent(uint8 cable, uint8 *midiMsg) {
    uint8_t ret=pdTRUE;
    ret = xQueueSendFromISR(qMIDI_rx, midiMsg, NULL);
	if(ret==errQUEUE_FULL){
        alarm_push(ALM_PRIO_WARN,warn_midi_overrun,ALM_NO_VALUE);
    }
}


void update_midi_duty(){
    /*
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
    }*/
}
/*
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
    
}*/



/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * This is the main procedure that comprises the task.  Place the code required
 * to preform the desired function within the merge regions of the task procedure
 * to add functionality to the task.
 */

#define MIDI_MSG_SIZE 3

void tsk_midi_TaskProc(void *pvParameters) {
	/*
	 * Add and initialize local variables that are allocated on the Task stack
	 * the the section below.
	 */
	/* `#START TASK_VARIABLES` */
	qMIDI_rx = xQueueCreate(N_QUEUE_MIDI, MIDI_MSG_SIZE);
    
	uint8_t msg[MIDI_MSG_SIZE+1];
    msg[0] = 0;
    
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


    SigGen_init();
    VMS_init();
    Midi_init();
    Midi_setEnabled(1);
    
       
		/* `#END` */
    alarm_push(ALM_PRIO_INFO,warn_task_midi, ALM_NO_VALUE);
	for (;;) {
		/* `#START TASK_LOOP_CODE` */

        if(param.synth==SYNTH_MIDI ||param.synth==SYNTH_MIDI_QCW){
            
    		if (xQueueReceive(qMIDI_rx, msg+1, 1)) { //+1 copy only midi msg not midi-cable (byte 0)
    			Midi_run(msg);
    			while (xQueueReceive(qMIDI_rx, msg+1, 0)) { //+1 copy only midi msg not midi-cable (byte 0)
    				Midi_run(msg);
    			}
    		}
            VMS_run();
            
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
