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
 * @file tsk_fault.h
 * @brief Fault monitoring task
 *
 * Monitors system faults (UVLO, temperature, fuse, interlocks, watchdog)
 * and controls SYSFLT output signal. Runs at 50ms intervals.
 */

#if !defined(tsk_fault_TASK_H)
#define tsk_fault_TASK_H

/*
 * Add user task definitions, types, includes and other things in the below
 * merge region to customize the task.
 */
/* `#START USER_TYPES_AND_DEFINES` */
#include <device.h>

/* `#END` */

/**
 * @brief System fault status structure
 *
 * Each field is a boolean flag indicating active fault condition
 */
typedef struct __sysfault__ {
uint8_t uvlo;       //!< Undervoltage lockout
uint8_t temp1;      //!< Temperature sensor 1 fault
uint8_t temp2;      //!< Temperature sensor 2 fault
uint8_t fuse;       //!< Fuse blown
uint8_t charge;     //!< Bus charging in progress
uint8_t watchdog;   //!< Watchdog timeout
uint8_t eeprom;     //!< EEPROM error
uint8_t bus_uv;     //!< Bus undervoltage
uint8_t interlock;  //!< Interlock open
uint8_t link_state; //!< Network link down
uint8_t feedback;   //!< Feedback signal error
} SYSFAULT;

extern SYSFAULT sysfault; //!< Global system fault status

extern uint32_t feedback_error_cnt; //!< Feedback error counter
    
/**
 * @brief Start fault monitoring task
 */
void tsk_fault_Start(void);

/**
 * @brief Enable or disable watchdog timer
 * @param enable 1 to enable, 0 to disable
 */
void WD_enable(uint8_t enable);

/**
 * @brief Reset all fault conditions
 */
void reset_fault();

/**
 * @brief Reset watchdog timer
 */
void WD_reset();

/**
 * @brief Reset watchdog from ISR context
 */
void WD_reset_from_ISR();

/**
 * @brief Set SYSFLT signal (fault active)
 * @param wait Delay in ticks before setting
 */
void sysflt_set(uint32_t wait);

/**
 * @brief Clear SYSFLT signal (fault cleared)
 * @param wait Delay in ticks before clearing
 */
void sysflt_clr(uint32_t wait);

/**
 * @brief Enable switching without feedback check
 * @param en 1 to enable, 0 to disable
 */
void set_switch_without_fb(uint32_t en);

/**
 * @brief Check if any fault is active
 * @return 1 if fault active, 0 otherwise
 */
uint8_t tsk_fault_is_fault();

/*
 * Add user function prototypes in the below merge region to add user
 * functionality to the task definition.
 */
/* `#START USER_TASK_PROTOS` */

/* `#END` */

/* ------------------------------------------------------------------------ */
#endif
/* [] END OF FILE */
