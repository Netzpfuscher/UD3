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
#include "helper/teslaterm.h"
    
typedef struct
{
    uint8 changed;
    uint16 index;
    uint8 data[400];
} ramp_params;

ramp_params volatile ramp; //added volatile

typedef struct
{
    uint32 time_start;
    uint32 time_stop;
    uint32 last_time;
} timer_params;


timer_params timer;

void qcw_start();
void qcw_modulate(uint16_t val);
void qcw_stop();
void qcw_regenerate_ramp();
void qcw_handle();   
void qcw_handle_synth();

void qcw_ramp_visualize(CHART *chart, port_str *ptr);
void qcw_ramp_line(uint16_t x0,uint8_t y0,uint16_t x1, uint8_t y1);
void qcw_ramp_point(uint16_t x,uint8_t y);
    
uint8_t qcw_command_ramp(char *commandline, port_str *ptr);
    
    
    
    
#endif