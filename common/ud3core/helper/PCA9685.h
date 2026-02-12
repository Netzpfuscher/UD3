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
 * @file PCA9685.h
 * @brief Driver for PCA9685 16-channel 12-bit PWM controller
 *
 * I2C-controlled PWM driver IC commonly used for LED dimming and servo control.
 * Used in UD3 for controlling analog outputs or external peripherals.
 */

#if !defined(tsk_PCA9685_H)
#define tsk_PCA9685_H

#include <stdint.h>

/**
 * @brief PCA9685 device instance
 */
typedef struct{
    uint8_t address;   //!< I2C device address (7-bit)
    uint8_t buffer[5]; //!< I2C write buffer for PWM commands
}PCA9685;

extern uint32_t i2c_bytes_rx;  //!< Total I2C bytes received (statistics)
extern uint32_t i2c_bytes_tx;  //!< Total I2C bytes transmitted (statistics)
    
/**
 * @brief Create a new PCA9685 device instance
 *
 * Allocates memory and initializes the PCA9685 controller to default settings.
 *
 * @param address I2C device address (7-bit, typically 0x40-0x7F)
 * @return Pointer to allocated PCA9685 instance, or NULL on allocation failure
 */
PCA9685* PCA9685_new(uint8_t address);  
/**
 * @brief Set PWM output with explicit on/off times
 *
 * Allows precise control of PWM phase and duty cycle by specifying the
 * counter values when the output turns on and off within the 4096-step cycle.
 *
 * @param ptr Pointer to PCA9685 instance
 * @param led Channel number (0-15)
 * @param on_value Counter value (0-4095) when output turns on
 * @param off_value Counter value (0-4095) when output turns off
 */
void PCA9685_setPWM_i(PCA9685* ptr, uint8_t led, int on_value, int off_value);
/**
 * @brief Set PWM duty cycle for a channel
 *
 * Simplified PWM control - output turns on at count 0 and off at specified value.
 *
 * @param ptr Pointer to PCA9685 instance
 * @param led Channel number (0-15)
 * @param value Duty cycle (0-4095), clamped to valid range
 */
void PCA9685_setPWM(PCA9685* ptr, uint8_t led, uint16_t value);
/**
 * @brief Reset PCA9685 to default mode
 *
 * Sets MODE1 to normal operation and MODE2 to totem-pole output.
 *
 * @param ptr Pointer to PCA9685 instance
 */
void PCA9685_reset(PCA9685* ptr);
/**
 * @brief Set PWM output frequency
 *
 * Configures the PWM frequency by setting the prescaler register.
 * Valid range depends on the internal 25MHz oscillator: ~24Hz to ~1526Hz.
 *
 * @param ptr Pointer to PCA9685 instance
 * @param freq Desired PWM frequency in Hz
 */
void PCA9685_setPWMFreq(PCA9685* ptr, int freq);
/**
 * @brief Initialize PCA9685 to UD3 defaults
 *
 * Performs reset, sets PWM frequency to 1kHz, and enables auto-increment mode.
 * Called automatically by PCA9685_new().
 *
 * @param ptr Pointer to PCA9685 instance
 */
void PCA9685_init(PCA9685* ptr);


#endif