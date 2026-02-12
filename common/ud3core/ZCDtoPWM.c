/**
 * @file ZCDtoPWM.c
 * @brief Implementation of zero-crossing detection and PWM control for resonant frequency tracking
 *
 * This file implements the configuration and initialization of the hardware feedback control
 * system that locks the interrupter to the Tesla coil's resonant frequency.
 *
 * Key algorithms:
 * - **CT configuration**: Converts current limits (amperes) to DAC threshold voltages
 * - **PWM calculation**: Converts startup frequency to timer period/compare values
 * - **Phase lead compensation**: Advances switching timing for optimal energy transfer
 * - **Filter priming**: Initializes digital filter to avoid startup transients
 *
 * Hardware initialization sequence:
 * 1. Start all PWM, comparator, DAC, and filter blocks
 * 2. Calculate runtime parameters from configuration
 * 3. Write parameters to hardware registers
 * 4. Prime feedback filter with startup frequency
 * 5. System ready for interrupter control
 *
 * @see ZCDtoPWM.h for detailed hardware architecture and operating principle
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

#include "ZCDtoPWM.h"
#include "cli_common.h"
#include "interrupter.h"
#include "tasks/tsk_fault.h"
#include <device.h>
#include <math.h>

/**
 * @brief Feedback filter input buffer (raw period measurements)
 *
 * Initialized to 1 to avoid division by zero during startup.
 */
uint16_t fb_filter_in = 1;

/**
 * @brief Feedback filter output buffer (smoothed period)
 *
 * Initialized to 1 to avoid division by zero during startup.
 */
uint16_t fb_filter_out = 1;

void initialize_ZCD_to_PWM(void) {
	//Start PWM generators for gate drive signals
	PWMA_Start();
	PWMB_Start();

	//Start feedback capture timer (measures zero-crossing period)
	FB_capture_Start();

	//Start zero-crossing detection counter (startup cycle counting)
	ZCD_counter_Start();

	//Start glitch detection timer (prevents false triggering)
	FB_glitch_detect_Start();

	//Start comparators for zero-crossing detection
	ZCD_compA_Start();
	ZCD_compB_Start();

	//Start current limit comparator
	CT1_comp_Start();

	//Start DACs for threshold voltages
	CT1_dac_Start();      // Current limit threshold
	ZCDref_Start();       // Zero-crossing reference voltage
	FB_THRSH_DAC_Start(); // Minimum feedback current threshold

	//Start operational amplifier for signal conditioning
	Opamp_1_Start();

	//Start hardware digital filter for period smoothing
	FB_Filter_Start();
	FB_Filter_SetCoherency(FB_Filter_CHANNEL_A, FB_Filter_KEY_MID);

	//Configure all runtime parameters from user settings
	configure_ZCD_to_PWM();
}

void configure_CT1(void) {
	float max_tr_cl_dac_val_temp;
	float max_qcw_cl_dac_val_temp;
	float min_tr_cl_dac_val_temp;

	//Calculate maximum transient current limit DAC value
	//Formula: (current_amps / ct_ratio * burden_ohms) / (volts_per_step * gain)
	//Gain of 10 from amplifier stage
	max_tr_cl_dac_val_temp = (((float)configuration.max_tr_current / (float)configuration.ct1_ratio) * configuration.ct1_burden) / (DAC_VOLTS_PER_STEP * 10);
	if (max_tr_cl_dac_val_temp > 255) {
		max_tr_cl_dac_val_temp = 255; // Clamp to 8-bit DAC range
	}

	params.max_tr_cl_dac_val = round(max_tr_cl_dac_val_temp);

	//Calculate maximum QCW current limit DAC value
	max_qcw_cl_dac_val_temp = (((float)configuration.max_qcw_current / (float)configuration.ct1_ratio) * configuration.ct1_burden) / (DAC_VOLTS_PER_STEP * 10);
	if (max_qcw_cl_dac_val_temp > 255) {
		max_qcw_cl_dac_val_temp = 255;
	}

	//Calculate minimum transient current limit DAC value
	min_tr_cl_dac_val_temp = (((float)configuration.min_tr_current / (float)configuration.ct1_ratio) * configuration.ct1_burden) / (DAC_VOLTS_PER_STEP * 10);
	if (min_tr_cl_dac_val_temp > 255) {
		min_tr_cl_dac_val_temp = 255;
	}
	params.min_tr_cl_dac_val = round(min_tr_cl_dac_val_temp);

	//Populate DAC value array for DMA access
	ct1_dac_val[0] = params.max_tr_cl_dac_val; // Transient mode
	ct1_dac_val[1] = params.max_tr_cl_dac_val; // Transient mode (duplicate)
	ct1_dac_val[2] = round(max_qcw_cl_dac_val_temp); // QCW mode

	//Calculate range for dynamic current limiting
	params.diff_tr_cl_dac_val = params.max_tr_cl_dac_val - params.min_tr_cl_dac_val;
}

