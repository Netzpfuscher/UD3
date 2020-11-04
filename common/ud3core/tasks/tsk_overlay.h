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

#if !defined(overlay_TASK_H)
#define overlay_TASK_H
    
#include <device.h>
#include "cli_basic.h"
#include "TTerm.h"    
    
/*
 * Add user task definitions, types, includes and other things in the below
 * merge region to customize the task.
 */
/* `#START USER_TYPES_AND_DEFINES` */

/* `#END` */

void tsk_overlay_TaskProc(void *pvParameters);

void tsk_overlay_chart_stop();
void tsk_overlay_chart_start();
void init_telemetry();
void start_overlay_task(TERMINAL_HANDLE * handle);
void stop_overlay_task(TERMINAL_HANDLE * handle);
void init_tt(uint8_t with_chart, TERMINAL_HANDLE * handle);
void recalc_telemetry_limits();


uint8_t CMD_tterm(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
uint8_t CMD_status(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
uint8_t CMD_telemetry(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
    
/*
 * Add user function prototypes in the below merge region to add user
 * functionality to the task definition.
 */
/* `#START USER_TASK_PROTOS` */

/* `#END` */

/* ------------------------------------------------------------------------ */
#endif
/* [] END OF FILE */
