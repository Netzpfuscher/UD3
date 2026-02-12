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
 * @file autotune.h
 * @brief Primary coil frequency sweep and resonance detection
 * 
 * Provides automated frequency tuning functionality for finding the
 * primary resonant frequency of DRSSTC coils through current response
 * measurement.
 */

#ifndef AUTOTUNE_H
#define AUTOTUNE_H

#include <device.h>
#include "TTerm.h"

/**
 * @brief Terminal command for primary coil frequency tuning
 * @param handle Terminal handle
 * @param argCount Number of arguments
 * @param args Command arguments
 * @return TERM_CMD_EXIT_SUCCESS
 * 
 * Performs a frequency sweep across the configured range (param.tune_start
 * to param.tune_end) to find the primary resonant frequency by measuring
 * current response. Results are displayed as a graph in VT100 (Braille)
 * or Teslaterm mode.
 * 
 * @warning Hard-switches the bridge - ensure bus voltage is appropriate
 * 
 * Usage: tune
 * 
 * The sweep:
 * - Takes 128 measurements across the frequency range
 * - Uses configured pulse width (param.tune_pw) and delay (param.tune_delay)
 * - Averages multiple samples per frequency (configuration.autotune_s)
 * - Identifies peak current response frequency
 * - Displays graphical results (VT100 Braille or Teslaterm chart)
 */
uint8_t CMD_tune(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);

#endif

//[] END OF FILE
