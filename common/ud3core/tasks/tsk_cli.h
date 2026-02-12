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
 * @file tsk_cli.h
 * @brief CLI (Command Line Interface) task
 *
 * Manages terminal instances for USB, UART, and MIN protocol connections.
 * Processes commands via TTerm library and handles multiple concurrent
 * terminal sessions.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "cli_basic.h"
#include "tsk_eth_common.h"
#include "TTerm.h"

#if !defined(cli_TASK_H)
#define cli_TASK_H
    
#define portM   ((port_str*)handle->port) //!< Macro to cast handle port to port_str
    
/*
 * Add user task definitions, types, includes and other things in the below
 * merge region to customize the task.
 */
/* `#START USER_TYPES_AND_DEFINES` */

/* `#END` */

/**
 * @brief Start CLI task
 */
void tsk_cli_Start(void);

/**
 * @brief CLI task procedure (terminal handler)
 * @param pvParameters Task parameters (port_str pointer)
 */
void tsk_cli_TaskProc(void *pvParameters);
extern xTaskHandle MIN_Terminal_TaskHandle[NUM_MIN_CON]; //!< MIN terminal task handles

extern port_str min_port[NUM_MIN_CON];   //!< MIN protocol port structures
extern TERMINAL_HANDLE * min_handle[NUM_MIN_CON]; //!< MIN terminal handles
extern port_str serial_port;             //!< UART serial port structure
extern port_str usb_port;                //!< USB CDC port structure
extern port_str null_port;               //!< Null port (no output)
extern TERMINAL_HANDLE * null_handle;    //!< Null terminal handle

extern TERMINAL_HANDLE * usb_handle;     //!< USB terminal handle

/*
 * Add user function prototypes in the below merge region to add user
 * functionality to the task definition.
 */
/* `#START USER_TASK_PROTOS` */

/* `#END` */

/* ------------------------------------------------------------------------ */
#endif
/* [] END OF FILE */
