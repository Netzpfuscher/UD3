/**
 * @file ZCDtoPWM.h
 * @brief Zero-Crossing Detection to PWM bridge for resonant frequency tracking
 *
 * This module implements the critical feedback control system that locks the interrupter
 * frequency to the Tesla coil's resonant frequency. It uses zero-crossing detection (ZCD)
 * on the primary current waveform to dynamically adjust PWM timing for optimal energy transfer.
 *
 * Key features:
 * - **Resonant frequency tracking**: Detects primary current zero-crossings and adjusts PWM
 * - **Current limiting**: CT1 (primary current) and CT2 (DC current) sensing with DAC thresholds
 * - **Feedback filtering**: Hardware digital filter smooths frequency measurements
 * - **Startup sequencing**: Configurable startup frequency and phase lead time
 * - **Glitch detection**: Lockout period prevents false triggering during switching transients
 *
 * Hardware architecture:
 * - **ZCD comparators**: Detect zero-crossing of primary current (CT1)
 * - **CT1 comparator**: Current limit protection (peak primary current)
 * - **CT2 measurement**: DC bus current or voltage feedback
 * - **PWMA**: Main gate drive PWM (with phase lead compensation)
 * - **PWMB**: Startup oscillator (fixed frequency until feedback locks)
 * - **FB_capture**: Captures zero-crossing period for frequency calculation
 * - **FB_Filter**: Hardware digital filter for period smoothing
 * - **DACs**: Set reference voltages for comparators (ZCD threshold, current limits)
 *
 * Operating principle:
 * 1. Start with fixed frequency (PWMB) for initial oscillation buildup
 * 2. Detect primary current zero-crossings via comparators
 * 3. Measure half-period using FB_capture counter
 * 4. Filter period measurement with hardware digital filter
 * 5. Update PWMA period to match resonant frequency (with phase lead)
 * 6. Switch from PWMB to PWMA feedback-controlled PWM
 *
 * @note This is hardware-specific (PSoC analog/digital blocks) - simulator stubs only
 * @note Critical for safe operation - misconfiguration can damage hardware
 */

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

#ifndef ZCDTOPWM_H
#define ZCDTOPWM_H

#include <device.h>

/**
 * @brief CPU clock period in nanoseconds
 *
 * Calculated from bus clock frequency. Used for timing calculations.
 */
#define CPU_CLK_PERIOD 1000.0 / BCLK__BUS_CLK__MHZ

/**
 * @brief DAC resolution in volts per step
 *
 * PSoC 8-bit DAC: 4.096V full scale / 256 steps = 0.016V per step
 */
#define DAC_VOLTS_PER_STEP 0.016

/**
 * @brief Runtime parameters for ZCD and PWM control
 *
 * These parameters are calculated from user configuration during `configure_ZCD_to_PWM()`
 * and used by hardware blocks (PWMs, comparators, DACs) during operation.
 */
typedef struct {
	uint8 max_tr_cl_dac_val;    /**< Maximum transient current limit DAC value (0-255) */
	uint16 pwma_start_prd;       /**< PWMA initial period (timer counts) */
	uint16 pwma_start_cmp;       /**< PWMA initial compare value (timer counts) */
	uint16 pwmb_start_prd;       /**< PWMB startup period (timer counts) */
	uint16 pwmb_start_cmp;       /**< PWMB startup compare value (timer counts) */
	uint16 pwm_top;              /**< Maximum PWM period (prevents ultra-low frequency) */
	uint16 pwmb_psb_prd;         /**< PWMB phase shift B period (unused in current code) */
	uint16 pwmb_psb_val;         /**< PWMB phase shift B value (unused in current code) */
	uint16 pwmb_start_psb_val;   /**< PWMB startup phase shift value */
	uint16 min_tr_prd;           /**< Minimum transient period (unused in current code) */
	uint8 min_tr_cl_dac_val;    /**< Minimum transient current limit DAC value (0-255) */
	uint8 diff_tr_cl_dac_val;   /**< Difference between max and min current limits */
	uint16_t idc_ma_count;       /**< DC current measurement scaling factor (mA per ADC count) */
	uint16_t ct2_offset_cnt;     /**< CT2 offset in ADC counts (for voltage measurement mode) */
} parameters;
extern volatile parameters params;

