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
 * @file tsk_analog.h
 * @brief Analog task - ADC sampling and bus voltage control
 *
 * Handles continuous ADC sampling via DMA for bus voltage, battery voltage,
 * currents, and gate driver voltage. Manages bus precharge/relay control
 * for various power supply schemes. Runs at 8 kHz sample rate.
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
#include "config.h"

#define CT_PRIMARY 0   //!< Primary current transformer
#define CT_SECONDARY 1 //!< Secondary current transformer
 

#define AC_PRECHARGE_TIMEOUT 100  //!< Timeout for AC precharge detection (cycles)

/** @name Power Supply Schemes
 * Bus voltage control/precharge schemes
 * @{
 */
#define BAT_PRECHARGE_BUS_SCHEME   0   //!< Battery directly supplies bus voltage
#define BAT_BOOST_BUS_SCHEME       1   //!< Battery boosted via SLR converter
#define AC_PRECHARGE_BUS_SCHEME    2   //!< AC mains with precharge relay
#define AC_DUAL_MEAS_SCHEME        3   //!< Dual voltage measurement (input + capacitor)
#define AC_NO_RELAY_BUS_SCHEME     4   //!< AC mains without precharge relay
#define AC_PRECHARGE_FIXED_DELAY   5   //!< AC mains with fixed delay precharge
/** @} */

/** @name Relay States
 * @{
 */
#define RELAY_CHARGE 1     //!< Charging relay on
#define RELAY_OFF 0        //!< Relay off
#define RELAY_CHARGE_OFF 2 //!< Charge relay off
#define RELAY_ON 3         //!< Relay on
/** @} */

/** @name Bus Commands
 * Commands for bus voltage control
 * @{
 */
#define BUS_COMMAND_OFF 0   //!< Turn bus off
#define BUS_COMMAND_ON 1    //!< Turn bus on
#define BUS_COMMAND_FAULT 2 //!< Bus fault state
/** @} */

#if RELAY1_INVERTED
    #define relay_write_bus(val) Relay1_Write(val ? 0 : 1)
    #define relay_read_bus(val) (Relay1_Read() ? 0 : 1)
#else
    #define relay_write_bus(val) Relay1_Write(val)
    #define relay_read_bus(val) Relay1_Read()
#endif

#if RELAY2_INVERTED
    #define relay_write_charge_end(val) Relay2_Write(val ? 0 : 1)
    #define relay_read_charge_end(val) (Relay2_Read() ? 0 : 1)
#else
    #define relay_write_charge_end(val) Relay2_Write(val)
    #define relay_read_charge_end(val) Relay2_Read()
#endif

volatile uint8 bus_command; //!< Current bus control command

/**
 * @brief Initialize charging state machine
 */
void initialize_charging(void);

/**
 * @brief Control bus precharge logic (called at 8 kHz)
 */
void control_precharge(void);

extern uint16_t vdriver_lut[9]; //!< Gate driver voltage lookup table


/**
 * @brief ADC sample structure (DMA buffer element)
 */
typedef struct
{
	uint16_t v_bus;    //!< Bus voltage ADC count
	uint16_t v_batt;   //!< Battery voltage ADC count
    uint16_t i_bus;    //!< Bus current ADC count
    uint16_t v_driver; //!< Gate driver voltage ADC count
} adc_sample_t;


/* `#END` */

/**
 * @brief Start analog task
 */
void tsk_analog_Start(void);

/**
 * @brief Get primary current transformer reading
 * @return Current in mA
 */
uint32_t CT1_Get_Current();

/**
 * @brief Get primary current as float
 * @return Current in amperes
 */
float CT1_Get_Current_f();

/**
 * @brief Get maximum ADC value
 * @return Maximum ADC count
 */
uint16_t get_max(void);

/**
 * @brief Reconfigure charge timer period
 */
void reconfig_charge_timer();

/**
 * @brief Parameter callback for PID settings
 * @param params Parameter array
 * @param index Parameter index
 * @param handle Terminal handle
 * @return pdTRUE on success
 */
uint8_t callback_pid(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
adc_sample_t* tsk_analog_get_readable_buffer();

/**
 * @brief Read gate driver voltage in millivolts
 * @return Voltage in mV
 */
uint16_t read_driver_mv();

/**
 * @brief Recalculate driver top resistor with correction factor
 * @param factor Scaling factor for resistor divider
 */
void tsk_analog_recalc_drive_top(float factor);

extern adc_sample_t ADC_sample_buf_0[ADC_BUFFER_CNT]; //!< ADC DMA buffer 0
extern adc_sample_t ADC_sample_buf_1[ADC_BUFFER_CNT]; //!< ADC DMA buffer 1
extern SemaphoreHandle_t adc_ready_Semaphore;         //!< Semaphore signaled when ADC buffer ready

/*
 * Add user function prototypes in the below merge region to add user
 * functionality to the task definition.
 */
/* `#START USER_TASK_PROTOS` */

/* `#END` */

/* ------------------------------------------------------------------------ */
#endif
/* [] END OF FILE */
