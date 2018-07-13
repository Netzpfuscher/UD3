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
#include <stdarg.h>

#include "tsk_eth.h"

xTaskHandle tsk_eth_TaskHandle;
uint8 tsk_eth_initVar = 0u;

#if (1 == 1)
xSemaphoreHandle tsk_eth_Mutex;
#endif

/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */
#include "cli_common.h"
#include "tsk_midi.h"
#include "tsk_priority.h"
//#include <stdio.h>
#include "telemetry.h"
#include "ETH.h"



/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */


#define STREAMBUFFER_RX_SIZE    512     //bytes
#define STREAMBUFFER_TX_SIZE    1024    //bytes

#define LOCAL_ETH_BUFFER_SIZE 128


#define PORT_TELNET     23


/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * This is the main procedure that comprises the task.  Place the code required
 * to preform the desired function within the merge regions of the task procedure
 * to add functionality to the task.
 */
void tsk_eth_TaskProc(void *pvParameters) {
	/*
	 * Add and initialize local variables that are allocated on the Task stack
	 * the the section below.
	 */
	/* `#START TASK_VARIABLES` */
    
    uint8_t bytes_cnt;
    uint8_t buffer[LOCAL_ETH_BUFFER_SIZE];

	/* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */
    
    xETH_rx = xStreamBufferCreate(STREAMBUFFER_RX_SIZE,1);
    xETH_tx = xStreamBufferCreate(STREAMBUFFER_TX_SIZE,64);
    
 
    uint8_t socket_23 = ETH_TcpOpenServer(PORT_TELNET);
    
	   

	/* `#END` */

	for (;;) {
		/* `#START TASK_LOOP_CODE` */
 
        if(ETH_TcpConnected(socket_23) == ETH_SOCKET_OPEN){
            bytes_cnt = xStreamBufferReceive(xETH_tx, buffer, LOCAL_ETH_BUFFER_SIZE, 10 / portTICK_RATE_MS);
            if(bytes_cnt){
                ETH_TcpSend(socket_23,buffer,bytes_cnt, ETH_TXRX_FLG_WAIT);
            }
            bytes_cnt = ETH_TcpReceive(socket_23, buffer, LOCAL_ETH_BUFFER_SIZE, 0);
            if(bytes_cnt){
                xStreamBufferSend(xETH_tx, buffer, bytes_cnt, portMAX_DELAY);
            }
            
        }
    
        
        

		/* `#END` */
	}
}
/* ------------------------------------------------------------------------ */
void tsk_eth_Start(void) {
	/*
	 * Insert task global memeory initialization here. Since the OS does not
	 * initialize ANY global memory, execute the initialization here to make
	 * sure that your task data is properly 
	 */
	/* `#START TASK_GLOBAL_INIT` */
    
    SPIM0_Start();
    ETH_StartEx("0.0.0.0","255.255.255.0","00:DE:AD:BE:EF:00",configuration.ip_addr);

	/* `#END` */

	if (tsk_eth_initVar != 1) {
#if (1 == 1)
		tsk_eth_Mutex = xSemaphoreCreateMutex();
#endif

		/*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_eth_TaskProc, "ETH-Svc", 256, NULL, PRIO_ETH, &tsk_eth_TaskHandle);
		tsk_eth_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
