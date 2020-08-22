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
   
 
/*
*Teslaterm Configuration
*/
    
#define GAUGE0_NAME "Bus V"
#define GAUGE0_MIN 0
#define GAUGE0_MAX configuration.r_top / 1000
#define GAUGE0_VAR metering.bus_v->value
#define GAUGE0_SLOW 0
#define GAUGE0_HIGH_RES 0
#define GAUGE0_DIV 0
        
#define GAUGE1_NAME "Temp."
#define GAUGE1_MIN 0
#define GAUGE1_MAX configuration.temp1_max
#define GAUGE1_VAR metering.temp1->value
#define GAUGE1_SLOW 1
#define GAUGE1_HIGH_RES 0
#define GAUGE1_DIV 0

#define GAUGE2_NAME "Power"
#define GAUGE2_MIN 0
#define GAUGE2_MAX (configuration.r_top / 1000) * ((configuration.ct2_ratio * 50) / configuration.ct2_burden)
#define GAUGE2_VAR metering.avg_power->value
#define GAUGE2_SLOW 0
#define GAUGE2_HIGH_RES 1
#define GAUGE2_DIV 0
        
#define GAUGE3_NAME "Current"
#define GAUGE3_MIN 0
#define GAUGE3_MAX ((configuration.ct2_ratio * 50) / configuration.ct2_burden)
#define GAUGE3_VAR metering.batt_i->value
#define GAUGE3_SLOW 0
#define GAUGE3_HIGH_RES 1
#define GAUGE3_DIV 10
    
#define GAUGE4_NAME "P.Curr."
#define GAUGE4_MIN 0
#define GAUGE4_MAX ((5000ul * configuration.ct1_ratio) / configuration.ct1_burden)/100
#define GAUGE4_VAR metering.primary_i->value
#define GAUGE4_SLOW 0
#define GAUGE4_HIGH_RES 0
#define GAUGE4_DIV 0

#define GAUGE5_NAME "Voices"
#define GAUGE5_MIN 0
#define GAUGE5_MAX 4        
#define GAUGE5_VAR metering.midi_voices->value
#define GAUGE5_SLOW 0
#define GAUGE5_HIGH_RES 0
#define GAUGE5_DIV 0

/*    
#define GAUGE5_NAME "Timediff"
#define GAUGE5_MIN -5000
#define GAUGE5_MAX 5000        
#define GAUGE5_VAR time.diff
#define GAUGE5_SLOW 0
#define GAUGE5_HIGH_RES 0
#define GAUGE5_DIV 0
*/    
#define GAUGE6_NAME "Fuse"
#define GAUGE6_MIN 0
#define GAUGE6_MAX 100
#define GAUGE6_VAR metering.i2t_i->value
#define GAUGE6_SLOW 0
#define GAUGE6_HIGH_RES 0
#define GAUGE6_DIV 0
    
#define CHART0_NAME "Bus Voltage"
#define CHART0_MIN 0
#define CHART0_MAX configuration.r_top / 1000
#define CHART0_OFFSET 0
#define CHART0_UNIT TT_UNIT_V
#define CHART0_VAR metering.bus_v->value
        
#define CHART1_NAME "Temperature"
#define CHART1_MIN 0
#define CHART1_MAX configuration.temp1_max
#define CHART1_OFFSET 0
#define CHART1_UNIT TT_UNIT_C
#define CHART1_VAR metering.temp1->value
        
#define CHART2_NAME "Primary Curr."
#define CHART2_MIN 0
#define CHART2_MAX 2*configuration.max_tr_current
#define CHART2_OFFSET 0
#define CHART2_UNIT TT_UNIT_A
#define CHART2_VAR metering.primary_i->value        
        
#define CHART3_NAME "Power"
#define CHART3_MIN 0
#define CHART3_MAX 10000
#define CHART3_OFFSET 0
#define CHART3_UNIT TT_UNIT_W
#define CHART3_VAR metering.avg_power->value   

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
    TELE * bus_v;
    TELE * batt_v;
    TELE * driver_v;
    TELE * temp1;
    TELE * temp2;
    TELE * bus_status;
    TELE * avg_power;
    TELE * batt_i;
    TELE * i2t_i;
    TELE * primary_i;
    TELE * midi_voices;
    TELE * duty;
    TELE * fres;
} TELE_HUMAN;

extern const TELE_HUMAN metering;
#endif

//[] END OF FILE
