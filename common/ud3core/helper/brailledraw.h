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
 * @file brailledraw.h
 * @brief Monochrome bitmap graphics renderer for VT100 terminals using Braille characters
 *
 * Provides a simple graphics API for drawing pixels and lines on a VT100 terminal
 * using Unicode Braille patterns (U+2800-U+28FF). Each Braille character encodes
 * an 8-dot (2x4) pattern, allowing a 128x64 pixel display in a 64x16 character grid.
 */

#include <device.h>
#include "TTerm.h"

#define PIX_HEIGHT 64L  //!< Display height in pixels
#define PIX_WIDTH 128L  //!< Display width in pixels

/**
 * @brief Allocate pixel buffer memory
 *
 * Must be called before any drawing operations. Call braille_free() when done.
 *
 * @param handle Terminal handle for error reporting
 */
void braille_malloc(TERMINAL_HANDLE * handle);
/**
 * @brief Free pixel buffer memory
 *
 * @param handle Terminal handle for error reporting
 */
void braille_free(TERMINAL_HANDLE * handle);
/**
 * @brief Render pixel buffer to terminal using Braille characters
 *
 * Converts the internal bitmap to UTF-8 Braille patterns and outputs them
 * to the terminal. Does not clear the buffer afterward.
 *
 * @param handle Terminal handle for output
 */
void braille_draw(TERMINAL_HANDLE * handle);
/**
 * @brief Clear pixel buffer to all zeros
 */
void braille_clear(void);
/**
 * @brief Draw a line using Bresenham's algorithm
 *
 * @param x0 Start X coordinate (0-127)
 * @param y0 Start Y coordinate (0-63)
 * @param x1 End X coordinate (0-127)
 * @param y1 End Y coordinate (0-63)
 */
void braille_line(int x0, int y0, int x1, int y1);
/**
 * @brief Set a single pixel in the buffer
 *
 * Coordinates are automatically clamped to valid range.
 *
 * @param x X coordinate (0-127)
 * @param y Y coordinate (0-63)
 */
void braille_setPixel(uint8_t x, uint8_t y);
