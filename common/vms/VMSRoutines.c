/*
 * VMS - Versatile Modulation System
 *
 * Copyright (c) 2020 Thorben Zethoff
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

#include <stdint.h>

#if PIC32
#include <xc.h>
#endif


//#include "ADSREngine.h"
#include "VMSRoutines.h"

const VMS_BLOCK ATTAC = {
    .nextBlocks[0] = &SUSTAIN,
    .offBlock = &RELEASE,
    .behavior = NORMAL,
    
    .thresholdDirection = RISING,
    .target = onTime,
    .targetFactor = 1000000,
    .type = VMS_JUMP,
};

const VMS_BLOCK SUSTAIN = {
    .nextBlocks[0] = VMS_DIE,
    .offBlock = &RELEASE,
    .behavior = NORMAL,
    
    .thresholdDirection = RISING,
    .target = onTime,
    .targetFactor = 1000000,
    .type = VMS_JUMP
};

const VMS_BLOCK RELEASE = {
    .behavior = INVERTED,
    
    .thresholdDirection = FALLING,
    .target = onTime,
    .targetFactor = 0,
    .type = VMS_JUMP
};