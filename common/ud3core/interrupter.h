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

/*
The internal_interrupter enables the ZCDtoPWM hardware.  
Its a 16 bit PWM clocked at 1MHz, so thats 1uS per count.

*/
#ifndef INTERRUPTER_H
#define INTERRUPTER_H

#include <device.h>

#define INTERRUPTER_CLK_FREQ 1000000

/* DMA Configuration for int1_dma */
#define int1_dma_BYTES_PER_BURST 8
#define int1_dma_REQUEST_PER_BURST 1
#define int1_dma_SRC_BASE (CYDEV_SRAM_BASE)
#define int1_dma_DST_BASE (CYDEV_PERIPH_BASE)

typedef struct
{
	uint16 pw;
	uint16 prd;
} interrupter_params;

interrupter_params interrupter;

void handle_qcw();
void handle_qcw_synth();

typedef struct
{
	uint16 modulation_value;
	uint32 qcw_recorded_prd;
	uint32 qcw_limiter_pw;
	uint16 shift_period;
} ramp_params;

typedef struct
{
    uint32 time_start;
    uint32 time_stop;
    uint32 last_time;
} timer_params;

ramp_params volatile ramp; //added volatile
timer_params timer;

extern volatile uint8 qcw_dl_ovf_counter;
extern uint8_t tr_running;
extern uint8_t blocked; 

void initialize_interrupter(void);
void update_interrupter();
void ramp_control(void);
void interrupter_oneshot(uint16_t pw, uint8_t vol);
uint8_t interrupter_get_kill(void);
void qcw_start();
void qcw_modulate(uint16_t val);
void qcw_stop();

void interrupter_kill(void);

void interrupter_unkill(void);

#endif