/**
 * @brief Feedback filter input value (raw period measurement)
 *
 * Written by FB_capture hardware, read by DMA to feed into FB_Filter.
 */
extern uint16_t fb_filter_in;

/**
 * @brief Feedback filter output value (smoothed period)
 *
 * Output from hardware digital filter, read by DMA to update PWMA period.
 */
extern uint16_t fb_filter_out;
extern uint8_t ct1_dac_val[3];

/**
 * @brief Initialize all ZCD and PWM hardware blocks
 *
 * Starts all PSoC hardware components needed for resonant frequency tracking:
 * - PWM generators (PWMA, PWMB)
 * - Zero-crossing comparators (ZCD_compA, ZCD_compB)
 * - Current limit comparator (CT1_comp)
 * - DACs (CT1_dac, ZCDref, FB_THRSH_DAC)
 * - Feedback capture timer (FB_capture)
 * - Feedback digital filter (FB_Filter)
 * - Glitch detection timer (FB_glitch_detect)
 * - Operational amplifier (Opamp_1)
 *
 * After initialization, calls `configure_ZCD_to_PWM()` to set runtime parameters.
 *
 * @note Hardware-specific (PSoC Creator components) - simulator provides stubs
 * @note Must be called before starting interrupter
 */
void initialize_ZCD_to_PWM(void);

/**
 * @brief Configure ZCD and PWM parameters from user settings
 *
 * Calculates runtime parameters from configuration struct and writes them to hardware.
 * Updates:
 * - PWM periods and compare values (startup frequency, phase lead)
 * - Current limit DAC thresholds (CT1, CT2)
 * - Zero-crossing counter period
 * - Feedback filter initialization
 * - Glitch detection lockout period
 *
 * Algorithm:
 * 1. Calculate CT1/CT2 current limit DAC values from config (max/min current, CT ratios)
 * 2. Calculate startup PWM period from start_freq (fixed frequency for initial buildup)
 * 3. Calculate PWMA period/compare with phase lead compensation
 * 4. Calculate feedback capture top value (prevents ultra-low frequency)
 * 5. Prime feedback filter with startup period (avoid transient on first lock)
 *
 * @note Called during initialization and when configuration changes
 * @note Interrupter must be stopped before calling (modifies active hardware)
 */
void configure_ZCD_to_PWM(void);

/**
 * @brief Configure CT1 current transformer parameters
 *
 * Calculates DAC threshold values for primary current limiting based on:
 * - `configuration.max_tr_current`: Maximum transient mode current (A)
 * - `configuration.max_qcw_current`: Maximum QCW mode current (A)
 * - `configuration.min_tr_current`: Minimum transient mode current (A)
 * - `configuration.ct1_ratio`: CT turns ratio (primary:secondary)
 * - `configuration.ct1_burden`: Burden resistor value (ohms)
 *
 * Formula: `DAC_value = (current_limit / ct1_ratio * ct1_burden) / (DAC_VOLTS_PER_STEP * 10)`
 *
 * Updates `params.max_tr_cl_dac_val`, `params.min_tr_cl_dac_val`, `ct1_dac_val[]` array.
 *
 * @note DAC values clamped to 0-255 range
 * @note Called by `configure_ZCD_to_PWM()`
 */
void configure_CT1(void);

/**
 * @brief Configure CT2 current/voltage measurement parameters
 *
 * CT2 supports two modes (configured by `configuration.ct2_type`):
 *
 * **CT2_TYPE_CURRENT** (DC current measurement):
 * - Calculates `params.idc_ma_count`: Scaling factor for ADC counts to mA
 * - Formula: `idc_ma_count = (ct2_ratio * 50mV * 1000) / (ct2_burden * 4096 ADC counts)`
 *
 * **CT2_TYPE_VOLTAGE** (DC voltage measurement):
 * - Calculates `params.ct2_offset_cnt`: ADC count offset for zero voltage
 * - Calculates `params.idc_ma_count`: Scaling factor for ADC counts to current equivalent
 * - Uses voltage divider parameters (`ct2_offset`, `ct2_voltage`, `ct2_current`)
 *
 * @note CT2_TYPE_VOLTAGE mode requires calibration parameters in configuration
 * @note Called by `configure_ZCD_to_PWM()`
 */
void configure_CT2(void);

#endif

//[] END OF FILE
