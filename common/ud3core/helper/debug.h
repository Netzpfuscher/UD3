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
 * @file debug.h
 * @brief Debug console utilities for UD3
 *
 * Provides debug command for accessing MIN protocol debug output and remote terminals.
 */

#if !defined(debug_H)
#define debug_H

#include <device.h>
#include "cli_basic.h"
#include "TTerm.h"
    
extern TERMINAL_HANDLE * debug_port;  //!< Current terminal handle for debug output (NULL if not active)
extern uint8_t debug_id;               //!< MIN ID for debug target (0xFF if not active)

/**
 * @brief Debug command handler
 *
 * Provides access to MIN protocol debugging and remote terminal sessions.
 *
 * Usage:
 * - `debug [id]` - Enter debug mode for specified MIN connection ID
 * - `debug min` - Show MIN protocol debug output
 * - `debug error` - Inject FIFO sequence number error (testing)
 * - `debug fn` - Enter Fibernet debug mode
 *
 * @param handle Terminal handle
 * @param argCount Number of arguments
 * @param args Argument array
 * @return TERM_CMD_EXIT_SUCCESS
 */
uint8_t CMD_debug(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);

#endif