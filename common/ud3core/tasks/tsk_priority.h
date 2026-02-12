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
 * @file tsk_priority.h
 * @brief FreeRTOS task priorities and stack sizes
 *
 * Defines priority levels and stack allocations for all system tasks.
 * Higher priority values indicate higher task priority.
 */

#if !defined(PRIORITY_H)
#define PRIORITY_H

/** @name Task Priority Levels
 * FreeRTOS task priorities (higher = more important)
 * @{
 */
#define PRIO_TERMINAL 3    //!< Terminal/CLI task priority
#define PRIO_OVERLAY 1     //!< Telemetry overlay display priority
#define PRIO_ANALOG 1      //!< ADC sampling task priority
#define PRIO_THERMISTOR 1  //!< Temperature monitoring priority
#define PRIO_DUTY 4        //!< Duty cycle measurement priority (high)
#define PRIO_UART 3        //!< UART communication priority
#define PRIO_USB 3         //!< USB CDC communication priority
#define PRIO_ETH 3         //!< Ethernet communication priority
#define PRIO_FAULT 4       //!< Fault monitoring priority (critical)
#define PRIO_MIDI 2        //!< MIDI processing priority
#define PRIO_QCW 3         //!< QCW mode control priority
#define PRIO_DISPLAY 1     //!< Display update priority
#define PRIO_I2C 1         //!< I2C communication priority
/** @} */

/** @name Task Stack Sizes
 * Stack sizes in 32-bit words for each task
 * @{
 */
#define STACK_TERMINAL 256   //!< Terminal task stack (256 words = 1KB)
#define STACK_OVERLAY 256    //!< Overlay task stack (256 words = 1KB)
#define STACK_ANALOG 128     //!< Analog task stack (128 words = 512 bytes)
#define STACK_THERMISTOR 100 //!< Thermistor task stack (100 words = 400 bytes)
#define STACK_UART 256       //!< UART task stack (256 words = 1KB)
#define STACK_MIN 256        //!< MIN protocol task stack (256 words = 1KB)
#define STACK_USB 128        //!< USB task stack (128 words = 512 bytes)
#define STACK_ETH 256        //!< Ethernet task stack (256 words = 1KB)
#define STACK_FAULT 100      //!< Fault task stack (100 words = 400 bytes)
#define STACK_MIDI 200       //!< MIDI task stack (200 words = 800 bytes)
#define STACK_DISPLAY 200    //!< Display task stack (200 words = 800 bytes)
#define STACK_I2C 200        //!< I2C task stack (200 words = 800 bytes)
/** @} */

/** @name Basic Bit Manipulation Macros
 * Portable bit manipulation utilities
 * @{
 */

/**
 * @brief Set bit y of x to 1
 * @param x Variable to modify
 * @param y Bit position (0-indexed)
 */
#define SET(x, y) x |= (1 << y)

/**
 * @brief Clear bit y of x to 0
 * @param x Variable to modify
 * @param y Bit position (0-indexed)
 */
#define CLEAR(x, y) x &= ~(1 << y)

/**
 * @brief Read bit y of x
 * @param x Variable to read
 * @param y Bit position (0-indexed)
 * @return 1 if bit is set, 0 if clear
 */
#define READ(x, y) ((0u == (x & (1 << y))) ? 0u : 1u)

/**
 * @brief Toggle bit y of x
 * @param x Variable to modify
 * @param y Bit position (0-indexed)
 */
#define TOGGLE(x, y) (x ^= (1 << y))
/** @} */

#endif