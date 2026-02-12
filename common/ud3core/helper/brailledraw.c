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
 * @file brailledraw.c
 * @brief Braille graphics implementation
 *
 * This implements a monochrome graphic display on a VT100 terminal. Lines and
 * points are drawn by setting a bit (one bit per pixel) in an offscreen pixmap.
 * This is later translated to characters on the terminal to "draw" the buffer.
 *
 * The characters used are the Braille characters (U+2800 ... U+28FF) which contains
 * all 256 possible patterns of an 8x8-dot Braille cell.
 *
 * Reference: See https://github.com/asciimoo/drawille for a Python implementation.
 */

//-----------------------------------------------------------------------------
// This implements a monochrome graphic display on a VT100 terminal.  Lines and 
// points are drawn by setting a bit (one bit per pixel) in an offscreen pixmap.
// This is later translated to characters on the terminal to "draw" the buffer.
//
// The characters used are the Braille characters (U+2800 ... U+28FF) which contains 
// all 256 possible patterns of an 8x8-dot Braille cell.
//
// Reference: See https://github.com/asciimoo/drawille for a Python implementation.
//-----------------------------------------------------------------------------

#include "brailledraw.h"
#include "cli_common.h"
#include <stdlib.h>

/**
 * Braille traditionally uses a 6 dot cell (2 wide by 3 tall). Unicode stores these at
 * 0x2800 to 0x283f. It was then extended to a 2 wide by 4 cell and the extended patterns
 * are stored at 0x2840 to 0x28ff. See https://en.wikipedia.org/wiki/Braille_Patterns for
 * a nice chart.
 *
 * Braille pixel numbers:
 * <pre>
 * |1 4|
 * |2 5|
 * |3 6|
 * |7 8|
 * </pre>
 *
 * Converts a pixel in a 4x2 grid to an offset from Unicode 0x2800. So if a single pixel
 * is set in row 2, col 1 of the 4x2 grid, the corresponding Unicode char is 0x2800 + 0x20 = 0x2820.
 */
const unsigned char pixmap[4][2] = {
	{0x01, 0x08},
	{0x02, 0x10},
	{0x04, 0x20},
	{0x40, 0x80}};

uint8_t *pix;  //!< Pixel buffer (column-major order)

/** @brief Convert x,y coordinates to linear pixel buffer address (column-major) */
#define pix_select(x,y)  pix[x*PIX_HEIGHT+y]

/**
 * @brief Report memory allocation error
 *
 * @param handle Terminal handle for output
 */
void raise_memory_error(TERMINAL_HANDLE * handle){
    ttprintf("WARNING: NULL pointer; malloc failed\r\n");
}

/**
 * @brief Convert Braille pattern offset to 3-byte UTF-8 sequence
 *
 * To encode a char in the range of 0x2800-0x28FF, the UTF-8 format is:
 * 1110xxxx 10xxxxxx 10xxxxxx where the x's correspond to the 16 bits being encoded.
 *
 * The range 0x2800-0x28FF encodes as: 11100010 101000xx 10xxxxxx where x's
 * correspond to the 'c' argument.
 *
 * @param pbuffer Output buffer (must be at least 3 bytes)
 * @param c Braille pattern offset from 0x2800 (0x00-0xFF)
 */
static void set_bytes(char *pbuffer, const unsigned char c) {
	pbuffer[0] = (char)0xE2;
    
    /** @todo Could be replaced by simpler/faster:
     * pbuffer[1] = (char)(0xa0 | (c >> 6));
     * pbuffer[2] = (char)(0x80 | (c & 0x3f));
     * Leaving for now because "if it ain't broke, don't fix it"
     */
    
	if (c & pixmap[3][0] && c & pixmap[3][1]) {
		pbuffer[1] = (char)0xA3;
	} else if (c & pixmap[3][1]) {
		pbuffer[1] = (char)0xA2;
	} else if (c & pixmap[3][0]) {
		pbuffer[1] = (char)0xA1;
	} else {
		pbuffer[1] = (char)0xA0;
	}

	pbuffer[2] = (char)((0xBF & c) | 0x80);
}

