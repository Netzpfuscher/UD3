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

#include "buffer.h"

int16_t buffer_get_int16(const uint8_t *buf, int32_t *ind) {
	int16_t res =	((uint16_t) buf[*ind]) << 8 |
					((uint16_t) buf[*ind + 1]);
	*ind += 2;
	return res;
}

uint16_t buffer_get_uint16(const uint8_t *buf, int32_t *ind) {
	uint16_t res = 	((uint16_t) buf[*ind]) << 8 |
					((uint16_t) buf[*ind + 1]);
	*ind += 2;
	return res;
}

int32_t buffer_get_int32(const uint8_t *buf, int32_t *ind) {
	int32_t res =	((uint32_t) buf[*ind]) << 24 |
					((uint32_t) buf[*ind + 1]) << 16 |
					((uint32_t) buf[*ind + 2]) << 8 |
					((uint32_t) buf[*ind + 3]);
	*ind += 4;
	return res;
}

uint32_t buffer_get_uint32(const uint8_t *buf, int32_t *ind) {
	uint32_t res =	((uint32_t) buf[*ind]) << 24 |
					((uint32_t) buf[*ind + 1]) << 16 |
					((uint32_t) buf[*ind + 2]) << 8 |
					((uint32_t) buf[*ind + 3]);
	*ind += 4;
	return res;
}