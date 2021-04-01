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
#include "cli_common.h"
#include <stdlib.h>
#include "telemetry.h"
#include "helper/PCA9685.h"

xTaskHandle tsk_i2c_TaskHandle;
uint8 tsk_i2c_initVar = 0u;



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

uint8_t CMD_i2c(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    uint8_t byte;
    uint8_t status;
    uint8_t cnt=0;
    //PCA9685_init();
    vTaskDelay(10);
    
    uint16_t val = atoi(args[0]);
    

    //ttprintf("Mode 1: %x\r\n",I2C_Read(0x40,MODE1));

       
    vTaskDelay(50);
 
    /*
    I2C_MasterClearStatus();
    do{
       
        status = I2C_MasterSendStart(cnt,I2C_READ_XFER_MODE);
        
        if(status==0) ttprintf("Found device = %x\r\n",cnt);
        if(status==0) byte = I2C_MasterReadByte(I2C_NAK_DATA);
        status = I2C_MasterSendStop();
        cnt++;
        if(cnt>127){
            cnt = 0;
            ttprintf("Finished scan...\r\n",cnt);
            break;
        }
        
        
    }while(Term_check_break(handle,20));
    */
    return TERM_CMD_EXIT_SUCCESS;
}



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
    
    PCA9685_init();
    
    uint16_t cnt=0;
    uint8_t tog=1;
    
	/* `#END` */

	for (;;) {

        for(uint8_t i = 0;i<16;i++){
            PCA9685_setPWM(i,cnt);
        }
        
        if(tog){
            cnt+=100;
        }else{
            cnt-=100;
        }
        if(cnt>4095) tog=0;
        if(cnt==0) tog=1;
        vTaskDelay(20);
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
