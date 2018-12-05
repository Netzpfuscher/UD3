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

#if !defined(PRIORITY_H)
#define PRIORITY_H

#define PRIO_TERMINAL 3
#define PRIO_OVERLAY 1
#define PRIO_ANALOG 1
#define PRIO_THERMISTOR 1
#define PRIO_UART 3
#define PRIO_USB 3
#define PRIO_ETH 3
#define PRIO_FAULT 3
#define PRIO_MIDI 2
#define PRIO_QCW 3
    
    
#define STACK_TERMINAL 500
#define STACK_OVERLAY 256
#define STACK_ANALOG 128
#define STACK_THERMISTOR 100
#define STACK_UART 256
#define STACK_MIN 256
#define STACK_USB 400
#define STACK_ETH 256
#define STACK_FAULT 100
#define STACK_MIDI 160
    

    
/* Basic bit manipulation macros
No one should ever have to rewrite these
*/

//Set bit y (0-indexed) of x to '1' by generating a a mask with a '1' in the proper bit location and ORing x with the mask.

#define SET(x,y) x |= (1 << y)

//Set bit y (0-indexed) of x to '0' by generating a mask with a '0' in the y position and 1's elsewhere then ANDing the mask with x.

#define CLEAR(x,y) x &= ~(1<< y)

//Return '1' if the bit value at position y within x is '1' and '0' if it's 0 by ANDing x with a bit mask where the bit in y's position is '1' and '0' elsewhere and comparing it to all 0's.  Returns '1' in least significant bit position if the value of the bit is '1', '0' if it was '0'.

#define READ(x,y) ((0u == (x & (1<<y)))?0u:1u)

//Toggle bit y (0-index) of x to the inverse: '0' becomes '1', '1' becomes '0' by XORing x with a bitmask where the bit in position y is '1' and all others are '0'.

#define TOGGLE(x,y) (x ^= (1<<y))

#endif