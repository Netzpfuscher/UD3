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
 * @file cli_common.h
 * @brief CLI parameter system and command interface for UD3 Tesla coil controller
 *
 * Defines the configuration and parameter structures, CLI command handlers,
 * and the parameter registration system for the UD3 DRSSTC controller.
 */

#ifndef CLI_COMMON_H
#define CLI_COMMON_H

#include <device.h>
#include "cli_basic.h"

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "TTerm.h"
#include "tasks/tsk_thermistor.h"

/**
 * @brief Initialize configuration parameters to default values
 */
void init_config();

/**
 * @brief Load configuration from EEPROM and apply settings
 * @param handle Terminal handle for output messages
 */
void eeprom_load(TERMINAL_HANDLE * handle);

/**
 * @brief Clear screen command (legacy)
 * @param commandline Command line string
 * @param ptr Port structure pointer
 * @return Command execution status
 */
uint8_t command_cls(char *commandline, port_str *ptr);

/**
 * @brief Set UART baudrate and reconfigure hardware
 * @param baudrate Desired baudrate in bps
 */
void uart_baudrate(uint32_t baudrate);

/**
 * @brief Set SPI speed
 * @param speed SPI clock speed
 */
void spi_speed(uint32_t speed);

/**
 * @brief Update parameter visibility based on configuration
 *
 * Dynamically shows/hides parameters based on current settings
 * (e.g., CT2 type selects which CT2 parameters are visible)
 */
void update_visibilty(void);

