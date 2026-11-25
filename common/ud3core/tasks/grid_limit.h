/*
 * UD3 - Grid / fuse current limiter
 *
 * Copyright (c) 2026 Jens Kerrinnes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file grid_limit.h
 * @brief Input power / fuse current limiter
 *
 * The limiter derives an estimated grid current from the measured DC-bus
 * power (CT2 bus current times bus voltage), compares it with the configured
 * fuse rating, and reduces the effective duty-cycle limit.  A normalized I2t
 * accumulator provides a slow fuse-thermal model and a hard trip.
 */

#if !defined(GRID_LIMIT_H)
#define GRID_LIMIT_H

#include <stdint.h>

/**
 * @brief Run the grid limiter (call once every 100 ms)
 */
void grid_regulator_update(void);

/**
 * @brief Clear the limiter state, I2t accumulator and trip latch
 */
void grid_regulator_reset(void);

/**
 * @brief Current duty-scale factor, 0..1000 (1000 = unlimited)
 */
uint16_t grid_regulator_get_scale(void);

/**
 * @brief Effective max TR duty in 0.1 % units after grid limiting
 */
uint16_t grid_regulator_effective_max_duty_tenths(void);

/**
 * @brief True while the I2t/severe-overload trip is latched
 */
uint8_t grid_regulator_is_tripped(void);

#endif
