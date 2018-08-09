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

#include "charging.h"
#include "cli_common.h"
#include "telemetry.h"
#include <device.h>
#include <math.h>

uint32 low_battery_counter;
uint8 charger_pwm;
int16 initial_vbus, final_vbus, delta_vbus;
uint32 charging_counter;

void initialize_charging(void) {
	telemetry.bus_status = BUS_OFF;
	if ((configuration.ps_scheme == BAT_BOOST_BUS_SCHEME) || (configuration.ps_scheme == AC_OR_BATBOOST_SCHEME)) {
		SLR_Control = 0;
		SLRPWM_Start();
		if (configuration.slr_fswitch == 0) {
			configuration.slr_fswitch = 500; //just in case it wasnt ever programmed
		}
		uint16 x;
		x = (320000 / (configuration.slr_fswitch));
		if ((x % 2) != 0)
			x++; //we want x to be even
		SLRPWM_WritePeriod(x + 1);
		SLRPWM_WriteCompare(x >> 1);
	}
	initial_vbus = 0;
	final_vbus = 0;
	charging_counter = 0;
}

void control_precharge(void) { //this gets called from analogs.c when the ADC data set is ready, 8khz rep rate
	uint32 v_threshold;
	static uint8_t cnt = 0;

	if (bus_command == BUS_COMMAND_ON) {
		if (configuration.ps_scheme == BAT_PRECHARGE_BUS_SCHEME) {
			v_threshold = (float)telemetry.batt_v * 0.95;
			if (telemetry.batt_v >= (configuration.batt_lockout_v - 1)) {
				if (telemetry.bus_v >= v_threshold) {
					relay_Write(RELAY_ON);
					telemetry.bus_status = BUS_READY;
				} else if (telemetry.bus_status != BUS_READY) {
					telemetry.bus_status = BUS_CHARGING;
					relay_Write(RELAY_CHARGE);
				}
				low_battery_counter = 0;
			} else {
				low_battery_counter++;
				if (low_battery_counter > LOW_BATTERY_TIMEOUT) {
					relay_Write(RELAY_OFF);
					telemetry.bus_status = BUS_BATT_UV_FLT;
					bus_command = BUS_COMMAND_FAULT;
				}
			}
		}

		else if (configuration.ps_scheme == BAT_BOOST_BUS_SCHEME) {
			if (telemetry.bus_v < (configuration.slr_vbus - 15)) {
				SLR_Control = 1;
				telemetry.bus_status = BUS_READY; //its OK to operate the TC when charging from SLR
			} else if (telemetry.bus_v > configuration.slr_vbus) {
				SLR_Control = 0;
				telemetry.bus_status = BUS_READY;
			}

			if (telemetry.batt_v >= (configuration.batt_lockout_v - 1)) {
				low_battery_counter = 0;
			} else {
				low_battery_counter++;
				if (low_battery_counter > LOW_BATTERY_TIMEOUT) {
					SLR_Control = 0;
					telemetry.bus_status = BUS_BATT_UV_FLT;
					bus_command = BUS_COMMAND_FAULT;
				}
			}
		}

		else if (configuration.ps_scheme == AC_PRECHARGE_BUS_SCHEME) {
			//we cant know the AC line voltage so we will watch the bus voltage climb and infer when its charged by it not increasing fast enough
			//this logic is part of a charging counter
			if (charging_counter == 0)
				initial_vbus = telemetry.bus_v;
			charging_counter++;
			if (charging_counter > AC_PRECHARGE_TIMEOUT) {

				final_vbus = telemetry.bus_v;
				delta_vbus = final_vbus - initial_vbus;
				if ((delta_vbus < 4) && (telemetry.bus_v > 20)) {
					relay_Write(RELAY_ON);
					telemetry.bus_status = BUS_READY;
				} else if (telemetry.bus_status != BUS_READY) {
					relay_Write(RELAY_CHARGE);
					telemetry.bus_status = BUS_CHARGING;
				}
				charging_counter = 0;
			}
		}

		else if (configuration.ps_scheme == AC_OR_BATBOOST_SCHEME) {
			//this routine determines if the battery or the AC line should be used
			//first, assume there is mains voltage
			relay_Write(RELAY_CHARGE);
			telemetry.bus_status = BUS_CHARGING;
			//test if this is true by watching the bus voltage increase
			if (charging_counter == 0)
				initial_vbus = telemetry.bus_v;
			charging_counter++;
			if (charging_counter > CHARGE_TEST_TIMEOUT) {
				final_vbus = telemetry.bus_v;
				delta_vbus = final_vbus - initial_vbus;
				if (delta_vbus > 4)
					configuration.ps_scheme = AC_PRECHARGE_BUS_SCHEME;
				//if this fails, check, is there battery voltage?
				else if (telemetry.batt_v >= (configuration.batt_lockout_v - 1)) {
					configuration.ps_scheme = BAT_BOOST_BUS_SCHEME;
					relay_Write(RELAY_OFF);
				} else
					bus_command = BUS_COMMAND_FAULT;
				charging_counter = 0;
			}
		}
	} else {
		if (relay_Read()) {
			relay_Write(RELAY_CHARGE);
			if (cnt > 100) {
				relay_Write(RELAY_OFF);
				telemetry.bus_status = BUS_OFF;
				cnt = 0;
			}
			cnt++;
		}
		SLR_Control = 0;
	}
}

/* [] END OF FILE */
