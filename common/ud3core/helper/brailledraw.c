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

#include "brailledraw.h"
#include "cli_common.h"
#include <stdlib.h>

const unsigned char pixmap[4][2] = {
	{0x01, 0x08},
	{0x02, 0x10},
	{0x04, 0x20},
	{0x40, 0x80}};

uint8_t *pix;

#define pix_select(x,y)  pix[x*PIX_HEIGHT+y]

void raise_mermory_error(TERMINAL_HANDLE * handle){
    ttprintf("WARNING: NULL pointer; malloc failed\r\n");
}

static void set_bytes(char *pbuffer, const unsigned char c) {
	pbuffer[0] = (char)0xE2;
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
    pix = pvPortMalloc(sizeof(uint8_t)*PIX_HEIGHT*(PIX_WIDTH / 8));
    if(pix==NULL)
        raise_mermory_error(handle);
}
void braille_free(TERMINAL_HANDLE * handle){
    if(pix==NULL){
        raise_mermory_error(handle);
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
        raise_mermory_error(handle);
        return;
    }
	for (uint16_t y = 0; y < PIX_HEIGHT; y += 4) {
		for (uint16_t x = 0; x < PIX_WIDTH / 8; x++) {
			for (uint16_t bpos = 0; bpos < 8; bpos += 2) {
				byte = 0x00;
                
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
                
                char out_buffer[3];
				set_bytes(out_buffer, byte);
				ttprintb(out_buffer, sizeof(out_buffer));
			}
		}
		ttprintf("\r\n");
	}
}
