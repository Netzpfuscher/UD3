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
    
#if !defined(mapper_H)
#define mapper_H
    
    #if PIC32
    #include <xc.h>
    #endif
    #include <stdint.h>
    #include "vms_types.h"
    //#include "sys/attribs.h"

    #define MAP_ENA_PITCHBEND   0x80
    #define MAP_ENA_STEREO      0x40
    #define MAP_ENA_VOLUME      0x20
    #define MAP_ENA_DAMPER      0x10
    #define MAP_ENA_PORTAMENTO  0x08
    #define MAP_FREQ_MODE       0x01

    #define FREQ_MODE_OFFSET    1
    #define FREQ_MODE_FIXED     0

    #define MAPPER_FLASHSIZE    16384

    void MAPPER_map(uint8_t voice, uint8_t note, uint8_t velocity, uint8_t channel);
    MAPTABLE_ENTRY * MAPPER_getEntry(uint8_t header, uint8_t index);
    MAPTABLE_HEADER * MAPPER_getHeader(uint8_t index);
    MAPTABLE_HEADER * MAPPER_findHeader(uint8_t prog);
    void MAPPER_programChangeHandler(uint8_t channel, uint8_t program);
    MAPTABLE_DATA * MAPPER_getMap(uint8_t note, MAPTABLE_HEADER * table);
    void MAPPER_bendHandler(uint8_t channel);
    void MAPPER_volumeHandler(uint8_t channel);
    void MAPPER_handleMapWrite();

#endif