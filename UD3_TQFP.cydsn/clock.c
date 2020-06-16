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

#include "clock.h"

static const int32_t n_increment = 0x20000000;
volatile uint64_t h_time;
volatile uint32_t l_time;
volatile uint32_t increment = 0x20000000;


void clock_tick(){
    h_time += increment;
    l_time = h_time >> 32;
}

void clock_reset_inc(){
    increment = n_increment;
}

void clock_set(uint32_t time){
    h_time = (uint64_t)time << 32;
}
int32_t i=0;
int32_t p=0;
void clock_trim(int32_t trim){
    i += trim * 250;
    if(i>5120000) i=5120000;
    if(i<-5120000) i=-5120000;
    p = trim* 100000;
    increment = n_increment +p+i;
}


/* [] END OF FILE */
