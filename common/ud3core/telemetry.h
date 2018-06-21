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
    
#define BUS_OFF 0
#define BUS_CHARGING 1
#define BUS_READY 2
#define BUS_TEMP1_FAULT 3
#define BUS_TEMP2_FAULT 4
#define BUS_TEMP3_FAULT 5
#define BUS_BATT_OV_FLT 6
#define BUS_BATT_UV_FLT 7

typedef struct
{
	uint16 bus_v;		//in volts
	uint16 batt_v;		//in volts
	uint16 aux_batt_v;  //not used
	uint8 temp1;		//in *C
	uint8 temp2;		//in *C
	uint8 temp3;		//not used
	uint8 bus_status;   //0 = charging, 1 = ready
	uint16 avg_power;   //Average power in watts
	uint16 bang_energy; //bang energy in joules
	uint16 batt_i;		//battery current in centiamps
	uint8 uvlo_stat;	//uvlo status
	uint16_t primary_i;
    uint8_t midi_voices;
} telemetry_struct;
telemetry_struct telemetry;

#endif

//[] END OF FILE
