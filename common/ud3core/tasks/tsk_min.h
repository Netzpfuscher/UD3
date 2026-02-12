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

/**
 * @file tsk_min.h
 * @brief MIN protocol task for UART communication
 *
 * Handles MIN (Microcontroller Interconnect Network) protocol over UART.
 * Provides multi-client communication with flow control and time synchronization.
 */

#if !defined(tsk_min_TASK_H)
#define tsk_min_TASK_H

/*
 * Add user task definitions, types, includes and other things in the below
 * merge region to customize the task.
 */
/* `#START USER_TYPES_AND_DEFINES` */
#include <device.h>
    
/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "tsk_eth_common.h"
#include "min.h"

/* `#END` */

/**
 * @brief Socket information structure for MIN connections
 *
 * Tracks state information for each MIN protocol socket.
 */
struct _socket_info {
	uint8_t socket;      //!< Socket number
	uint8_t old_state;   //!< Previous connection state
	char info[16];       //!< Descriptive information string
};

/**
 * @brief Time synchronization structure
 *
 * Manages time offset between local and remote systems.
 */
struct _time {
	uint32_t remote;    //!< Remote system time (milliseconds)
	int32_t diff;       //!< Filtered time difference (milliseconds)
	int32_t diff_raw;   //!< Raw time difference (milliseconds)
	uint32_t resync;    //!< Resync counter
};

extern struct min_context min_ctx;  //!< MIN protocol context

/**
 * @brief Queue a MIN protocol frame for transmission
 *
 * @param id MIN protocol message ID
 * @param data Pointer to message payload
 * @param len Length of payload in bytes
 * @param ticks Maximum ticks to wait for queue space
 * @return pdTRUE if queued successfully, pdFALSE otherwise
 */
uint8_t min_queue(uint8_t id, uint8_t *data, uint8_t len, TickType_t ticks);

/**
 * @brief Send a MIN protocol frame immediately
 *
 * @param id MIN protocol message ID
 * @param data Pointer to message payload
 * @param len Length of payload in bytes
 * @param ticks Maximum ticks to wait for semaphore
 * @return pdTRUE if sent successfully, pdFALSE otherwise
 */
uint8_t min_send(uint8_t id, uint8_t *data, uint8_t len, TickType_t ticks);

/**
 * @brief Print formatted text to MIN protocol port
 *
 * @param port Pointer to MIN port (cast to void*)
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
void MIN_print(void *port, char *format, ...);

/**
 * @brief Start the MIN protocol task
 */
void tsk_min_Start(void);

/**
 * @brief Reset MIN protocol flow control state
 */
void min_reset_flow(void);

extern struct _socket_info socket_info[NUM_MIN_CON]; //!< Array of socket information structures
extern struct _time min_time;                         //!< Time synchronization data
extern uint32_t uart_bytes_rx;                        //!< Total UART bytes received
extern uint32_t uart_bytes_tx;                        //!< Total UART bytes transmitted
/*
 * Add user function prototypes in the below merge region to add user
 * functionality to the task definition.
 */
/* `#START USER_TASK_PROTOS` */

/* `#END` */

/* ------------------------------------------------------------------------ */
#endif
/* [] END OF FILE */
