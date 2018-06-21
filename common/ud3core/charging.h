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

#ifndef CHARGING_H
#define CHARGING_H

#include <device.h>

#define LOW_BATTERY_TIMEOUT 20000 //8khz ticks, 40000 = 5sec
#define CHARGE_TEST_TIMEOUT 80000 //10 sec
#define AC_PRECHARGE_TIMEOUT 100  //

#define BAT_PRECHARGE_BUS_SCHEME 0 //battery "directly" supplies bus voltage
#define BAT_BOOST_BUS_SCHEME 1	 //battery is boosted via SLR converter
#define AC_PRECHARGE_BUS_SCHEME 2  //no batteries, mains powered
#define AC_OR_BATBOOST_SCHEME 3	//might be battery powered, if not, then mains powered

#define BATTERY 0
#define BUS 1
#define RELAY_CHARGE 1
#define RELAY_OFF 0
#define RELAY_ON 3

#define BUS_COMMAND_OFF 0
#define BUS_COMMAND_ON 1
#define BUS_COMMAND_FAULT 2

volatile uint8 bus_command;

void initialize_charging(void);
void run_battery_charging(void);
void control_precharge(void);

#endif

//[] END OF FILE
