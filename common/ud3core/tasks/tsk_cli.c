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

#include "tsk_cli.h"

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

xTaskHandle UART_Terminal_TaskHandle;
xTaskHandle USB_Terminal_TaskHandle;
xTaskHandle ETH_Terminal_TaskHandle;
uint8 tsk_cli_initVar = 0u;

/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */

#include "cli_common.h"
#include "ntshell.h"
#include "tsk_priority.h"
#include "tsk_uart.h"
#include "tsk_usb.h"
#include "tsk_eth.h"
#include <project.h>

#define UNUSED_VARIABLE(N) \
	do {                   \
		(void)(N);         \
	} while (0)
void *extobjt = 0;

static int serial_write(const char *buf, int cnt, void *extobj);
static int usb_write(const char *buf, int cnt, void *extobj);
static int eth_write(const char *buf, int cnt, void *extobj);
void initialize_cli(ntshell_t *ptr, uint8_t port);
static int serial_callback(const char *text, void *extobj);
static int usb_callback(const char *text, void *extobj);
static int eth_callback(const char *text, void *extobj);

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */



void initialize_cli(ntshell_t *ptr, uint8_t port) {

	switch (port) {
	case SERIAL:
		UART_2_Start();
		ntshell_init(ptr, serial_write, serial_callback, extobjt);
        command_cls("", SERIAL);
		break;
	case USB:
		USBMIDI_1_Start(0, USBMIDI_1_5V_OPERATION);
		ntshell_init(ptr, usb_write, usb_callback, extobjt);
		break;
    case ETH:
		ntshell_init(ptr, eth_write, eth_callback, extobjt);
		break;
        
	default:
		break;
	}
	ntshell_set_prompt(ptr, ":>");
	ntshell_show_promt(ptr);
}

static int serial_write(const char *buf, int cnt, void *extobj) {
	UNUSED_VARIABLE(extobj);
    
    xStreamBufferSend(xUART_tx,buf, cnt,portMAX_DELAY);

	return cnt;
}

static int usb_write(const char *buf, int cnt, void *extobj) {
	UNUSED_VARIABLE(extobj);
	if (0u != USBMIDI_1_GetConfiguration()) {

		for (int i = 0; i < cnt; i++) {
			xQueueSend(qUSB_tx, &buf[i], portMAX_DELAY);
		}
	}

	return cnt;
}

static int eth_write(const char *buf, int cnt, void *extobj) {
	UNUSED_VARIABLE(extobj);
    
    xStreamBufferSend(xETH_tx,buf, cnt,portMAX_DELAY);

	return cnt;
}

static int serial_callback(const char *text, void *extobj) {
	UNUSED_VARIABLE(extobj);
	/*
     * This is a really simple example codes for the callback function.
     */
	nt_interpret(text, SERIAL);
	return 0;
}

static int usb_callback(const char *text, void *extobj) {
	UNUSED_VARIABLE(extobj);
	/*
     * This is a really simple example codes for the callback function.
     */
	nt_interpret(text, USB);
	return 0;
}

static int eth_callback(const char *text, void *extobj) {
	UNUSED_VARIABLE(extobj);
	/*
     * This is a really simple example codes for the callback function.
     */
	nt_interpret(text, ETH);
	return 0;
}

uint8_t handle_uart_terminal(ntshell_t *ptr) {
	static uint8_t blink = 0;
	char c;
	if (blink)
		blink--;
	if (blink == 1)
		rx_blink_Control = 1;

	if (xStreamBufferReceive(xUART_rx, &c,1, portMAX_DELAY)) {
		if (xSemaphoreTake(block_term[SERIAL], portMAX_DELAY)) {
			rx_blink_Control = 0;
			blink = 240;
			ntshell_execute_nb(ptr, c);
			xSemaphoreGive(block_term[SERIAL]);
		} 
	}

	return 0;
}

uint8_t handle_USB_terminal(ntshell_t *ptr) {
	static uint8_t blink = 0;
	char c;
	if (blink)
		blink--;
	if (blink == 1)
		rx_blink_Control = 1;
	if (xQueueReceive(qUSB_rx, &c, portMAX_DELAY)) {
		xSemaphoreTake(block_term[USB], portMAX_DELAY);
		rx_blink_Control = 0;
		blink = 240;
		ntshell_execute_nb(ptr, c);
		xSemaphoreGive(block_term[USB]);
	}

	return 0;
}

uint8_t handle_ETH_terminal(ntshell_t *ptr) {
	char c;
	if (xStreamBufferReceive(xETH_rx, &c,1, 20 /portTICK_RATE_MS)) {
		if (xSemaphoreTake(block_term[ETH], 20 /portTICK_RATE_MS)) {
			ntshell_execute_nb(ptr, c);
			xSemaphoreGive(block_term[ETH]);
		} 
	}

	return 0;
}

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * This is the main procedure that comprises the task.  Place the code required
 * to preform the desired function within the merge regions of the task procedure
 * to add functionality to the task.
 */
void tsk_cli_TaskProc(void *pvParameters) {
	/*
	 * Add and initialize local variables that are allocated on the Task stack
	 * the the section below.
	 */
	/* `#START TASK_VARIABLES` */

	ntshell_t ntserial;

	/* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */

	initialize_cli(&ntserial, (uint32_t)pvParameters);

	/* `#END` */

	for (;;) {
		/* `#START TASK_LOOP_CODE` */

		switch ((uint32_t)pvParameters) {
		case SERIAL:
			handle_uart_terminal(&ntserial);
			break;
		case USB:
			handle_USB_terminal(&ntserial);
			break;
        case ETH:
			handle_ETH_terminal(&ntserial);
			break;
		default:
			break;
		}
        

		/* `#END` */

		//vTaskDelay( 20/ portTICK_PERIOD_MS);
	}
}
/* ------------------------------------------------------------------------ */
void tsk_cli_Start(void) {
/*
	 * Insert task global memeory initialization here. Since the OS does not
	 * initialize ANY global memory, execute the initialization here to make
	 * sure that your task data is properly 
	 */
/* `#START TASK_GLOBAL_INIT` */

/* `#END` */

	if (tsk_cli_initVar != 1) {

		/*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_cli_TaskProc, "UART-CLI", 576, (void *)SERIAL, PRIO_TERMINAL, &UART_Terminal_TaskHandle);
		xTaskCreate(tsk_cli_TaskProc, "USB-CLI", 576, (void *)USB, PRIO_TERMINAL, &USB_Terminal_TaskHandle);
        xTaskCreate(tsk_cli_TaskProc, "ETH-CLI", 576, (void *)ETH, PRIO_TERMINAL, &ETH_Terminal_TaskHandle);
		tsk_cli_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
