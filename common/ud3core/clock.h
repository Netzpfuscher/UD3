/**
 * @file clock.h
 * @brief System time tracking and synchronization module
 *
 * Implements a high-precision 64-bit timestamp counter with trimming capability
 * for synchronization with external time sources. Used for MIDI timing and
 * system event timestamping.
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

#if !defined(clock_H)
#define clock_H
    
#include <device.h>

/** @brief Low 32-bit timestamp in milliseconds (upper half of h_time) */
extern volatile uint32_t l_time;

/** @brief High-precision 64-bit timestamp counter (fixed-point format) */
extern volatile uint64_t h_time;

/**
 * @brief Increment the system clock by one tick
 *
 * Advances h_time by the current increment value and updates l_time.
 * Called periodically by timer interrupt (typically 1ms period).
 */
void clock_tick();

/**
 * @brief Reset clock increment to nominal value
 *
 * Resets the increment to default (0x20000000) and clears trim adjustments.
 */
void clock_reset_inc();

/**
 * @brief Set absolute time value
 * @param time Time value in milliseconds to set
 *
 * Sets h_time to the specified value (shifted left 32 bits for fixed-point format).
 */
void clock_set(uint32_t time);

/**
 * @brief Trim clock rate for synchronization
 * @param trim Difference between local and reference time (used for PI controller)
 *
 * Adjusts the clock increment using a PI controller to synchronize with external
 * time source (e.g., MIDI clock). Positive diff speeds up clock, negative slows down.
 */
void clock_trim(int32_t trim);

    
    
#endif
/* [] END OF FILE */