/** @brief Display signal states and diagnostic info */
uint8_t CMD_signals(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Transient mode commands */
uint8_t CMD_tr(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Trigger single pulse */
uint8_t CMD_oneshot(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Connection and MIN protocol statistics */
uint8_t CMD_con(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Display alarm history */
uint8_t CMD_alarms(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Jump to bootloader for firmware update */
uint8_t CMD_bootloader(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Control bus power on/off */
uint8_t CMD_bus(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Calibrate drive voltage measurement */
uint8_t CMD_calib(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Send feature list to Teslaterm */
uint8_t CMD_features(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Get configuration for Teslaterm */
uint8_t CMD_config_get(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Load/save EEPROM configuration */
uint8_t CMD_eeprom(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Emergency kill/unkill interrupter */
uint8_t CMD_udkill(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Fuse configuration */
uint8_t CMD_fuse(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Restore default parameters */
uint8_t CMD_load_defaults(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Get parameter value(s) */
uint8_t CMD_get(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Set parameter value */
uint8_t CMD_set(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief QCW mode commands */
uint8_t CMD_qcw(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Control user relays 3/4 */
uint8_t CMD_relay(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Set PWM duty for relays 3/4 */
uint8_t CMD_pwm(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Software reset controller */
uint8_t CMD_reset(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
/** @brief Display hardware revision */
uint8_t CMD_hwrev(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);

/**
 * @brief Send colored signal state with newline
 * @param signal Signal value (0 or 1)
 * @param inverted If true, invert signal before display
 * @param handle Terminal handle for output
 */
void send_signal_state_new(uint8_t signal, uint8_t inverted, TERMINAL_HANDLE * handle);

/**
 * @brief Send colored signal state without newline
 * @param signal Signal value (0 or 1)
 * @param inverted If true, invert signal before display
 * @param handle Terminal handle for output
 */
void send_signal_state_wo_new(uint8_t signal, uint8_t inverted, TERMINAL_HANDLE * handle);

/** @brief Global parameter table containing all configurable parameters */
extern parameter_entry confparam[];

/**
 * @brief Configuration structure stored in EEPROM
 *
 * Contains all persistent configuration parameters including safety limits,
 * hardware calibration, and system settings. Saved/loaded via EEPROM commands.
 */
struct config_struct{
    uint8_t watchdog; /**< Watchdog enable flag */
    uint16_t watchdog_timeout; /**< Watchdog timeout in milliseconds */
    uint16_t max_tr_pw; /**< Maximum transient mode pulse width in microseconds */
    uint16_t max_tr_prf; /**< Maximum transient mode PRF in Hz */
    uint16_t max_qcw_pw; /**< Maximum QCW pulse width in 10us units */
    uint16_t max_tr_current; /**< Maximum transient mode current in amperes */
    uint16_t min_tr_current; /**< Minimum transient mode current in amperes */
    uint16_t max_qcw_current; /**< Maximum QCW current in amperes */
    uint8_t temp1_max; /**< Maximum temperature 1 threshold in degrees C */
    uint8_t temp2_max; /**< Maximum temperature 2 threshold in degrees C */
    uint16_t ct1_ratio; /**< CT1 (feedback) current transformer ratio (N turns) */
    uint16_t ct2_ratio; /**< CT2 (bus) current transformer ratio (N turns) */
    uint16_t ct1_burden; /**< CT1 burden resistor in ohms (div 10) */
    uint16_t ct2_burden; /**< CT2 burden resistor in ohms (div 10) */
    uint16_t lead_time; /**< ZCD to PWM lead time in nanoseconds */
    uint16_t start_freq; /**< Resonant start frequency in 10Hz units */
    uint8_t  start_cycles; /**< Number of start cycles before feedback */
    uint16_t max_tr_duty; /**< Maximum TR duty cycle in percent (div 10) */
    uint16_t max_qcw_duty; /**< Maximum QCW duty cycle in percent (div 10) */
    uint16_t temp1_setpoint; /**< Fan control temperature setpoint in degrees C */
    uint16_t temp2_setpoint; /**< Temperature 2 setpoint in degrees C */
    uint16_t pid_temp_set; /**< PID temperature controller setpoint */
    uint8_t pid_temp_mode; /**< PID temperature mode (0=disabled, 1-4=various modes) */
    float pid_temp_p; /**< Temperature PID proportional gain */
    float pid_temp_i; /**< Temperature PID integral gain */
    uint8_t temp2_mode; /**< Temp2 control mode (0=off, 1=fan, 3=relay3, 4=relay4) */
    uint8_t ps_scheme; /**< Power supply control scheme */
    uint8_t autotune_s; /**< Number of samples for autotune */
    char ud_name[16]; /**< User-defined coil name (15 chars + null) */
    uint32_t baudrate; /**< Serial UART baudrate */
    uint8_t ct2_type; /**< CT2 type: 0=current, 1=voltage */
    uint16_t ct2_current; /**< CT2 current at ct2_voltage (div 10) */
    uint16_t ct2_voltage; /**< CT2 voltage at ct2_current (div 1000) */
    uint16_t ct2_offset; /**< CT2 offset voltage (div 1000) */
    uint32_t r_top; /**< Bus voltage divider top resistor in ohms (div 1000) */
    uint16_t chargedelay; /**< Charge relay delay in milliseconds */
    uint8_t ivo_uart; /**< UART invert option: [RX][TX] bits */
    uint8_t ext_interrupter; /**< External interrupter enable (0=off, 1=on, 2=inverted) */
    uint8_t is_qcw; /**< QCW coil mode flag (0=TR, 1=QCW) */
    uint8_t pca9685; /**< PCA9685 PWM controller enable */
    uint16_t max_fb_errors; /**< Max feedback errors per second before fault (0=off) */
    uint16_t ntc_b; /**< NTC thermistor beta coefficient in kelvin */
    uint16_t ntc_r25; /**< NTC resistance at 25C in ohms (div 1000) */
    uint16_t idac; /**< iDAC measured current in microamperes */
    uint8_t ivo_led; /**< LED invert option */
    uint16_t uvlo_analog; /**< Analog UVLO threshold (div 1000, 0=GPIO UVLO) */
    float vdrive; /**< Drive voltage setpoint for digipot */
    uint8_t hw_rev; /**< Hardware revision (0=3.0-3.1a, 1=3.1b, 2=3.1c) */
    uint8_t autostart; /**< Autostart flag */
    uint8_t min_fb_current; /**< Current threshold to switch to feedback mode */
    
    uint8_t SigGen_minOtOffset; /**< Signal generator minimum offtime offset */
    
    uint8_t compressor_attac; /**< Duty compressor attack setting */
    uint8_t compressor_sustain; /**< Duty compressor sustain setting */
    uint8_t compressor_release; /**< Duty compressor release setting */
    uint8_t compressor_maxDutyOffset; /**< Max duty offset before hard limit */
    
    float drive_factor; /**< Drive voltage measurement calibration factor */
};
typedef struct config_struct cli_config;

/**
 * @brief Runtime parameter structure (not saved to EEPROM)
 *
 * Contains runtime parameters that control interrupter operation but are
 * not persisted across resets unless explicitly saved.
 */
struct parameter_struct{
    uint16_t    pw; /**< Pulse width in microseconds */
    uint16_t    pwd; /**< Pulse period in microseconds */
    uint16_t    vol; /**< Volume (0-MAX_VOL) */
    uint16_t    tune_start; /**< Autotune start frequency in 10Hz units */
    uint16_t    tune_end; /**< Autotune end frequency in 10Hz units */
    uint16_t    tune_pw; /**< Autotune pulse width */
    uint16_t    tune_delay; /**< Autotune delay between steps */
    uint16_t    offtime; /**< Offtime for MIDI (minimum 3) */
    uint16_t    qcw_ramp; /**< QCW ramp increment per 125us (div 100) */
    uint8_t     qcw_holdoff; /**< QCW time before ramp starts in 125us units */
    uint8_t     qcw_offset; /**< QCW ramp start value */
    uint8_t     qcw_max; /**< QCW ramp end value */
    uint16_t    qcw_repeat; /**< QCW repeat interval in ms (<100=single shot) */
    uint16_t    qcw_freq; /**< QCW modulation frequency in 10Hz units */
    uint8_t     qcw_vol; /**< QCW modulation volume */
    uint16_t    qcw_pw; /**< QCW pulse width in 10us units */
    uint16_t    burst_on; /**< Burst mode on-time in ms (0=off) */
    uint16_t    burst_off; /**< Burst mode off-time in ms */
    uint8_t     synth; /**< Synth mode (0=off, 1=MIDI, 2=SID, 3=TR) */
    uint16_t    temp_duty; /**< Temperature-limited duty cycle */
};
typedef struct parameter_struct cli_parameter;

/** @brief Global configuration structure instance */
extern cli_config configuration;
/** @brief Global runtime parameter structure instance */
extern cli_parameter param;

/** @brief Size of configuration parameter array */
#define CONF_SIZE sizeof(confparam) / sizeof(parameter_entry)

/**
 * @brief Autocomplete handler for parameter names
 * @param handle Terminal handle
 * @param params Parameter pointer (unused)
 * @return Number of autocomplete candidates
 */
uint8_t complete_parameter_name(TERMINAL_HANDLE * handle, void * params);

#endif
