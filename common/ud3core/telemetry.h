/**
 * @file telemetry.h
 * @brief Telemetry data structures and definitions for real-time monitoring
 *
 * This module defines the telemetry system used to transmit real-time sensor and status
 * data to host applications (Teslaterm, UD3-node). Telemetry includes voltage, current,
 * temperature, power, duty cycle, and status information.
 *
 * Key features:
 * - Flexible telemetry item structure with units, ranges, scaling
 * - Union-based access (named fields or array iteration)
 * - Support for gauge and chart assignments in host UI
 * - Bus status state machine definitions
 * - Fast/slow update rate control
 * - High-resolution mode for precision measurements
 *
 * Architecture:
 * - TELE struct: Single telemetry item with metadata (unit, range, divider, etc.)
 * - TELE_HUMAN: Named collection of all telemetry items
 * - TELEMETRY union: Allows access as named struct (tt.n.bus_v) or array (tt.a[i])
 * - Transmitted via MIN protocol to connected clients
 *
 * @note Telemetry transmission handled by tsk_min task
 * @note Update rates controlled by resend_time field in each TELE item
 */

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

/** @defgroup BusStatus Bus Status States
 * @brief Bus voltage state machine states for power supply control
 * @{
 */
#define BUS_OFF 0           /**< Bus power off */
#define BUS_CHARGING 1      /**< Bus capacitors charging */
#define BUS_READY 2         /**< Bus ready for operation */
#define BUS_TEMP1_FAULT 3   /**< Temperature sensor 1 fault */
#define BUS_TEMP2_FAULT 4   /**< Temperature sensor 2 fault */
#define BUS_TEMP3_FAULT 5   /**< Temperature sensor 3 fault */
#define BUS_BATT_OV_FLT 6   /**< Battery overvoltage fault */
#define BUS_BATT_UV_FLT 7   /**< Battery undervoltage fault */
/** @} */
    
/** @defgroup TelemetryRates Telemetry Update Rates
 * @brief Control telemetry transmission frequency
 * @{
 */
#define TT_NO_TELEMETRY -1  /**< Disable telemetry transmission for this item */
#define TT_SLOW 0           /**< Slow update rate (typically 1 Hz) */
#define TT_FAST 1           /**< Fast update rate (typically 10 Hz) */
/** @} */

/** @defgroup TelemetryLimits Telemetry UI Limits
 * @brief Maximum number of gauges and charts in host UI
 * @{
 */
#define N_GAUGES 7          /**< Maximum number of gauges in Teslaterm */
#define N_CHARTS 4          /**< Maximum number of charts in Teslaterm */
/** @} */
   
/**
 * @brief Single telemetry item with metadata
 *
 * Defines a single telemetry value with all information needed for transmission,
 * display, and scaling. Host applications use this metadata to correctly display
 * and interpret the raw value.
 */
typedef struct __tele__ {
	int32_t value;       /**< Raw telemetry value (scaled by divider) */
	uint8_t unit;        /**< Unit type (voltage, current, temperature, etc.) */
	char *name;          /**< Human-readable name for display */
	uint32_t min;        /**< Minimum value for display range */
	uint32_t max;        /**< Maximum value for display range */
	int32_t offset;      /**< Offset applied before divider scaling */
	int16_t divider;     /**< Scaling divider (display_value = (value + offset) / divider) */
	uint16_t resend_time; /**< Milliseconds between transmissions (0 = on change only) */
	uint8_t high_res;    /**< High resolution mode flag (use 32-bit transmission) */
	int8_t gauge;        /**< Gauge assignment in host UI (-1 = none, 0-6 = gauge number) */
	int8_t chart;        /**< Chart assignment in host UI (-1 = none, 0-3 = chart number) */
} TELE;


/**
 * @brief Named collection of all telemetry items
 *
 * Provides named access to each telemetry item for code readability.
 * Used as part of the TELEMETRY union to allow both named and array-based access.
 */
typedef struct __tele_human__ {
	TELE bus_v;         /**< Bus voltage (primary DC bus) */
	TELE batt_v;        /**< Battery voltage */
	TELE driver_v;      /**< Gate driver supply voltage */
	TELE temp1;         /**< Temperature sensor 1 (typically inverter heatsink) */
	TELE temp2;         /**< Temperature sensor 2 (typically transformer/secondary) */
	TELE bus_status;    /**< Bus status state (BUS_OFF, BUS_CHARGING, BUS_READY, etc.) */
	TELE avg_power;     /**< Average power consumption */
	TELE batt_i;        /**< Battery current */
	TELE i2t_i;         /**< I²t accumulated current (overcurrent protection metric) */
	TELE primary_i;     /**< Primary coil current */
	TELE midi_voices;   /**< Number of active MIDI voices (polyphony count) */
	TELE fres;          /**< Resonant frequency (measured or configured) */
	TELE tx_datarate;   /**< Data transmission rate (bytes/sec) */
	TELE rx_datarate;   /**< Data reception rate (bytes/sec) */
	TELE dutycycle;     /**< Current duty cycle (percent * 100) */
} TELE_HUMAN;

/**
 * @brief Total number of telemetry items
 *
 * Automatically calculated from TELE_HUMAN struct size.
 */
#define N_TELE sizeof(TELE_HUMAN) / sizeof(TELE)

/**
 * @brief Telemetry union for dual access methods
 *
 * Allows telemetry data to be accessed either by name (tt.n.bus_v) or by
 * array index (tt.a[i]). Array access is useful for iterating over all
 * telemetry items during transmission or initialization.
 */
typedef union telemetry_u {
	TELE_HUMAN n; /**< Named access to telemetry items (e.g., tt.n.bus_v) */
	TELE a[N_TELE]; /**< Array access for iteration (e.g., tt.a[i]) */
} TELEMETRY;


/**
 * @brief Global telemetry data structure
 *
 * Central telemetry storage accessed by all tasks for reading/updating values.
 * Updated by tsk_analog, tsk_midi, tsk_min, and other monitoring tasks.
 * Transmitted to host applications via MIN protocol.
 *
 * @note Access via named fields (tt.n.bus_v) or array iteration (tt.a[i])
 */
extern TELEMETRY tt;

#endif

//[] END OF FILE
