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

#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <device.h>
#include "interrupter.h"
#include "tasks/tsk_min.h"
    
#define BUS_OFF 0
#define BUS_CHARGING 1
#define BUS_READY 2
#define BUS_TEMP1_FAULT 3
#define BUS_TEMP2_FAULT 4
#define BUS_TEMP3_FAULT 5
#define BUS_BATT_OV_FLT 6
#define BUS_BATT_UV_FLT 7
    
#define TT_NO_TELEMETRY -1
    
#define TT_SLOW 0
#define TT_FAST 1
    
#define N_GAUGES 7
#define N_CHARTS 4
   
typedef struct __tele__ {
    int32_t value;
    uint8_t unit;
    char * name;
    uint32_t min;
    uint32_t max;
    int32_t offset;
    int16_t divider;
    uint16_t resend_time;
    uint8_t high_res;
    int8_t gauge;
    int8_t chart;
} TELE;


typedef struct __tele_human__ {
    TELE bus_v;
    TELE batt_v;
    TELE driver_v;
    TELE temp1;
    TELE temp2;
    TELE bus_status;
    TELE avg_power;
    TELE batt_i;
    TELE i2t_i;
    TELE primary_i;
    TELE midi_voices;
    TELE fres;
    TELE tx_datarate;
    TELE rx_datarate;
    TELE dutycycle;
} TELE_HUMAN;

#define N_TELE sizeof(TELE_HUMAN)/sizeof(TELE)

typedef union telemetry_u
{
    TELE_HUMAN n;
    TELE a[N_TELE];
}TELEMETRY;


extern TELEMETRY tt;
#endif

//[] END OF FILE
