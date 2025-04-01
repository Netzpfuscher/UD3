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


#if !defined(qcw_H)
#define qcw_H
    
#include <device.h>
#include "cli_common.h"
#include "TTerm.h"
#include "helper/teslaterm.h"
    
typedef struct
{
    uint8_t changed;
    uint16_t index;
    uint8_t data[400];
} ramp_params;

ramp_params volatile ramp; //added volatile

typedef struct
{
    uint32_t time_start;
    uint32_t time_stop;
    uint32_t last_time;
} timer_params;


timer_params timer;

void qcw_start();
void qcw_modulate(uint16_t val);
void qcw_stop();
void qcw_regenerate_ramp();
void qcw_handle();   
void qcw_handle_synth();

void qcw_cmd_midi_pulse(int32_t volume, int32_t frequencyTenths);

void qcw_ramp_visualize(CHART *chart, TERMINAL_HANDLE * handle);
void qcw_ramp_line(uint16_t x0,uint8_t y0,uint16_t x1, uint8_t y1);
void qcw_ramp_point(uint16_t x,uint8_t y);
    
uint8_t CMD_ramp(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
uint8_t callback_rampFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
BaseType_t QCW_delete_timer(void);    
    
#endif