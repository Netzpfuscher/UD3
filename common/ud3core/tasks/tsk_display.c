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

#include "tsk_display.h"
#include "tsk_midi.h"

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"


xTaskHandle tsk_display_TaskHandle;
uint8 tsk_display_initVar = 0u;





/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */
#include "cli_common.h"
#include "telemetry.h"
#include "tsk_priority.h"
#include "tsk_analog.h"

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */

#define DISPLAY_BUS_STATE   0
#define DISPLAY_SYNTH_STATE 1
#define DISPLAY_SYNTHMON    2

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * This is the main procedure that comprises the task.  Place the code required
 * to preform the desired function within the merge regions of the task procedure
 * to add functionality to the task.
 */
void display_bus_state(){
    Disp_MemClear(0);
    switch(tt.n.bus_status.value){
        case BUS_OFF:
            if(tt.n.bus_v.value<40){
                Disp_DrawRect(3,0,4,4,1,Disp_GREEN);        
                Disp_DrawRect(3,6,4,7,1,Disp_GREEN);
            }
        break;
        case BUS_TEMP1_FAULT:
            Disp_DrawLine(0,0,7,7,Disp_RED);
            Disp_DrawLine(7,0,0,7,Disp_RED);
    		break;    
        default:
            Disp_DrawRect(3,0,4,4,1,Disp_RED);        
            Disp_DrawRect(3,6,4,7,1,Disp_RED);
        break;
        
    }
    Disp_Trigger(1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

uint8_t display_text(char *text,uint16_t len,int32_t *memory){
    Disp_MemClear(0);
    Disp_PrintString(*memory,0,text,Disp_WHITE,Disp_BLACK);
    *memory = *memory -1;
    
    Disp_Trigger(1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    if(*memory<(-1*(len+1)*5)){
        *memory = 8;
        return pdFALSE;
    }
    return pdTRUE;
}

void display_synthmon(){
    Disp_MemClear(0);
    uint8_t freq[8];
    for(uint8_t i=0; i<8;i++){
        freq[i]=0;
    }
    
    for(uint8_t i=0; i<N_CHANNEL;i++){
        if(channel[i].volume>0){
            if(channel[i].freq > 0 && channel[i].freq <187){
                freq[0]=freq[0]+channel[i].volume;
                if(freq[0]>127)freq[0]=127;
            }
            if(channel[i].freq > 186 && channel[i].freq <375){
                freq[1]=freq[1]+channel[i].volume;
                if(freq[1]>127)freq[1]=127;
            }
            if(channel[i].freq > 374 && channel[i].freq <561){
                freq[2]=freq[2]+channel[i].volume;
                if(freq[2]>127)freq[2]=127;
            }
            if(channel[i].freq > 560 && channel[i].freq <748){
                freq[3]=freq[3]+channel[i].volume;
                if(freq[3]>127)freq[3]=127;
            }
            if(channel[i].freq > 747 && channel[i].freq <935){
                freq[4]=freq[4]+channel[i].volume;
                if(freq[4]>127)freq[4]=127;
            }
            if(channel[i].freq > 934 && channel[i].freq <1122){
                freq[5]=freq[5]+channel[i].volume;
                if(freq[5]>127)freq[5]=127;
            }
            if(channel[i].freq > 1121 && channel[i].freq <1309){
                freq[6]=freq[6]+channel[i].volume;
                if(freq[6]>127)freq[6]=127;
            }
            if(channel[i].freq > 1308){
                freq[7]=freq[7]+channel[i].volume;
                if(freq[7]>127)freq[7]=127;
            }
        }   
    }
    for(uint8_t i=0; i<8;i++){
        freq[i]=freq[i]/15;
        Disp_DrawLine(i,7,i,7-freq[i],Disp_BLUE);
    }
    
    Disp_Trigger(1);
    vTaskDelay(20 / portTICK_PERIOD_MS);
}

uint8_t display_syntstate(int32_t *memory){
    switch(param.synth){
                case SYNTH_OFF:
                    return display_text("OFF",3,memory);
                    break;
                case SYNTH_MIDI:
                    return display_text("MIDI",4,memory);
                    break;
                case SYNTH_SID:
                    return display_text("SID",3,memory);
                    break;
            }
    return 0;
}

void tsk_display_TaskProc(void *pvParameters) {
	/*
	 * Add and initialize local variables that are allocated on the Task stack
	 * the the section below.
	 */
	/* `#START TASK_VARIABLES` */

	/* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */
    
    Disp_Start();
    Disp_MemClear(0);
    Disp_Dim(configuration.enable_display-1);
    Disp_Trigger(1);
    //param.display = 1;
    vTaskDelay(250 / portTICK_PERIOD_MS);
    
    
    int32_t memory =0;
    uint8_t old_state=param.synth;
    
    while(display_text("UD3 Starting...",15,&memory));
    
	/* `#END` */

	for (;;) {
		/* `#START TASK_LOOP_CODE` */
        if(old_state!=param.synth){
            param.display=2;
        }
        switch(param.display){
        case DISPLAY_BUS_STATE:
            display_bus_state();
            break;
        case DISPLAY_SYNTH_STATE:
            display_syntstate(&memory);
            break;
        case DISPLAY_SYNTHMON:
            if(old_state!=param.synth){
                while(display_syntstate(&memory));
                old_state=param.synth;
                if(param.synth==SYNTH_OFF) param.display=0;
            }
            display_synthmon();
            break;
        default:
            vTaskDelay(500 / portTICK_PERIOD_MS);
            break;
            
        }
        
        
        
		/* `#END` */
	}
}
/* ------------------------------------------------------------------------ */
void tsk_display_Start(void) {
	/*
	 * Insert task global memeory initialization here. Since the OS does not
	 * initialize ANY global memory, execute the initialization here to make
	 * sure that your task data is properly 
	 */
	/* `#START TASK_GLOBAL_INIT` */
    
    

	/* `#END` */

	if (tsk_display_initVar != 1) {

		/*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_display_TaskProc, "Display-Svc", STACK_DISPLAY, NULL, PRIO_DISPLAY, &tsk_display_TaskHandle);
		tsk_display_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
