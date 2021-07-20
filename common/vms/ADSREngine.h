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
    
#if !defined(ADSREngine_H)
#define ADSREngine_H

#include <stdint.h>

#include "MidiController.h"

#define VMS_DIE 0xdeadbeef
#define VMS_PREVIEW_MEMSIZE 15000

extern DLLObject * VMS_listHead;
extern uint8_t * VMSBLOCKS;

uint32_t SYS_timeSince(uint32_t start);
uint32_t SYS_getTime();

int32_t qSin(int32_t x);
unsigned VMS_calculateValue(VMS_listDataObject * data);
void VMS_clear();
void VMS_nextBlock(VMS_listDataObject * data, unsigned blockSet);
void VMS_addBlockToList(VMS_BLOCK * block, SynthVoice * voice);
void VMS_removeBlockFromList(VMS_listDataObject * target);
unsigned VMS_hasReachedThreshold(VMS_BLOCK * block, int32_t factor, unsigned customDirection);
int32_t VMS_getCurrentFactor(KNOWN_VALUE ID, SynthVoice * voice);
void VMS_setKnownValue(KNOWN_VALUE ID, int32_t value, SynthVoice * voice);
int32_t VMS_getKnownValue(KNOWN_VALUE ID, SynthVoice * voice);
void VMS_resetVoice(SynthVoice * voice, VMS_BLOCK * startBlock);
int32_t VMS_getParam(VMS_BLOCK * block, SynthVoice * voice, uint8_t param);
void VMS_run();
void VMS_init();

#endif