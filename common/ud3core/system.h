/**
 * @file system.h
 * @brief System utilities for CPU load monitoring and hardware detection
 *
 * Provides system-level helper functions:
 * - CPU load calculation from FreeRTOS task statistics
 * - Task state enumeration to string conversion
 * - Hardware revision detection via GPIO pins
 * - Hardware revision string lookup
 *
 * Copyright (c) 2021 Jens Kerrinnes
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
#if !defined(system_H)
#define system_H

#include "FreeRTOS.h"
#include "task.h"
    

/* ===== CPU Load Monitoring ===== */

/**
 * @brief Calculate fine-grained CPU load from FreeRTOS task statistics
 *
 * Computes CPU load by finding IDLE task and calculating:
 * CPU load = configTICK_RATE_HZ - (idleRunTime / (sysTime / configTICK_RATE_HZ))
 *
 * Method:
 * 1. Search taskStats array for task named "IDLE"
 * 2. Calculate idle percentage from ulRunTimeCounter
 * 3. Return active CPU load (inverse of idle)
 *
 * @param taskStats Pointer to array of TaskStatus_t from uxTaskGetSystemState()
 * @param taskCount Number of tasks in taskStats array
 * @param sysTime System time in FreeRTOS ticks (from ulTaskGetIdleRunTimeCounter())
 * @return CPU load in ticks/second, or -1 if IDLE task not found, or 0 if sysTime < 500
 *
 * @note Requires FreeRTOS runtime statistics enabled (configGENERATE_RUN_TIME_STATS)
 * @note Returns 0 for sysTime < 500 to avoid divide-by-zero and initialization transients
 */
uint32_t SYS_getCPULoadFine(TaskStatus_t * taskStats, uint32_t taskCount, uint32_t sysTime);

/**
 * @brief Convert FreeRTOS task state enum to human-readable string
 * @param state Task state from eTaskState enumeration
 * @return String representation: "running", "ready", "blocked", "suspended", "deleted", or "invalid"
 *
 * @note Used for CLI "top" command and telemetry display
 */
const char * SYS_getTaskStateString(eTaskState state);    

/* ===== Hardware Detection ===== */

/**
 * @brief Detect hardware revision from GPIO version bits
 *
 * Reads 6-bit hardware version from GPIO pins VB0-VB5:
 * - QFN package: Always returns 0 (no version pins)
 * - TQFP package: Reads VB0-VB5, inverts logic (0=pulled up, 1=grounded)
 *
 * Known revisions:
 * - 0 (0b000000): v3.0 or v3.1a
 * - 1 (0b000001): v3.1b
 * - 2 (0b000010): v3.1c
 *
 * @return Hardware revision number (0-63)
 *
 * @note QFN boards have no version detection (always report v3.0/3.1a)
 * @note TQFP boards use GPIO pins with pull-ups; ground pins to set version
 */
uint8_t SYS_detect_hw_rev(void);

/**
 * @brief Get human-readable hardware revision string
 * @param rev Hardware revision number from SYS_detect_hw_rev()
 * @return String describing hardware revision: "3.0 or 3.1a", "3.1b", "3.1c", or "unknown"
 *
 * @note Used for startup banner and CLI "info" command
 */
char * SYS_get_rev_string(uint8_t rev);    
    
#endif