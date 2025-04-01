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
#include "NoteMapper.h"
#include "VMSWrapper.h"

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
#include "tsk_cli.h"
#include "MidiProcessor.h"

xQueueHandle qMIDI_rx;

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */


uint8_t CMD_midi_inject(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args) {
    
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf("midi [event] [note] [velocity]\r\n");
        ttprintf("midi kill\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    }
    if(argCount==1){
        if(strcmp(args[0], "kill") == 0){  
            tsk_midi_kill();
            ttprintf("Kill\r\n");
        }
    }
    
    if(argCount==3){
        uint8_t cmd = 0;
        uint8_t note = 0;
        uint8_t vel = 0;
        if(strcmp(args[0], "on") == 0){
            cmd = MIDI_CMD_NOTE_ON;
        }else if(strcmp(args[0], "off") == 0){
            cmd = MIDI_CMD_NOTE_OFF;
        }
        note = atoi(args[1]);
        vel = atoi(args[2]);
        if(note > 127) note = 127;
        if(vel > 127) vel = 127;
        
        uint8_t msg[MIDI_MSG_SIZE];
        msg[0] = cmd;
        msg[1] = note;
        msg[2] = vel;
        msg[3] = 0x00;
        queue_midi_message(msg);
        ttprintf("Queued a '%s'-event Note: %u Vel: %u\r\n" , args[0], note, vel);
    }
    
    
    
    
    return TERM_CMD_EXIT_SUCCESS;
}


void tsk_midi_kill(){
    xQueueReset(qMIDI_rx);
    MidiProcessor_resetMidi();
    vTaskDelay(20);
    MidiProcessor_resetMidi();
}


void queue_midi_message(uint8 *midiMsg) {
    uint8_t ret = xQueueSend(qMIDI_rx, midiMsg,MIDI_MSG_SIZE);
	if(ret==errQUEUE_FULL){
        alarm_push(ALM_PRIO_WARN, "COM: MIDI buffer overrun",ALM_NO_VALUE);
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
	qMIDI_rx = xQueueCreate(N_QUEUE_MIDI, MIDI_MSG_SIZE);
    
	uint8_t msg[MIDI_MSG_SIZE];
    msg[0] = 0;
    
	/* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */

    MidiProcessor_init();
    Mapper_init();
    VMSW_init();
   
       
		/* `#END` */
    alarm_push(ALM_PRIO_INFO, "TASK: MIDI started", ALM_NO_VALUE);
	for (;;) {
		/* `#START TASK_LOOP_CODE` */

        if(param.synth==SYNTH_MIDI){
            
    		while (xQueueReceive(qMIDI_rx, msg, 1)) {

                uint8_t cable = 0;
                uint8_t channel = msg[0] & 0xf;
                uint8_t cmd = msg[0] & 0xf0;
                uint8_t param1 = msg[1];
                uint8_t param2 = msg[2];

                if(cmd == 0) break;

                MidiProcessor_processCmd(cable, channel, cmd, param1, param2);
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
