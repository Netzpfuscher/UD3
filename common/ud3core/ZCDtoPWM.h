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

#define CPU_CLK_PERIOD 1000.0/BCLK__BUS_CLK__MHZ  
#define DAC_VOLTS_PER_STEP 0.016

typedef struct
{
	uint8 max_tr_cl_dac_val;
	uint16 pwma_start_prd;
	uint16 pwma_start_cmp;
	uint16 pwmb_start_prd;
	uint16 pwmb_start_cmp;
	uint16 pwm_top;
	uint16 pwmb_psb_prd;
	uint16 pwmb_psb_val;
	uint16 pwmb_start_psb_val;
	uint16 min_tr_prd;
	uint8 min_tr_cl_dac_val;
	uint8 diff_tr_cl_dac_val;
	uint16_t idc_ma_count;
    uint16_t ct2_offset_cnt;
} parameters;
volatile parameters params;

//variables read by DMA for PWM stuffs
extern uint16_t fb_filter_in;
extern uint16_t fb_filter_out;
uint8_t ct1_dac_val[3];

void initialize_ZCD_to_PWM(void);
void configure_ZCD_to_PWM(void);
void configure_CT1(void);
void configure_CT2(void);

#endif

//[] END OF FILE
