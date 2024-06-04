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

#include "tsk_sid.h"
#include "tsk_fault.h"
#include "clock.h"
#include "SignalGenerator.h"
#include "MidiProcessor.h"
#include "NoteMapper.h"
#include "VMSWrapper.h"

xTaskHandle tsk_sid_TaskHandle;
uint8 tsk_sid_initVar = 0u;


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
#include "SidProcessor.h"

xQueueHandle qSID;

uint32_t volatile next_frame=4294967295;
uint32_t last_frame=4294967295;

static SIDFrame_t currentFrame;

void tsk_sid_TaskProc(void *pvParameters) {

    alarm_push(ALM_PRIO_INFO, "TASK: SID started", ALM_NO_VALUE);
	for (;;) {
        //save some cpu cycles if not in use 
        if(param.synth != SYNTH_SID){
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        
        //did the time come to recive a new frame from the queue? (l_time is system millisecond time)
        if (l_time > next_frame){
            //yes => do so
            if(xQueueReceive(qSID, &currentFrame, 0)){
                //when do we need to do this again next time?
                last_frame=l_time;
                next_frame = currentFrame.next_frame;
                
                //send the frame to the processor
                SidProcessor_handleFrame(&currentFrame);
            }
        }
        
        //did the last frame expire a long time ago without us getting a new one? If so kill the output
        if((l_time - last_frame)>100){
            next_frame = l_time;
            last_frame = l_time;
            SidProcessor_handleFrame(NULL);  
        }
        
        //also calculate ADSR
        SidProcessor_runADSR();
        
        vTaskDelay(pdMS_TO_TICKS(1));
	}
}
/* ------------------------------------------------------------------------ */
void tsk_sid_Start(void) {
	/*
	 * Insert task global memeory initialization here. Since the OS does not
	 * initialize ANY global memory, execute the initialization here to make
	 * sure that your task data is properly 
	 */
	/* `#START TASK_GLOBAL_INIT` */

	/* `#END` */

	if (tsk_sid_initVar != 1) {

        //create queue that stores all sid frames after they were received by the min task
        qSID = xQueueCreate(N_QUEUE_SID, sizeof(SIDFrame_t));
        /*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_sid_TaskProc, "SID-Svc", STACK_MIDI, NULL, PRIO_MIDI, &tsk_sid_TaskHandle);
		tsk_sid_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
