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
    
#if !defined(mapper_H)
#define mapper_H
    
    #if PIC32
    #include <xc.h>
    #endif
    #include <stdint.h>
    #include "helper/vms_types.h"
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


    uint8_t MAPPER_getNextChannel(uint8_t note, uint8_t channel);
    uint8_t MAPPER_getMaps(uint8_t note, MAPTABLE_HEADER * table, MAPTABLE_DATA ** dst);
    void MAPPER_map(uint8_t note, uint8_t velocity, uint8_t channel);
    MAPTABLE_ENTRY * MAPPER_getEntry(uint8_t header, uint8_t index);
    MAPTABLE_HEADER * MAPPER_getHeader(uint8_t index);
    MAPTABLE_HEADER * MAPPER_findHeader(uint8_t prog);
    void MAPPER_programChangeHandler(uint8_t channel, uint8_t program);
    void MAPPER_bendHandler(uint8_t channel);
    void MAPPER_volumeHandler(uint8_t channel);
    void MAPPER_handleMapWrite();

#endif