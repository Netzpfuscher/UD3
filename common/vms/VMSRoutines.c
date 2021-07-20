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