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

#if !defined(tsk_analog_TASK_H)
#define tsk_analog_TASK_H

/*
 * Add user task definitions, types, includes and other things in the below
 * merge region to customize the task.
 */
/* `#START USER_TYPES_AND_DEFINES` */
#include <device.h>
#include "cli_basic.h"

#define CT_PRIMARY 0
#define CT_SECONDARY 1
 
enum I2T {
    I2T_NORMAL,
    I2T_WARNING,
    I2T_LIMIT
};

#define AC_PRECHARGE_TIMEOUT 100  //

#define BAT_PRECHARGE_BUS_SCHEME   0   //battery "directly" supplies bus voltage
#define BAT_BOOST_BUS_SCHEME       1   //battery is boosted via SLR converter
#define AC_PRECHARGE_BUS_SCHEME    2   //no batteries, mains powered
#define AC_DUAL_MEAS_SCHEME        3   //measures the input voltage and the capacitor voltage
#define AC_NO_RELAY_BUS_SCHEME     4   //mains powered without a builtin precharging relay
#define AC_PRECHARGE_FIXED_DELAY   5

#define RELAY_CHARGE 1
#define RELAY_OFF 0
#define RELAY_CHARGE_OFF 2
#define RELAY_ON 3

#define BUS_COMMAND_OFF 0
#define BUS_COMMAND_ON 1
#define BUS_COMMAND_FAULT 2

#define relay_write_bus(val) Relay1_Write(val)
#define relay_write_charge_end(val) Relay2_Write(val)

#define relay_read_bus(val) Relay1_Read()
#define relay_read_charge_end(val) Relay2_Read()

volatile uint8 bus_command;

void initialize_charging(void);
void control_precharge(void);
extern uint16_t vdriver_lut[9];


typedef struct
{
	uint16_t v_bus;
	uint16_t v_batt;
    uint16_t i_bus;
    uint16_t v_driver;
} adc_sample_t;


/* `#END` */

void tsk_analog_Start(void);
uint32_t CT1_Get_Current(uint8_t channel);
float CT1_Get_Current_f(uint8_t channel);
uint16_t get_max(void);
void i2t_set_limit(uint32_t const_current, uint32_t ovr_current, uint32_t limit_ms);
void i2t_set_warning(uint8_t percent);
void i2t_reset();
void reconfig_charge_timer();
uint8_t callback_pid(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
extern adc_sample_t *ADC_active_sample_buf;
/*
 * Add user function prototypes in the below merge region to add user
 * functionality to the task definition.
 */
/* `#START USER_TASK_PROTOS` */

/* `#END` */

/* ------------------------------------------------------------------------ */
#endif
/* [] END OF FILE */
