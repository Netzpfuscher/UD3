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

#include "tsk_min.h"
#include "tsk_uart.h"

xTaskHandle tsk_min_TaskHandle;
uint8 tsk_min_initVar = 0u;


/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */
#include "cli_common.h"
#include "tsk_priority.h"
//#include <stdio.h>
#include "min.h"
#include "min_id.h"
#include "telemetry.h"
#include "stream_buffer.h" 


struct min_context min_ctx;


/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */

#define LOCAL_UART_BUFFER_SIZE  128     //bytes
#define CONNECTION_TIMEOUT      150     //ms
#define CONNECTION_HEARTBEAT    50      //ms
#define STREAMBUFFER_RX_SIZE    512     //bytes
#define STREAMBUFFER_TX_SIZE    1024    //bytes

uint16_t min_tx_space(uint8_t port){
    return (UART_2_TX_BUFFER_SIZE - UART_2_GetTxBufferSize());
}

void min_tx_byte(uint8_t port, uint8_t byte){
    UART_2_PutChar(byte);
}

uint32_t min_time_ms(void){
  return (xTaskGetTickCount() * portTICK_RATE_MS);
}

void min_tx_start(uint8_t port){
}
void min_tx_finished(uint8_t port){
}
uint32_t reset;

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
        case MIN_ID_RESET:
            reset  = min_time_ms(); 
            watchdog_reset_Control = 1;
			watchdog_reset_Control = 0;
        break;

    }
}

void min_reset_wd(void){
    static uint32_t last=0;
    
    if((min_time_ms()-reset)>CONNECTION_TIMEOUT){
            min_transport_reset(&min_ctx,true);
        }
        
        if((min_time_ms()-last) > CONNECTION_HEARTBEAT){
            last = min_time_ms();
            uint8_t byte=0;
            min_send_frame(&min_ctx,MIN_ID_RESET,&byte,1);   
        }
    
}


void poll_UART(uint8_t* ptr){
        
    uint8_t bytes = UART_2_GetRxBufferSize();
    uint8_t bytes_cnt=0;
    if(bytes){
        if (bytes > LOCAL_UART_BUFFER_SIZE) bytes = LOCAL_UART_BUFFER_SIZE;
            while(bytes_cnt < bytes){
                ptr[bytes_cnt] = UART_2_GetByte();
                bytes_cnt++;
            }
    }
    min_poll(&min_ctx, ptr, (uint8_t)bytes_cnt);
    
}

void write_telemetry(struct min_context* ptr){
    telemetry.dropped_frames = ptr->transport_fifo.dropped_frames;
    telemetry.spurious_acks = ptr->transport_fifo.spurious_acks;
    telemetry.sequence_mismatch_drop = ptr->transport_fifo.sequence_mismatch_drop;
    telemetry.resets_received = ptr->transport_fifo.resets_received;
    telemetry.min_frames_max = ptr->transport_fifo.n_frames_max;
}


/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * This is the main procedure that comprises the task.  Place the code required
 * to preform the desired function within the merge regions of the task procedure
 * to add functionality to the task.
 */
void tsk_min_TaskProc(void *pvParameters) {
	/*
	 * Add and initialize local variables that are allocated on the Task stack
	 * the the section below.
	 */
	/* `#START TASK_VARIABLES` */
    
    uint8_t buffer[LOCAL_UART_BUFFER_SIZE];
    uint8_t buffer_u[LOCAL_UART_BUFFER_SIZE];

    uint8_t bytes_cnt=0;

	/* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */
    
    xUART_rx = xStreamBufferCreate(STREAMBUFFER_RX_SIZE,1);
    xUART_tx = xStreamBufferCreate(STREAMBUFFER_TX_SIZE,LOCAL_UART_BUFFER_SIZE/2);

    //Init MIN Protocol
    min_init_context(&min_ctx, 0);
    min_transport_reset(&min_ctx,1);
        
	/* `#END` */

	for (;;) {
		/* `#START TASK_LOOP_CODE` */
        write_telemetry(&min_ctx);
        poll_UART(buffer_u);
        min_reset_wd();
  
        bytes_cnt = xStreamBufferReceive(xUART_tx, buffer, LOCAL_UART_BUFFER_SIZE, 10 / portTICK_RATE_MS);
        if(bytes_cnt){
            uint8_t res=0;
           
            res = min_queue_frame(&min_ctx,MIN_ID_TERM,buffer,bytes_cnt);

            while(!res){
                min_reset_wd();
                
                min_poll(&min_ctx, (uint8_t *)buffer_u, 0);
                
                vTaskDelay(5);
                
                poll_UART(buffer_u);
                
                vTaskDelay(5);
                res = min_queue_frame(&min_ctx,MIN_ID_TERM,buffer,bytes_cnt);
            }
        }
      
        
        

		/* `#END` */
	}
}
/* ------------------------------------------------------------------------ */
void tsk_min_Start(void) {
	/*
	 * Insert task global memeory initialization here. Since the OS does not
	 * initialize ANY global memory, execute the initialization here to make
	 * sure that your task data is properly 
	 */
	/* `#START TASK_GLOBAL_INIT` */

	/* `#END` */

	if (tsk_min_initVar != 1) {

		/*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_min_TaskProc, "MIN-Svc", 256, NULL, PRIO_UART, &tsk_min_TaskHandle);
		tsk_min_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */