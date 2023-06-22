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

uint16_t fb_filter_in=1;
uint16_t fb_filter_out=1;

void initialize_ZCD_to_PWM(void) {
	//initialize all the timers/counters/PWMs
	PWMA_Start();
	PWMB_Start();
	FB_capture_Start();
	ZCD_counter_Start();
	FB_glitch_detect_Start();

	//initialize all comparators
	ZCD_compA_Start();
	ZCD_compB_Start();
	CT1_comp_Start();

	//initialize the DACs
	CT1_dac_Start();
	ZCDref_Start();
	FB_THRSH_DAC_Start();
    Opamp_1_Start();

	//start the feedback filter block
	FB_Filter_Start();
	FB_Filter_SetCoherency(FB_Filter_CHANNEL_A, FB_Filter_KEY_MID);

	configure_ZCD_to_PWM();
}

void configure_CT1(void) {

	float max_tr_cl_dac_val_temp;
	float max_qcw_cl_dac_val_temp;
	float min_tr_cl_dac_val_temp;

	//figure out the CT setups
	max_tr_cl_dac_val_temp = (((float)configuration.max_tr_current / (float)configuration.ct1_ratio) * configuration.ct1_burden) / (DAC_VOLTS_PER_STEP * 10);
	if (max_tr_cl_dac_val_temp > 255) {
		max_tr_cl_dac_val_temp = 255;
	}
    
    params.max_tr_cl_dac_val = round(max_tr_cl_dac_val_temp);

	max_qcw_cl_dac_val_temp = (((float)configuration.max_qcw_current / (float)configuration.ct1_ratio) * configuration.ct1_burden) / (DAC_VOLTS_PER_STEP * 10);
	if (max_qcw_cl_dac_val_temp > 255) {
		max_qcw_cl_dac_val_temp = 255;
	}

	min_tr_cl_dac_val_temp = (((float)configuration.min_tr_current / (float)configuration.ct1_ratio) * configuration.ct1_burden) / (DAC_VOLTS_PER_STEP * 10);
	if (min_tr_cl_dac_val_temp > 255) {
		min_tr_cl_dac_val_temp = 255;
	}
	params.min_tr_cl_dac_val = round(min_tr_cl_dac_val_temp);

	ct1_dac_val[0] = params.max_tr_cl_dac_val;
	ct1_dac_val[1] = params.max_tr_cl_dac_val;
	ct1_dac_val[2] = round(max_qcw_cl_dac_val_temp);

	params.diff_tr_cl_dac_val = params.max_tr_cl_dac_val - params.min_tr_cl_dac_val;
}

void configure_CT2(void) {
    if(configuration.ct2_type==CT2_TYPE_CURRENT){
        //DC CT mA_Count
        params.idc_ma_count = (uint32_t)((configuration.ct2_ratio * 50 * 1000) / configuration.ct2_burden) / 4096;
    }else{
        params.ct2_offset_cnt = (uint32_t)(4096ul*(uint32_t)configuration.ct2_offset)/5000ul;
        uint32_t cnt_fs = ((4096ul*(uint32_t)configuration.ct2_voltage)/5000ul)-params.ct2_offset_cnt;
        params.idc_ma_count = (((uint32_t)configuration.ct2_current*100ul) / cnt_fs);   
    }
}

void configure_ZCD_to_PWM(void) {
	//this function calculates the variables used at run-time for the ZCD to PWM hardware
	//it also initializes that hardware so its ready for interrupter control

	
	float pwm_start_prd_temp;
    uint16 fb_glitch_cmp;

	configure_CT1();
	configure_CT2();

	//initialize the ZCD counter
	if (configuration.start_cycles == 0) {
		ZCD_counter_WritePeriod(1);
		ZCD_counter_WriteCompare(1);
        set_switch_without_fb(pdTRUE);
	} else {
		ZCD_counter_WritePeriod(configuration.start_cycles * 2);
		ZCD_counter_WriteCompare(4);
        set_switch_without_fb(pdFALSE);
	}
    
    uint32_t lead_time_temp;
	//calculate lead time in cycles
	lead_time_temp = (configuration.lead_time*1000) / (1000000/BCLK__BUS_CLK__MHZ);

	//calculate starting period
	pwm_start_prd_temp = BCLK__BUS_CLK__HZ / (configuration.start_freq * 200); //why 200? well 2 because its half-periods, and 100 because frequency is in hz*100
	params.pwm_top = round(pwm_start_prd_temp * 2);								  //top value for FB capture and pwm generators to avoid ULF crap
	params.pwma_start_prd = params.pwm_top - lead_time_temp;
	params.pwma_start_cmp = params.pwm_top - pwm_start_prd_temp + 4; //untested, was just 4;
    
	params.pwmb_start_prd = round(pwm_start_prd_temp); //experimental start up mode, 2 cycles of high frequency to start things up but stay ahead of the game.
	params.pwmb_start_cmp = 4;
	params.pwmb_start_psb_val = 15;				   //config.psb_start_val;// params.pwmb_start_prd>>3;
												   //Set up FB_GLITCH PWM
	fb_glitch_cmp = pwm_start_prd_temp / 4; //set the lock out period to be 1/4 of a cycle long
	if (fb_glitch_cmp > 255) {
		fb_glitch_cmp = 255;
	}
	FB_glitch_detect_WritePeriod(255);
	FB_glitch_detect_WriteCompare1(255 - fb_glitch_cmp);
	FB_glitch_detect_WriteCompare2(255);

	//initialize PWM generator
	PWMA_WritePeriod(params.pwma_start_prd);
	PWMA_WriteCompare(params.pwma_start_cmp);

	//start up oscillator
	PWMB_WritePeriod(params.pwmb_start_prd);
	PWMB_WriteCompare(params.pwmb_start_cmp);
	//initialize FB Capture period to account for 10 count offset in timing due to reset delay.
	FB_capture_WritePeriod(params.pwm_top - 10);

	//prime the feedback filter with fake values so that the first time we use it, its not absolutely garbage
	for (uint8_t z = 0; z < 5; z++) {
		FB_Filter_Write24(FB_Filter_CHANNEL_A, params.pwm_top - round(pwm_start_prd_temp));
        CyDelayUs(4);
	}

	//set reference voltage for zero current detector comparators
	ZCDref_Data = 25;
    
    //write value for minimum feedback current
	float FB_THRSH_DAC_value = ((((float)configuration.min_fb_current / (float)configuration.ct1_ratio) * configuration.ct1_burden) / (DAC_VOLTS_PER_STEP * 10));
	if (FB_THRSH_DAC_value > 255) {
		FB_THRSH_DAC_value = 255;
	}
    
    FB_THRSH_DAC_Data = (uint8_t) FB_THRSH_DAC_value;
}

/* [] END OF FILE */
