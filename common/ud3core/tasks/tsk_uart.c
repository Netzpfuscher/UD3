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
#include "tsk_fault.h"
#include "alarmevent.h"

xTaskHandle tsk_uart_TaskHandle;
uint8 tsk_uart_initVar = 0u;

xSemaphoreHandle tx_Semaphore;


/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */
#include "cli_common.h"
#include "tsk_midi.h"
#include "tsk_cli.h"
#include "tsk_priority.h"

volatile uint8_t midi_count = 0;
volatile uint8_t midiMsg[3];


/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */

CY_ISR(isr_uart_tx) {
	if (tx_Semaphore != NULL) {
		xSemaphoreGiveFromISR(tx_Semaphore, NULL);
	}
}
CY_ISR(isr_uart_rx) {
	char c;
    LED_com_Write(LED_ON);
	while (UART_GetRxBufferSize()) {
		c = UART_GetByte();
		if (c & 0x80) {
			midi_count = 1;
			midiMsg[0] = c;

			goto end;
		} else if (!midi_count) {
            if(xStreamBufferSendFromISR(min_port[0].rx, &c, 1, 0)==0){
                alarm_push(ALM_PRIO_WARN, "COM: Serial buffer overrun",ALM_NO_VALUE);   
            }
			goto end;
		}
		switch (midi_count) {
		case 1:
			midiMsg[1] = c;
			midi_count = 2;
			break;
		case 2:
			midiMsg[2] = c;
			midi_count = 0;
			if (midiMsg[0] == 0xF0) {
				if (midiMsg[1] == 0x0F) {
					WD_reset_from_ISR();
					goto end;
				}
			}

			USBMIDI_1_callbackLocalMidiEvent(0, (uint8_t*)midiMsg);
			break;
		}
	end:;
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

	tx_Semaphore = xSemaphoreCreateBinary();

	uart_rx_StartEx(isr_uart_rx);
	uart_tx_StartEx(isr_uart_tx);

	char c;

	/* `#END` */
    alarm_push(ALM_PRIO_INFO, "TASK: UART started", ALM_NO_VALUE);
	for (;;) {
		/* `#START TASK_LOOP_CODE` */

		if (xStreamBufferReceive(min_port[0].tx, &c, 1, portMAX_DELAY)) {
            LED_com_Write(LED_ON);
			UART_PutChar(c);
			if (UART_GetTxBufferSize() == 4) {
				xSemaphoreTake(tx_Semaphore, portMAX_DELAY);
			}
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
    UART_Start();
    
	if (tsk_uart_initVar != 1) {

		/*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_uart_TaskProc, "UART-Svc", STACK_UART, NULL, PRIO_UART, &tsk_uart_TaskHandle);
		tsk_uart_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */