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

#include "tsk_fault.h"
#include "tsk_midi.h"
#include "alarmevent.h"
#include "cli_basic.h"


/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "ZCDtoPWM.h"

xTaskHandle tsk_fault_TaskHandle;
uint8 tsk_fault_initVar = 0u;

#define FAULT_LOOP_SPEED_MS 50
#define FB_ERROR_TIME 1000 / FAULT_LOOP_SPEED_MS


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

TimerHandle_t xWD_Timer;
SYSFAULT sysfault;
uint32_t switch_without_fb = 0;


void WD_enable(uint8_t enable){
    if(xWD_Timer!=NULL){
        if(enable){
            xTimerStart(xWD_Timer,10);
        }else{  
            xTimerStop(xWD_Timer,10);
        }
    } 
}

void WD_reset(){
    if(xWD_Timer!=NULL && configuration.watchdog){
        xTimerStart(xWD_Timer,0);
    }
}

void WD_reset_from_ISR(){
    if(xWD_Timer!=NULL && configuration.watchdog){
        xTimerStartFromISR(xWD_Timer, NULL);
    }
}

void set_switch_without_fb(uint32_t en){
    switch_without_fb = en ? 0 : 0b10;
}

void handle_UVLO(void) {
    // read the driver voltage
    tt.n.driver_v.value = read_driver_mv();
    
    uint8_t undervoltage = pdFALSE;
    
    if(configuration.uvlo_analog){  //Analog UVLO
        if(tt.n.driver_v.value < configuration.uvlo_analog){
            undervoltage = pdTRUE;   
        }
    }else{  //Digital UVLO
        if(UVLO_Read() == 0){
            undervoltage = pdTRUE;
        }
    }
    
	//UVLO feedback via system_fault (LED2)
	if(undervoltage){
        if(sysfault.uvlo==0){
            alarm_push(ALM_PRIO_CRITICAL, "DRIVER: Undervoltage", ALM_NO_VALUE);
        }
        sysfault.uvlo=1;
    }else{
        sysfault.uvlo=0;
    }
}

uint32_t feedback_error_cnt=0;

void reset_fault(){
    feedback_error_cnt = 0;
    for(uint8_t i=0;i<sizeof(SYSFAULT);i++){
        ((uint8_t*)&sysfault)[i]=0;
    }
}

uint8_t tsk_fault_is_fault(){
    return (system_fault_Control & 0b1) ? pdFALSE : pdTRUE;   
}

void sysflt_set(uint32_t wait){
    system_fault_Control = (switch_without_fb & 0b10);
    if(wait) CyDelayUs(500); //TODO evaluate delay time
}

void sysflt_clr(uint32_t wait){
    system_fault_Control = (switch_without_fb & 0b10) | 0b1;
    if(wait) CyDelayUs(500); //TODO evaluate delay time
}

void handle_FAULT(void) {
	//UVLO feedback via system_fault (LED2)
	uint8_t flag = 1;
    
    for(uint8_t i=0;i<sizeof(SYSFAULT);i++){
        if(((uint8_t*)&sysfault)[i]) flag = 0; //Sysfault is active low
    }
    system_fault_Control = (switch_without_fb & 0b10) | (flag & 0b1);
    
    if(tsk_fault_is_fault()){
        LED_sysfault_Write(LED_ON);
    }else{
        LED_sysfault_Write(LED_OFF);
    }
}



void handle_no_fb(void){
    uint32_t time = ((uint32_t)fb_filter_out * 15625ul)/1000; //ns
    tt.n.fres.value = (5000000ul / time);
    static uint8_t fb_error_timer=FB_ERROR_TIME;
    
    if(no_fb_reg_Read()){
        feedback_error_cnt++;
        fb_error_timer=FB_ERROR_TIME;
    }else{
        fb_error_timer--;
        if(fb_error_timer==0){
            fb_error_timer = 20;
            feedback_error_cnt = 0;
        }
    }
    
    if(configuration.max_fb_errors){
        if(feedback_error_cnt > configuration.max_fb_errors){
            if(sysfault.feedback==0){
                alarm_push(ALM_PRIO_CRITICAL, "FAULT: No Feedback", feedback_error_cnt);
            }
            sysfault.feedback = pdTRUE;          
        }
    }else{
        sysfault.feedback = pdFALSE;   
    }
    
    
    
}

void vWD_Timer_Callback(TimerHandle_t xTimer){
    if(sysfault.watchdog==0){
        alarm_push(ALM_PRIO_CRITICAL, "WD: Watchdog triggered", ALM_NO_VALUE);
    }
    sysfault.watchdog = 1;
    interrupter1_control_Control = 0;
	QCW_enable_Control = 0;
    interrupter_kill();
    tsk_midi_kill();
    xTimerReset(xTimer,0);
}

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * This is the main procedure that comprises the task.  Place the code required
 * to preform the desired function within the merge regions of the task procedure
 * to add functionality to the task.
 */
void tsk_fault_TaskProc(void *pvParameters) {
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
    
    xWD_Timer = xTimerCreate("WD-Tmr", configuration.watchdog_timeout / portTICK_PERIOD_MS, pdFALSE,(void * ) 0, vWD_Timer_Callback);
    
    WD_enable(configuration.watchdog);
    reset_fault();
    alarm_push(ALM_PRIO_INFO, "TASK: Fault started", ALM_NO_VALUE);
	/* `#END` */
	for (;;) {
		/* `#START TASK_LOOP_CODE` */
        sysfault.eeprom = EEPROM_not_valid();
        
		handle_UVLO();
        handle_FAULT();
        LED_com_Write(LED_OFF);
        handle_no_fb();
		/* `#END` */
		vTaskDelay(FAULT_LOOP_SPEED_MS / portTICK_PERIOD_MS);
	}
}
/* ------------------------------------------------------------------------ */
void tsk_fault_Start(void) {
	/*
	 * Insert task global memeory initialization here. Since the OS does not
	 * initialize ANY global memory, execute the initialization here to make
	 * sure that your task data is properly 
	 */
	/* `#START TASK_GLOBAL_INIT` */
    
    

	/* `#END` */

	if (tsk_fault_initVar != 1) {

		/*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_fault_TaskProc, "Fault-Svc", STACK_FAULT, NULL, PRIO_FAULT, &tsk_fault_TaskHandle);
		tsk_fault_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
