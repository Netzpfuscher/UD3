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

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "tsk_priority.h"
#include "tsk_cli.h"
#include "cli_common.h"
#include <stdlib.h>
#include "telemetry.h"
#include "alarmevent.h"
#include "helper/PCA9685.h"

xTaskHandle tsk_i2c_TaskHandle;
uint8 tsk_i2c_initVar = 0u;

#define N_PCA9685 4

typedef struct  {
    int32_t* value;
    int32_t min;
    int32_t max;
    int32_t divider;
}PCA9685_str;

PCA9685_str gauge_data[N_PCA9685];


/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */


/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * This is the main procedure that comprises the task.  Place the code required
 * to preform the desired function within the merge regions of the task procedure
 * to add functionality to the task.
 */


void tsk_i2c_TaskProc(void *pvParameters) {
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
    I2C_Start();
    
    PCA9685* pca_ptr = PCA9685_new(0x40);
    if(pca_ptr==NULL) alarm_push(ALM_PRIO_CRITICAL, "PCA9685: Malloc failed", ALM_NO_VALUE);
    
    gauge_data[0].value = &tt.n.midi_voices.value;
    gauge_data[0].min = 0;
    gauge_data[0].max = 4;
    gauge_data[0].divider = 0;
    
    gauge_data[1].value = &tt.n.primary_i.value;
    gauge_data[1].min = 0;
    gauge_data[1].max = 1500;
    gauge_data[1].divider = 0;
    
    gauge_data[2].value = &tt.n.bus_v.value;
    gauge_data[2].min = 0;
    gauge_data[2].max = 600;
    gauge_data[2].divider = 0;
    
    gauge_data[3].value = &tt.n.batt_i.value;
    gauge_data[3].min = 0;
    gauge_data[3].max = 40;
    gauge_data[3].divider = 10;
    
    
	/* `#END` */

    alarm_push(ALM_PRIO_INFO, "TASK: I2C started", ALM_NO_VALUE);
    
	for (;;) {
        
        for(uint8_t i = 0; i < N_PCA9685;i++){
            if(gauge_data[i].value != NULL){
                int32_t temp_val;
                if(gauge_data[i].divider){
                    temp_val = *gauge_data[i].value / gauge_data[i].divider;
                }else{
                    temp_val = *gauge_data[i].value;
                }

                
                if(temp_val > gauge_data[i].max) temp_val = gauge_data[i].max;
                if(temp_val < gauge_data[i].min) temp_val = gauge_data[i].min;
                
                temp_val = (temp_val * 4095) / gauge_data[i].max;

                PCA9685_setPWM(pca_ptr,i,temp_val);
            }
        }
        vTaskDelay(50);
	}
}
/* ------------------------------------------------------------------------ */
void tsk_i2c_Start(void) {
	/*
	 * Insert task global memeory initialization here. Since the OS does not
	 * initialize ANY global memory, execute the initialization here to make
	 * sure that your task data is properly 
	 */
	/* `#START TASK_GLOBAL_INIT` */
    
    

	/* `#END` */

	if (tsk_i2c_initVar != 1) {

		/*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_i2c_TaskProc, "I2C-Svc", STACK_I2C, NULL, PRIO_I2C, &tsk_i2c_TaskHandle);
		tsk_i2c_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
