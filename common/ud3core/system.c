/**
 * @file system.c
 * @brief System utility implementations for CPU monitoring and hardware detection
 *
 * Provides:
 * - CPU load calculation from FreeRTOS IDLE task runtime statistics
 * - Task state enumeration to string mapping
 * - Hardware revision detection via GPIO pins (TQFP package only)
 * - Hardware revision string lookup for known board versions
 *
 * Hardware revisions:
 * - v3.0/3.1a: Original designs, identified as revision 0
 * - v3.1b: Revision 1 (minor improvements)
 * - v3.1c: Revision 2 (additional fixes)
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

#include "system.h"
#include "hardware.h"

/**
 * @brief Calculate fine-grained CPU load from FreeRTOS task statistics
 *
 * Computes CPU load by finding the IDLE task and calculating active CPU time:
 * CPU load = configTICK_RATE_HZ - (idleRunTime / (sysTime / configTICK_RATE_HZ))
 *
 * Algorithm:
 * 1. Return 0 if sysTime < 500 (avoid divide-by-zero and initialization noise)
 * 2. Iterate through taskStats array to find task named "IDLE" (exact 4-char match)
 * 3. Calculate idle task's average runtime per tick
 * 4. Return active CPU load as ticks/second
 *
 * @param taskStats Array of TaskStatus_t from uxTaskGetSystemState()
 * @param taskCount Number of tasks in array
 * @param sysTime Total system runtime in ticks (from ulTaskGetIdleRunTimeCounter())
 * @return CPU load in ticks/second, or 0 if sysTime < 500, or -1 if IDLE task not found
 *
 * @note Requires configGENERATE_RUN_TIME_STATS=1 in FreeRTOSConfig.h
 * @note Higher return value = higher CPU usage
 * @note Used by telemetry system and CLI "ps" command
 */
uint32_t SYS_getCPULoadFine(TaskStatus_t * taskStats, uint32_t taskCount, uint32_t sysTime){
    if(sysTime<500) return 0;
    uint32_t currTask = 0;
    for(;currTask < taskCount; currTask++){
        if(strlen(taskStats[currTask].pcTaskName) == 4 && strcmp(taskStats[currTask].pcTaskName, "IDLE") == 0){
            return configTICK_RATE_HZ - ((taskStats[currTask].ulRunTimeCounter) / (sysTime/configTICK_RATE_HZ));
        }
    }
    return -1;
}

/**
 * @brief Convert FreeRTOS task state to human-readable string
 *
 * Maps eTaskState enumeration to descriptive strings for display.
 *
 * @param state Task state from TaskStatus_t.eCurrentState
 * @return Constant string pointer:
 *         - "running" - Task currently executing
 *         - "ready" - Task ready to run, waiting for scheduler
 *         - "blocked" - Task waiting on semaphore/queue/delay
 *         - "suspended" - Task suspended via vTaskSuspend()
 *         - "deleted" - Task deleted, handle invalid
 *         - "invalid" - Unknown/corrupted state
 *
 * @note Returned string is static, do not free
 * @note Used by CLI "ps" command to display task states
 */
const char * SYS_getTaskStateString(eTaskState state){
    switch(state){
        case eRunning:
            return "running";
        case eReady:
            return "ready";
        case eBlocked:
            return "blocked";
        case eSuspended:
            return "suspended";
        case eDeleted:
            return "deleted";
        default:
            return "invalid";
    }
}

/**
 * @brief Detect hardware revision from GPIO pins
 *
 * Reads 6-bit hardware version from VB0-VB5 GPIO pins (TQFP package only):
 * - Pins pulled up by default (read as 1)
 * - Grounding a pin sets corresponding bit to 1 in result
 * - Logic inverted: VBx_Read()==0 → bit set to 1
 *
 * Bit mapping:
 * - bits[0] = VB0 (LSB)
 * - bits[1] = VB1
 * - bits[2] = VB2
 * - bits[3] = VB3
 * - bits[4] = VB4
 * - bits[5] = VB5 (MSB)
 *
 * Known configurations:
 * - 0b000000 (0): v3.0 or v3.1a
 * - 0b000001 (1): v3.1b
 * - 0b000010 (2): v3.1c
 *
 * @return Hardware revision number (0-63)
 *         - QFN package: Always 0 (no version pins)
 *         - TQFP package: 6-bit version from VB0-VB5
 *
 * @note QFN boards lack version detection GPIO
 * @note TQFP boards must ground appropriate VBx pins to indicate revision
 * @note Called during system initialization to configure hardware-specific parameters
 */
uint8_t SYS_detect_hw_rev(void){
#if IS_QFN
    return 0;
#else
    uint8_t bits = 0;
    
    bits |= VB0_Read() ? 0 : 1;
    bits |= (VB1_Read() ? 0 : 1) << 1;
    bits |= (VB2_Read() ? 0 : 1) << 2;
    bits |= (VB3_Read() ? 0 : 1) << 3;
    bits |= (VB4_Read() ? 0 : 1) << 4;
    bits |= (VB5_Read() ? 0 : 1) << 5;

    return bits;   
#endif
}

/**
 * @brief Get human-readable hardware revision string
 *
 * Converts numeric revision to descriptive string for display.
 *
 * @param rev Hardware revision number from SYS_detect_hw_rev()
 * @return Constant string pointer:
 *         - "3.0 or 3.1a" - Original designs (rev 0)
 *         - "3.1b" - First revision (rev 1)
 *         - "3.1c" - Second revision (rev 2)
 *         - "unknown" - Unrecognized revision
 *
 * @note Returned string is static, do not free
 * @note Used for startup banner and CLI "info" command
 * @note QFN boards always report "3.0 or 3.1a" (cannot distinguish)
 */
char * SYS_get_rev_string(uint8_t rev){
    
    switch(rev){
    case 0:
        return "3.0 or 3.1a";
    case 1:
        return "3.1b";
    case 2:
        return "3.1c";
    default:
        return "unknown";
    }
}