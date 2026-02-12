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
 * @file tsk_overlay.h
 * @brief Terminal overlay task for telemetry display
 *
 * Provides real-time telemetry display with live charts and status information
 * using VT100 terminal escape sequences.
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

/**
 * @brief Main overlay task entry point
 *
 * @param pvParameters Task parameters (unused)
 */
void tsk_overlay_TaskProc(void *pvParameters);

/**
 * @brief Stop the telemetry chart display
 */
void tsk_overlay_chart_stop();

/**
 * @brief Start the telemetry chart display
 */
void tsk_overlay_chart_start();

/**
 * @brief Initialize telemetry system
 *
 * Sets up telemetry data structures and limits.
 */
void init_telemetry();

/**
 * @brief Start overlay task for specified terminal
 *
 * @param handle Pointer to terminal handle to display overlay on
 */
void start_overlay_task(TERMINAL_HANDLE *handle);

/**
 * @brief Stop overlay task for specified terminal
 *
 * @param handle Pointer to terminal handle to stop overlay on
 */
void stop_overlay_task(TERMINAL_HANDLE *handle);

/**
 * @brief Initialize telemetry terminal display
 *
 * @param with_chart Enable live charting (1) or disable (0)
 * @param handle Pointer to terminal handle
 */
void init_tt(uint8_t with_chart, TERMINAL_HANDLE *handle);

/**
 * @brief Recalculate telemetry display limits
 *
 * Updates min/max scaling for telemetry values.
 */
void recalc_telemetry_limits();

/**
 * @brief Command handler for tterm command
 *
 * @param handle Terminal handle
 * @param argCount Number of arguments
 * @param args Array of argument strings
 * @return pdTRUE if command succeeded, pdFALSE otherwise
 */
uint8_t CMD_tterm(TERMINAL_HANDLE *handle, uint8_t argCount, char **args);

/**
 * @brief Command handler for status display
 *
 * @param handle Terminal handle
 * @param argCount Number of arguments
 * @param args Array of argument strings
 * @return pdTRUE if command succeeded, pdFALSE otherwise
 */
uint8_t CMD_status(TERMINAL_HANDLE *handle, uint8_t argCount, char **args);

/**
 * @brief Command handler for telemetry display
 *
 * @param handle Terminal handle
 * @param argCount Number of arguments
 * @param args Array of argument strings
 * @return pdTRUE if command succeeded, pdFALSE otherwise
 */
uint8_t CMD_telemetry(TERMINAL_HANDLE *handle, uint8_t argCount, char **args);
    
/*
 * Add user function prototypes in the below merge region to add user
 * functionality to the task definition.
 */
/* `#START USER_TASK_PROTOS` */

/* `#END` */

/* ------------------------------------------------------------------------ */
#endif
/* [] END OF FILE */
