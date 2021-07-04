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

#if !defined(tsk_PCA9685_H)
#define tsk_PCA9685_H

#include <stdint.h>

typedef struct{
    uint8_t address;
    uint8_t buffer[5];
}PCA9685;

extern uint32_t i2c_bytes_rx;
extern uint32_t i2c_bytes_tx;
    
PCA9685* PCA9685_new(uint8_t address);  
void PCA9685_setPWM_i(PCA9685* ptr, uint8_t led, int on_value, int off_value);
void PCA9685_setPWM(PCA9685* ptr, uint8_t led, uint16_t value);
void PCA9685_reset(PCA9685* ptr);
void PCA9685_setPWMFreq(PCA9685* ptr, int freq);
void PCA9685_init(PCA9685* ptr);


#endif