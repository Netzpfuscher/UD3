/*
    Copyright (C) 2020,2021 TMax-Electronics.de
   
    This file is part of the MidiStick Firmware.

    the MidiStick Firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    the MidiStick Firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the MidiStick Firmware.  If not, see <https://www.gnu.org/licenses/>.
*/
    
#if !defined(ADSREngine_H)
#define ADSREngine_H

#include <stdint.h>

#include "MidiController.h"

#define VMS_DIE 0xdeadbeef
#define VMS_PREVIEW_MEMSIZE 15000

#define VMS_FLAG_ISVARIABLE_PARAM1          0x00000001
#define VMS_FLAG_ISVARIABLE_PARAM2          0x00000002
#define VMS_FLAG_ISVARIABLE_PARAM3          0x00000004
#define VMS_FLAG_ISVARIABLE_TARGETFACTOR    0x00000008

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
unsigned VMS_hasReachedThreshold(VMS_BLOCK * block, int32_t factor, int32_t targetFactor, int32_t param1);
int32_t VMS_getCurrentFactor(KNOWN_VALUE ID, SynthVoice * voice);
void VMS_setKnownValue(KNOWN_VALUE ID, int32_t value, SynthVoice * voice);
int32_t VMS_getKnownValue(KNOWN_VALUE ID, SynthVoice * voice);
void VMS_resetVoice(SynthVoice * voice, VMS_BLOCK * startBlock);
int32_t VMS_getParam(VMS_BLOCK * block, SynthVoice * voice, uint8_t param);
int32_t VMS_getParameter(uint8_t param, VMS_BLOCK * block, SynthVoice * voice);
DIRECTION VMS_getThresholdDirection(VMS_BLOCK * block, int32_t param1);
void VMS_run();
void VMS_init();

#endif