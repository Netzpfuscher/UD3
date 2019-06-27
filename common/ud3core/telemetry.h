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
    
#define BUS_OFF 0
#define BUS_CHARGING 1
#define BUS_READY 2
#define BUS_TEMP1_FAULT 3
#define BUS_TEMP2_FAULT 4
#define BUS_TEMP3_FAULT 5
#define BUS_BATT_OV_FLT 6
#define BUS_BATT_UV_FLT 7
    
 
/*
*Teslaterm Configuration
*/
    
#define GAUGE0_NAME "Bus Voltage"
#define GAUGE0_MIN 0
#define GAUGE0_MAX 600
#define GAUGE0_VAR telemetry.bus_v
#define GAUGE0_SLOW 0
        
#define GAUGE1_NAME "Temperature"
#define GAUGE1_MIN 0
#define GAUGE1_MAX 100
#define GAUGE1_VAR telemetry.temp1
#define GAUGE1_SLOW 1

#define GAUGE2_NAME "Power"
#define GAUGE2_MIN 0
#define GAUGE2_MAX 10000
#define GAUGE2_VAR telemetry.avg_power
#define GAUGE2_SLOW 0
        
#define GAUGE3_NAME "Current"
#define GAUGE3_MIN 0
#define GAUGE3_MAX 50
#define GAUGE3_VAR telemetry.batt_i /10
#define GAUGE3_SLOW 0
    
#define GAUGE4_NAME "Primary Curr."
#define GAUGE4_MIN 0
#define GAUGE4_MAX 1000
#define GAUGE4_VAR telemetry.primary_i
#define GAUGE4_SLOW 0

#define GAUGE5_NAME "Voices"
#define GAUGE5_MIN 0
#define GAUGE5_MAX 4        
#define GAUGE5_VAR telemetry.midi_voices
#define GAUGE5_SLOW 0
 
/*    
#define GAUGE5_NAME "Fres"
#define GAUGE5_MIN 0
#define GAUGE5_MAX 1000        
#define GAUGE5_VAR telemetry.fres
#define GAUGE5_SLOW 0
*/    
#define GAUGE6_NAME "Fuse"
#define GAUGE6_MIN 0
#define GAUGE6_MAX 100
#define GAUGE6_VAR telemetry.i2t_i
#define GAUGE6_SLOW 0
    
#define CHART0_NAME "Bus Voltage"
#define CHART0_MIN 0
#define CHART0_MAX 600
#define CHART0_OFFSET 0
#define CHART0_UNIT TT_UNIT_V
#define CHART0_VAR telemetry.bus_v
        
#define CHART1_NAME "Temperature"
#define CHART1_MIN 0
#define CHART1_MAX 100
#define CHART1_OFFSET 0
#define CHART1_UNIT TT_UNIT_C
#define CHART1_VAR telemetry.temp1
        
#define CHART2_NAME "Primary Curr."
#define CHART2_MIN 0
#define CHART2_MAX 1000
#define CHART2_OFFSET 0
#define CHART2_UNIT TT_UNIT_A
#define CHART2_VAR telemetry.primary_i        
        
#define CHART3_NAME "Power"
#define CHART3_MIN 0
#define CHART3_MAX 10000
#define CHART3_OFFSET 0
#define CHART3_UNIT TT_UNIT_W
#define CHART3_VAR telemetry.avg_power   
    
    
typedef struct
{
	uint16 bus_v;		//in volts
	uint16 batt_v;		//in volts
	uint8 temp1;		//in *C
	uint8 temp2;		//in *C
	uint8 bus_status;   //0 = charging, 1 = ready
	uint16 avg_power;   //Average power in watts
	uint16 batt_i;		//battery current in centiamps
    uint32 i2t_i;
	uint16_t primary_i;
    uint8_t midi_voices;
    uint16_t duty;
    int16 fres;
} telemetry_struct;
telemetry_struct telemetry;

#endif

//[] END OF FILE
