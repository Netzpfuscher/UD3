/*
 * UD3
 *
 * Copyright (c) 2022 Jens Kerrinnes
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
 * @file buffer.h
 * @brief Big-endian binary buffer parsing utilities
 *
 * Helper functions for extracting integers from byte buffers in big-endian (network) byte order.
 * Used for parsing binary protocol messages (e.g., MIN protocol, MIDI).
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdint.h>

/**
 * @brief Extract signed 16-bit integer from buffer (big-endian)
 *
 * @param buf Source buffer
 * @param ind Pointer to current index (will be incremented by 2)
 * @return Extracted int16_t value
 */
int16_t buffer_get_int16(const uint8_t *buf, int32_t *ind);
/**
 * @brief Extract unsigned 16-bit integer from buffer (big-endian)
 *
 * @param buf Source buffer
 * @param ind Pointer to current index (will be incremented by 2)
 * @return Extracted uint16_t value
 */
uint16_t buffer_get_uint16(const uint8_t *buf, int32_t *ind);
/**
 * @brief Extract signed 32-bit integer from buffer (big-endian)
 *
 * @param buf Source buffer
 * @param ind Pointer to current index (will be incremented by 4)
 * @return Extracted int32_t value
 */
int32_t buffer_get_int32(const uint8_t *buf, int32_t *ind);
/**
 * @brief Extract unsigned 32-bit integer from buffer (big-endian)
 *
 * @param buf Source buffer
 * @param ind Pointer to current index (will be incremented by 4)
 * @return Extracted uint32_t value
 */
uint32_t buffer_get_uint32(const uint8_t *buf, int32_t *ind);   
    
#endif /* BUFFER_H_ */