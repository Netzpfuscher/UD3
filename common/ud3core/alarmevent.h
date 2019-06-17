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

#if !defined(alarmevent_H)
#define alarmevent_H
    
#include <device.h>
    
typedef struct __alarms__ {
uint16 num;
uint8 alarm_level;
char* message;
uint32_t timestamp;
} ALARMS;
    

void alarm_push(uint8_t level, const char* message);
uint32_t alarm_get_num();
void alarm_init();
uint32_t alarm_get(uint32_t index, ALARMS * alm);
void alarm_clear();

#define ALM_PRIO_INFO       0
#define ALM_PRIO_WARN       1
#define ALM_PRIO_ALARM      2
#define ALM_PRIO_CRITICAL   3


static const char warn_general_startup[]= "INFO: UD3 startup";

static const char warn_task_analog[]= "TASK: Analog started";
static const char warn_task_cli[]= "TASK: CLI started";
static const char warn_task_eth[]= "TASK: ETH started";
static const char warn_task_fault[]= "TASK: Fault started";
static const char warn_task_midi[]= "TASK: MIDI started";
static const char warn_task_min[]= "TASK: MIN started";
static const char warn_task_overlay[]= "TASK: Overlay started";
static const char warn_task_thermistor[]= "TASK: Thermistor started";
static const char warn_task_uart[]= "TASK: UART started";
static const char warn_task_usb[]= "TASK: USB started";

static const char warn_bus_charging[]= "BUS: Charging";
static const char warn_bus_ready[]= "BUS: Ready";
static const char warn_bus_fault[]= "BUS: Fault";
static const char warn_bus_off[]= "BUS: Off";
static const char warn_bus_undervoltage[]= "BUS: Undervoltage";

static const char warn_temp_fault[]= "NTC: Temperature fault";

static const char warn_driver_undervoltage[]= "DRIVER: Undervoltage";

static const char warn_kill_set[]= "FAULT: Killbit set";
static const char warn_kill_reset[]= "INFO: Killbit reset";

static const char warn_eeprom_no_dataset[]= "EEPROM: No or old dataset found";
static const char warn_eeprom_unknown_id[]= "EEPROM: Found unknown ID";
static const char warn_eeprom_unknown_param[]= "EEPROM: Found unknown parameter";
static const char warn_eeprom_loaded[]= "EEPROM: Dataset loaded";
static const char warn_eeprom_written[]= "EEPROM: Dataset written";

static const char warn_W5500_failed[]= "W5550: Timeout, check connection";

static const char warn_watchdog[]= "WD: Watchdog triggerd";
    
    
    
/* ------------------------------------------------------------------------ */
#endif
/* [] END OF FILE */