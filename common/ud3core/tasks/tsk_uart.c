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

#include "tsk_uart.h"

xTaskHandle tsk_uart_TaskHandle;
uint8 tsk_uart_initVar = 0u;

#if (1 == 1)
xSemaphoreHandle tsk_uart_Mutex;
xSemaphoreHandle tx_Semaphore;
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
#include <stdio.h>
#include "min.h"
#include "min_id.h"


//volatile uint8_t midi_count = 0;
//volatile uint8_t midiMsg[3];

struct min_context min_ctx;



/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */

#define LOCAL_UART_BUFFER_SIZE 32

uint16_t min_tx_space(uint8_t port){
  // This is a lie but we will handle the consequences
  return 512U;
}

void min_tx_byte(uint8_t port, uint8_t byte)
{
    UART_2_PutChar(byte);
}

uint32_t min_time_ms(void)
{
  uint32_t ticks = xTaskGetTickCount() * portTICK_RATE_MS;
  return ticks;
}

void min_tx_start(uint8_t port){
}
void min_tx_finished(uint8_t port){
}


void min_application_handler(uint8_t min_id, uint8_t *min_payload, uint8_t len_payload, uint8_t port)
{
    switch(min_id){
        case MIN_ID_WD:
            watchdog_reset_Control = 1;
			watchdog_reset_Control = 0;
        break;
        case MIN_ID_MIDI:
            USBMIDI_1_callbackLocalMidiEvent(0, min_payload);
        break;
        case MIN_ID_TERM:
            xStreamBufferSend( xUART_rx, min_payload, len_payload,0);
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
void tsk_uart_TaskProc(void *pvParameters) {
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
    
    xUART_rx = xStreamBufferCreate(512,1);
    xUART_tx = xStreamBufferCreate(512,1);

	tx_Semaphore = xSemaphoreCreateBinary();

    uint8_t buffer[LOCAL_UART_BUFFER_SIZE];
    uint16_t rx_bytes=0;
    uint8_t bytes_cnt=0;
    
    
    //Init MIN Protocol
    min_init_context(&min_ctx, 0);

	/* `#END` */

	for (;;) {
		/* `#START TASK_LOOP_CODE` */
/*
		if (xQueueReceive(qUart_tx, &c, portMAX_DELAY)) {
			UART_2_PutChar(c);
			if (UART_2_GetTxBufferSize() == 4) {
				xSemaphoreTake(tx_Semaphore, portMAX_DELAY);
			}
		}
  */
        rx_bytes = UART_2_GetRxBufferSize();
        bytes_cnt = 0;
        if(rx_bytes){
            if (rx_bytes > LOCAL_UART_BUFFER_SIZE) rx_bytes = LOCAL_UART_BUFFER_SIZE;
                while(bytes_cnt < rx_bytes){
                    buffer[bytes_cnt] = UART_2_GetByte();
                    bytes_cnt++;
                }
        }
        min_poll(&min_ctx, (uint8_t *)buffer, (uint8_t)bytes_cnt);
        
        bytes_cnt = xStreamBufferReceive(xUART_tx, buffer, LOCAL_UART_BUFFER_SIZE, 10 / portTICK_RATE_MS);
        if(bytes_cnt){
            UART_2_PutArray(buffer,bytes_cnt);
        }
      
        
        

		/* `#END` */
	}
}
/* ------------------------------------------------------------------------ */
void tsk_uart_Start(void) {
	/*
	 * Insert task global memeory initialization here. Since the OS does not
	 * initialize ANY global memory, execute the initialization here to make
	 * sure that your task data is properly 
	 */
	/* `#START TASK_GLOBAL_INIT` */

	/* `#END` */

	if (tsk_uart_initVar != 1) {
#if (1 == 1)
		tsk_uart_Mutex = xSemaphoreCreateMutex();
#endif

		/*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_uart_TaskProc, "UART-Svc", 128, NULL, PRIO_UART, &tsk_uart_TaskHandle);
		tsk_uart_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
