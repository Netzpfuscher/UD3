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

#ifndef TSK_HWGAUGE_INC
#define TSK_HWGAUGE_INC
    
#define NUM_HWGAUGE 6
#define HWGAUGE_DEFAULT 0xffffffff
#define HWGAUGE_CURRVERSION 1
    
#include <stdint.h>
#include <device.h>
#include "cli_basic.h"
#include "telemetry.h"
#include "TTerm.h"  

typedef struct{
    TELE* src;
    uint8_t version;
    uint32_t tele_hash;
    int16_t calMin;
    int16_t calMax;
    int32_t scalMin;
    int32_t scalMax;
}HWGauge_s;

typedef union{
    HWGauge_s gauge[NUM_HWGAUGE];
    uint16_t rawData[(sizeof(HWGauge_s) * NUM_HWGAUGE) / 2];
} HWGauge;
    
extern HWGauge hwGauges;

void tsk_hwGauge_init(); 
void tsk_hwGauge_proc();
uint32_t HWGauge_getGaugeColor(uint8_t id);
void HWGauge_setValue(uint32_t number, int32_t value);
int32_t HWGauge_scaleTelemetry(TELE * src, int32_t cMin, int32_t cMax, unsigned allowNegative);
uint8_t callback_hwGauge(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);

uint8_t CMD_hwGauge(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
    
#endif