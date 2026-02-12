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
 * @file alarmevent.h
 * @brief Alarm and event management system for UD3
 * 
 * Provides a queue-based alarm/event logging system with priority levels,
 * timestamps, and terminal display capabilities. Used for fault tracing,
 * warnings, and informational messages.
 */

#if !defined(alarmevent_H)
#define alarmevent_H
    
#include <device.h>
#include "FreeRTOS.h"
#include "TTerm.h"
    
/**
 * @brief Alarm entry structure
 * 
 * Stores a single alarm/event with metadata including priority level,
 * timestamp, and optional numeric value.
 */
typedef struct __alarms__ {
	uint16_t num;           /**< Sequential alarm number */
	uint8_t alarm_level;    /**< Priority level (ALM_PRIO_*) */
	char* message;          /**< Alarm message (flash or heap) */
	uint32_t timestamp;     /**< Timestamp in milliseconds since boot */
	uint32_t value;         /**< Optional numeric value (ALM_NO_VALUE if unused) */
} ALARMS;


    
/**
 * @brief Check if pointer points to flash memory
 * @param ptr Pointer to check
 * @return pdTRUE if pointer is in flash, pdFALSE otherwise
 */
BaseType_t ptr_is_in_flash(void* ptr);

/**
 * @brief Push alarm with copied message string
 * @param level Priority level (ALM_PRIO_INFO/WARN/ALARM/CRITICAL)
 * @param message Message string to copy (will be allocated on heap)
 * @param len Length of message string
 * @param value Optional numeric value (use ALM_NO_VALUE if not needed)
 * 
 * Allocates heap memory for message copy. Use for dynamically generated messages.
 */
void alarm_push_c(uint8_t level, char* message, uint16_t len, int32_t value);

/**
 * @brief Push alarm with constant message string
 * @param level Priority level (ALM_PRIO_INFO/WARN/ALARM/CRITICAL)
 * @param message Message string (must be in flash or static storage)
 * @param value Optional numeric value (use ALM_NO_VALUE if not needed)
 * 
 * Message pointer is stored directly (not copied). Use for string literals.
 * If queue is full, oldest alarm is removed to make space.
 */
void alarm_push(uint8_t level, const char* message, int32_t value);

/**
 * @brief Get number of alarms in queue
 * @return Number of alarms currently stored
 */
uint32_t alarm_get_num();

/**
 * @brief Initialize alarm system
 * 
 * Creates the alarm queue. Must be called before using alarm functions.
 */
void alarm_init();

/**
 * @brief Get alarm at specific index without removing it
 * @param index Index in queue (0 = oldest)
 * @param alm Pointer to ALARMS structure to fill
 * @return pdPASS if successful, pdFAIL if queue empty or index invalid
 */
uint32_t alarm_get(uint32_t index, ALARMS * alm);

/**
 * @brief Clear all alarms from queue
 * 
 * Frees heap-allocated messages and empties the queue.
 */
void alarm_clear();

/**
 * @brief Remove and retrieve oldest alarm from queue
 * @param alm Pointer to ALARMS structure to fill
 * @return pdPASS if successful, pdFAIL if queue empty
 * 
 * Call alarm_free() on returned alarm if message was heap-allocated.
 */
uint32_t alarm_pop(ALARMS * alm);

/**
 * @brief Free heap-allocated alarm message
 * @param alm Pointer to alarm structure
 * @return pdTRUE if message was freed, pdFAIL if not heap-allocated
 */
uint32_t alarm_free(ALARMS * alm);

/**
 * @brief Terminal command for alarm management
 * @param handle Terminal handle
 * @param argCount Number of arguments
 * @param args Command arguments (get|roll|reset)
 * @return TERM_CMD_EXIT_SUCCESS
 * 
 * Commands:
 * - get: Display all alarms
 * - roll: Continuously display new alarms until interrupted
 * - reset: Clear all alarms
 */
uint8_t CMD_alarms(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);

/** @defgroup alarm_priorities Alarm Priority Levels
 * @{
 */
#define ALM_PRIO_INFO       0  /**< Informational message (green) */
#define ALM_PRIO_WARN       1  /**< Warning (white) */
#define ALM_PRIO_ALARM      2  /**< Alarm condition (cyan) */
#define ALM_PRIO_CRITICAL   3  /**< Critical fault, stops operation (red) */
/** @} */

/**
 * @brief Sentinel value indicating no numeric value associated with alarm
 */
#define ALM_NO_VALUE        0x80000000
   
/* ------------------------------------------------------------------------ */
#endif
/* [] END OF FILE */