void configure_CT2(void) {
	if (configuration.ct2_type == CT2_TYPE_CURRENT) {
		//CT2 in current measurement mode (DC current transformer)
		//Calculate ADC counts per milliamp
		//Formula: (ct_ratio * 50mV_shunt * 1000mA/A) / (burden_ohms * 4096_ADC_counts)
		params.idc_ma_count = (uint32_t)((configuration.ct2_ratio * 50 * 1000) / configuration.ct2_burden) / 4096;
	} else {
		//CT2 in voltage measurement mode (resistive divider)
		//Calculate ADC offset for zero voltage
		params.ct2_offset_cnt = (uint32_t)(4096ul * (uint32_t)configuration.ct2_offset) / 5000ul;

		//Calculate ADC counts for full-scale voltage (above offset)
		uint32_t cnt_fs = ((4096ul * (uint32_t)configuration.ct2_voltage) / 5000ul) - params.ct2_offset_cnt;

		//Calculate scaling factor (current equivalent per ADC count)
		params.idc_ma_count = (((uint32_t)configuration.ct2_current * 100ul) / cnt_fs);
	}
}

void configure_ZCD_to_PWM(void) {
	float pwm_start_prd_temp;
	uint16 fb_glitch_cmp;

	//Configure current transformer thresholds
	configure_CT1();
	configure_CT2();

	//Configure ZCD counter for startup cycle counting
	if (configuration.start_cycles == 0) {
		//Bypass startup cycles - switch directly to feedback mode
		ZCD_counter_WritePeriod(1);
		ZCD_counter_WriteCompare(1);
		set_switch_without_fb(pdTRUE);
	} else {
		//Count startup cycles before switching to feedback
		//Period = start_cycles * 2 (counting both zero-crossings per cycle)
		ZCD_counter_WritePeriod(configuration.start_cycles * 2);
		ZCD_counter_WriteCompare(4);
		set_switch_without_fb(pdFALSE);
	}

	//Calculate phase lead time in timer counts
	//lead_time is in microseconds, convert to CPU clock cycles
	uint32_t lead_time_temp;
	lead_time_temp = (configuration.lead_time * 1000) / (1000000 / BCLK__BUS_CLK__MHZ);

	//Calculate starting PWM period from configured startup frequency
	//start_freq is in Hz*100 (e.g., 10000 = 100.00 Hz)
	//Divide by 200: factor of 2 for half-periods, factor of 100 for frequency scaling
	pwm_start_prd_temp = BCLK__BUS_CLK__HZ / (configuration.start_freq * 200);

	//Calculate top period value (prevents ultra-low frequency operation)
	params.pwm_top = round(pwm_start_prd_temp * 2);

	//PWMA period (feedback-controlled PWM with phase lead compensation)
	params.pwma_start_prd = params.pwm_top - lead_time_temp;
	params.pwma_start_cmp = params.pwm_top - pwm_start_prd_temp + 4;

	//PWMB period (fixed-frequency startup oscillator)
	params.pwmb_start_prd = round(pwm_start_prd_temp);
	params.pwmb_start_cmp = 4;
	params.pwmb_start_psb_val = 15;

	//Configure glitch detection lockout period (1/4 cycle)
	//Prevents false triggering during switching transients
	fb_glitch_cmp = pwm_start_prd_temp / 4;
	if (fb_glitch_cmp > 255) {
		fb_glitch_cmp = 255; // Clamp to 8-bit timer range
	}
	FB_glitch_detect_WritePeriod(255);
	FB_glitch_detect_WriteCompare1(255 - fb_glitch_cmp);
	FB_glitch_detect_WriteCompare2(255);

	//Write calculated values to PWMA hardware
	PWMA_WritePeriod(params.pwma_start_prd);
	PWMA_WriteCompare(params.pwma_start_cmp);

	//Write calculated values to PWMB hardware (startup oscillator)
	PWMB_WritePeriod(params.pwmb_start_prd);
	PWMB_WriteCompare(params.pwmb_start_cmp);

	//Configure feedback capture period (compensate for 10-count reset delay)
	FB_capture_WritePeriod(params.pwm_top - 10);

	//Prime feedback filter with startup period to avoid transient
	//Write 5 samples to initialize filter state
	for (uint8_t z = 0; z < 5; z++) {
		FB_Filter_Write24(FB_Filter_CHANNEL_A, params.pwm_top - round(pwm_start_prd_temp));
		CyDelayUs(4); // Allow filter to process sample
	}

	//Set zero-crossing comparator reference voltage (empirically determined)
	ZCDref_Data = 25;

	//Calculate minimum feedback current threshold DAC value
	//Below this current level, feedback is considered invalid (too weak for reliable ZCD)
	float FB_THRSH_DAC_value = ((((float)configuration.min_fb_current / (float)configuration.ct1_ratio) * configuration.ct1_burden) / (DAC_VOLTS_PER_STEP * 10));
	if (FB_THRSH_DAC_value > 255) {
		FB_THRSH_DAC_value = 255;
	}

	FB_THRSH_DAC_Data = (uint8_t)FB_THRSH_DAC_value;
}

/* [] END OF FILE */