void braille_malloc(TERMINAL_HANDLE * handle){
    if(pix != NULL)
       raise_memory_error(handle);  // Someone forgot to call braille_free() first?
    
    pix = pvPortMalloc(sizeof(uint8_t)*PIX_HEIGHT*(PIX_WIDTH / 8));
    if(pix==NULL)
        raise_memory_error(handle);
}

void braille_free(TERMINAL_HANDLE * handle){
    if(pix==NULL){
        raise_memory_error(handle);
    }else{
        vPortFree(pix);
    }
}

void braille_setPixel(uint8_t x, uint8_t y) {
	if (x > PIX_WIDTH - 1)
		x = PIX_WIDTH - 1;
	if (y > PIX_HEIGHT - 1)
		y = PIX_HEIGHT - 1;
	uint8_t byte = x / 8;
	uint8_t bit = x % 8;
    if(pix==NULL) return;
    pix_select(byte,y) |= 1 << bit;
}

/**
 * @brief Draw line using Bresenham's algorithm
 *
 * Draws a series of pixels from (x0,y0) to (x1,y1).
 */
void braille_line(int x0, int y0, int x1, int y1) {

	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2, e2;

	for (;;) {
		braille_setPixel(x0, y0);
		if (x0 == x1 && y0 == y1)
			break;
		e2 = err;
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		}
	}
}

void braille_clear(void) {
    /** @todo Would be faster to memset(pix, 0, (PIX_WIDTH / 8) * PIX_HEIGHT); */
    if(pix!=NULL){
        for (uint8_t x = 0; x < PIX_WIDTH / 8; x++) {
		    for (uint8_t y = 0; y < PIX_HEIGHT; y++) {
                pix_select(x,y) = 0x00;
		    }
	    }  
    }
}

void braille_draw(TERMINAL_HANDLE * handle) {
	unsigned char byte;
    if(pix==NULL){
        raise_memory_error(handle);
        return;
    }
    
    /** Convert pixel buffer to Braille characters.
     * 4 rows of pixels fit in a single Braille char.
     */
	for (uint16_t y = 0; y < PIX_HEIGHT; y += 4) {
		for (uint16_t x = 0; x < PIX_WIDTH / 8; x++) {
            /** 2 columns of pixels fit in a single Braille char, so step by 2 pixels. */
			for (uint16_t bpos = 0; bpos < 8; bpos += 2) {
                /** Convert a 4 tall by 2 wide block of pixels to a single Braille
                 * Unicode character indexed from 0x2800.
                 */
				byte = 0x00;
                
                /** @todo The following pix_select() calls could be cached in 4 vars in the
                 * outer loop to avoid redundant function calls in the triply nested loop.
                 */
				if (pix_select(x,y) & (1 << bpos))
					byte |= 1 << 0;
				if (pix_select(x,y) & (1 << (bpos + 1)))
					byte |= 1 << 3;

				if (pix_select(x,y+1) & (1 << bpos))
					byte |= 1 << 1;
				if (pix_select(x,y+1) & (1 << (bpos + 1)))
					byte |= 1 << 4;

				if (pix_select(x,y+2) & (1 << bpos))
					byte |= 1 << 2;
				if (pix_select(x,y+2) & (1 << (bpos + 1)))
					byte |= 1 << 5;

				if (pix_select(x,y+3) & (1 << bpos))
					byte |= 1 << 6;
				if (pix_select(x,y+3) & (1 << (bpos + 1)))
					byte |= 1 << 7;
                
                /** Convert the Braille Unicode char to a 3 byte UTF-8 sequence and output to the terminal. */
                char out_buffer[3];
				set_bytes(out_buffer, byte);
                ttprintb(out_buffer, sizeof(out_buffer));
                
			}
		}
		ttprintf("\r\n");
	}
}
