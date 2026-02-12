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
 * @file i2c_helper.h
 * @brief I2C bus transaction helpers for PSoC hardware
 *
 * Wrapper functions for Cypress PSoC I2C master operations, providing
 * simplified register read/write operations for I2C peripherals.
 */

#if !defined(tsk_i2c_helper_H)
#define tsk_i2c_helper_H

#include <stdint.h>

/**
 * @brief Write single byte to I2C device register
 *
 * @param address I2C device address (7-bit)
 * @param registerAddress Register address within device
 * @param data Data byte to write
 */
void I2C_Write(uint8_t address, uint8_t registerAddress, uint8_t data);
/**
 * @brief Read single byte from I2C device register
 *
 * @param address I2C device address (7-bit)
 * @param registerAddress Register address within device
 * @return Byte read from register
 */
uint8_t I2C_Read(uint8_t address, uint8_t registerAddress);
/**
 * @brief Write multiple bytes to I2C device (blocking)
 *
 * First byte of buffer is typically register address. Waits for transfer completion.
 *
 * @param address I2C device address (7-bit)
 * @param buffer Data buffer (first byte = register address)
 * @param cnt Number of bytes to write
 */
void I2C_Write_Blk(uint8_t address, uint8_t * buffer, uint8_t cnt);
/**
 * @brief Write multiple bytes to I2C device (non-blocking)
 *
 * Initiates transfer and returns immediately without waiting for completion.
 *
 * @param address I2C device address (7-bit)
 * @param buffer Data buffer (first byte = register address)
 * @param cnt Number of bytes to write
 */
void I2C_Write_nBlk(uint8_t address, uint8_t * buffer, uint8_t cnt);

#endif