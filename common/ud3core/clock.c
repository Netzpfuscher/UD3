/**
 * @file clock.c
 * @brief System time tracking and synchronization implementation
 *
 * Provides high-precision timestamp counter with PI-based trim control
 * for synchronization with external time sources.
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

#include "clock.h"

/** @brief Nominal increment per tick (0x20000000 = ~536ms per increment when shifted) */
static const int32_t n_increment = 0x20000000;

/** @brief High-precision 64-bit timestamp (upper 32 bits = milliseconds) */
volatile uint64_t h_time;

/** @brief Millisecond timestamp (upper 32 bits of h_time) */
volatile uint32_t l_time;

/** @brief Current increment value per tick (adjusted by trim) */
volatile uint32_t increment = 0x20000000;

/**
 * @brief Increment the system clock by one tick
 *
 * Advances h_time by the current increment value and extracts the upper
 * 32 bits as the millisecond timestamp. Called periodically by timer ISR.
 */
void clock_tick(){
    h_time += increment;
    l_time = h_time >> 32;
}

/**
 * @brief Reset clock increment to nominal value
 *
 * Clears any trim adjustments by resetting increment to nominal value.
 * Used when synchronization is disabled or reset.
 */
void clock_reset_inc(){
    increment = n_increment;
}

/**
 * @brief Set absolute time value
 * @param time Time value in milliseconds
 *
 * Sets the clock to an absolute time value by shifting the input
 * left 32 bits to form the fixed-point representation.
 */
void clock_set(uint32_t time){
    h_time = (uint64_t)time << 32;
}

/**
 * @brief Trim clock rate using PI controller
 * @param diff Time difference between local and reference clock
 *
 * Adjusts clock increment using proportional and integral terms:
 * - Proportional: diff * 1000000 (immediate correction)
 * - Integral: accumulated (diff * 4096) with clamping at ±40960000
 *
 * This creates a feedback loop to synchronize with external time sources
 * like MIDI clock. Positive diff speeds up clock, negative slows it down.
 */
void clock_trim(int32_t diff){
    static int32_t i=0;
    int32_t p=0;
    i += diff * 4096;
    if(i>40960000) i=40960000;
    if(i<-40960000) i=-40960000;
    p = diff* 1000000;
    increment = n_increment +p+i;
}


/* [] END OF FILE */
