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

#if !defined(tsk_fault_TASK_H)
#define tsk_fault_TASK_H

/*
 * Add user task definitions, types, includes and other things in the below
 * merge region to customize the task.
 */
/* `#START USER_TYPES_AND_DEFINES` */
#include <device.h>

/* `#END` */

typedef struct __sysfault__ {
uint8_t uvlo;
uint8_t temp1;    
uint8_t temp2;
uint8_t fuse;    
uint8_t charge;
uint8_t watchdog;
uint8_t eeprom;    
uint8_t bus_uv;   
uint8_t interlock;
uint8_t link_state;
uint8_t feedback;
} SYSFAULT;

extern SYSFAULT sysfault;

extern uint32_t feedback_error_cnt;
    
void tsk_fault_Start(void);
void WD_enable(uint8_t enable);
void reset_fault();
void WD_reset();
void WD_reset_from_ISR();
void sysflt_set(uint32_t wait);
void sysflt_clr(uint32_t wait);
void set_switch_without_fb(uint32_t en);

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
