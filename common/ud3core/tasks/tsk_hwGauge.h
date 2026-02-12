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

/**
 * @file tsk_hwGauge.h
 * @brief Hardware gauge control task
 *
 * Controls external hardware gauges via PCA9685 PWM driver.
 * Maps telemetry values to PWM outputs with calibration and scaling.
 */

#ifndef TSK_HWGAUGE_INC
#define TSK_HWGAUGE_INC
    
#define NUM_HWGAUGE 6           //!< Number of hardware gauges supported
#define HWGAUGE_DEFAULT 0xffffffff //!< Default unconfigured value
#define HWGAUGE_CURRVERSION 1   //!< Current gauge configuration version
    
#include <stdint.h>
#include <device.h>
#include "cli_basic.h"
#include "telemetry.h"
#include "TTerm.h"  

/**
 * @brief Hardware gauge configuration structure
 */
typedef struct{
    TELE* src;          //!< Source telemetry pointer
    uint8_t version;    //!< Configuration version
    uint32_t tele_hash; //!< Telemetry name hash
    int16_t calMin;     //!< Calibration minimum (PWM counts)
    int16_t calMax;     //!< Calibration maximum (PWM counts)
    int32_t scalMin;    //!< Scale minimum (telemetry value)
    int32_t scalMax;    //!< Scale maximum (telemetry value)
}HWGauge_s;

/**
 * @brief Hardware gauge array union (for EEPROM storage)
 */
typedef union{
    HWGauge_s gauge[NUM_HWGAUGE]; //!< Gauge array
    uint16_t rawData[(sizeof(HWGauge_s) * NUM_HWGAUGE) / 2]; //!< Raw EEPROM data
} HWGauge;
    
extern HWGauge hwGauges; //!< Global gauge configuration

/**
 * @brief Initialize hardware gauge task
 */
void tsk_hwGauge_init();

/**
 * @brief Hardware gauge task procedure
 */
void tsk_hwGauge_proc();

/**
 * @brief Get gauge color (reserved for future use)
 * @param id Gauge ID
 * @return Color value
 */
uint32_t HWGauge_getGaugeColor(uint8_t id);

/**
 * @brief Set gauge PWM value directly
 * @param number Gauge number
 * @param value PWM value
 */
void HWGauge_setValue(uint32_t number, int32_t value);

/**
 * @brief Scale telemetry value to calibrated range
 * @param src Telemetry source
 * @param cMin Calibration min
 * @param cMax Calibration max
 * @param allowNegative Allow negative scaling
 * @return Scaled PWM value
 */
int32_t HWGauge_scaleTelemetry(TELE * src, int32_t cMin, int32_t cMax, unsigned allowNegative);

/**
 * @brief Parameter callback for gauge settings
 * @param params Parameter array
 * @param index Parameter index
 * @param handle Terminal handle
 * @return pdTRUE on success
 */
uint8_t callback_hwGauge(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);

/**
 * @brief Hardware gauge CLI command
 * @param handle Terminal handle
 * @param argCount Argument count
 * @param args Argument array
 * @return Command status
 */
uint8_t CMD_hwGauge(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
    
#endif