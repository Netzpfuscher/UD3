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
    
typedef struct __alarms__ {
	uint8 alarm_level;
    char* message;
    uint32_t timestamp;
} ALARMS;
    
void tsk_fault_Start(void);
void WD_enable(uint8_t enable);

void alarm_push(uint8_t level, const char* message);
uint32_t alarm_get_num();
uint32_t alarm_get(uint32_t index, ALARMS * alm);
void alarm_clear();

#define ALM_PRIO_INFO       0
#define ALM_PRIO_WARN       1
#define ALM_PRIO_ALARM      2
#define ALM_PRIO_CRITICAL   3


static const char warn_general_startup[]= "UD3 startup";

static const char warn_task_analog[]= "Task analog started";
static const char warn_task_cli[]= "Task CLI started";
static const char warn_task_eth[]= "Task ETH started";
static const char warn_task_fault[]= "Task fault started";
static const char warn_task_midi[]= "Task midi started";
static const char warn_task_min[]= "Task min started";
static const char warn_task_overlay[]= "Task overlay started";
static const char warn_task_thermistor[]= "Task thermistor started";
static const char warn_task_uart[]= "Task UART started";
static const char warn_task_usb[]= "Task USB started";

static const char warn_bus_charging[]= "Bus charging";
static const char warn_bus_ready[]= "Bus ready";
static const char warn_bus_fault[]= "Bus fault";
static const char warn_bus_off[]= "Bus off";
static const char warn_bus_undervoltage[]= "Bus undervoltage";

static const char warn_temp_fault[]= "Temperature fault";

static const char warn_driver_undervoltage[]= "Driver undervoltage";




/*
 * Add user function prototypes in the below merge region to add user
 * functionality to the task definition.
 */
/* `#START USER_TASK_PROTOS` */

/* `#END` */

/* ------------------------------------------------------------------------ */
#endif
/* [] END OF FILE */
