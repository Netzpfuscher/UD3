/**
 * @file version.h
 * @brief Protocol version and capability declarations for UD3 firmware
 *
 * This file defines the communication protocol version and firmware capabilities
 * advertised to host applications during connection. The version array is transmitted
 * as part of the initial handshake, allowing clients (Teslaterm, UD3-node) to detect
 * firmware features and adjust their behavior accordingly.
 *
 * Key information:
 * - **Protocol version**: Major.minor version (currently 3.0)
 * - **Build timestamp**: Compile date and time
 * - **Timing parameters**: Timebase and count direction
 * - **Feature flags**: Supported optional features
 *
 * Version history:
 * - Protocol 3.0: Added MIN multi-client support, SID support, no-telemetry mode
 * - Protocol 2.x: Legacy single-client protocol
 *
 * @note Version strings parsed by host software - format must remain compatible
 * @note Changes to protocol require incrementing protocol version number
 */

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

#if !defined(version_H)
#define version_H

/**
 * @brief Firmware version and capability strings
 *
 * Array of key=value strings transmitted during connection handshake. Host applications
 * parse these strings to determine firmware capabilities and adjust their behavior.
 *
 * String format: "key=value"
 *
 * Defined capabilities:
 * - **protocol**: Communication protocol version (major.minor format)
 * - **build_time**: Firmware compilation timestamp (MMM DD YYYY;HH:MM:SS)
 * - **timebase**: Microseconds per timer tick (1000 = 1ms ticks)
 * - **time_count**: Timer count direction ("up" or "down")
 * - **notelemetry_supported**: Can disable telemetry transmission (1 = yes, 0 = no)
 * - **min_sid_support**: SID (C64 audio) playback support (1 = yes, 0 = no)
 *
 * @note Adding new capability flags is backward-compatible (old clients ignore unknown keys)
 * @note Changing protocol version requires client updates for compatibility
 */
static const char *version[] = {
	"protocol=3.0",                                  /**< Protocol version 3.0 (MIN multi-client) */
	"build_time=" __DATE__ ";" __TIME__,            /**< Build timestamp from compiler */
	"timebase=1000",                                 /**< 1ms (1000µs) timer resolution */
	"time_count=up",                                 /**< Timer counts upward from 0 */
	"notelemetry_supported=1",                       /**< Supports disabling telemetry (TT_NO_TELEMETRY) */
	"min_sid_support=1"                              /**< SID (Commodore 64) audio playback supported */
};

#endif
