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
#if PIC32
    #include <xc.h>
#endif
#include <stdint.h>

#if !defined(vms_types_H)
#define vms_types_H

#define VMS_MAX_BRANCHES 4

#define NO_BLK (VMS_BLOCK*)0xFFFFFFFF
#define HARD_BLK(x) (VMS_BLOCK*)x

typedef struct _SynthVoice_ SynthVoice;
typedef struct _ChannelInfo_ ChannelInfo;
typedef struct _SINE_DATA_ SINE_DATA;
typedef struct _VMS_listDataObject_ VMS_listDataObject;
typedef struct _VMS_BLOCK_ VMS_BLOCK;
typedef struct _MapData_ MAPTABLE_DATA;
typedef struct _MapEntry_ MAPTABLE_ENTRY;
typedef struct _MapHeader_ MAPTABLE_HEADER;

typedef enum {TONE_NORMAL, TONE_NOISE, TONE_SINGE_SHOT} ToneType;
typedef enum {MOD_AM_NOISE, MOD_FM_NOISE, MOD_AM_SWEEP, MOD_FM_SWEEP} ModulationType;
typedef enum {VMS_EXP, VMS_EXP_INV, VMS_LIN, VMS_SIN, VMS_JUMP} VMS_MODTYPE;
typedef enum {maxOnTime, minOnTime, onTime, otCurrent, otTarget, otFactor, frequency, freqCurrent, freqTarget, freqFactor, noise, pTime, circ1, circ2, circ3, circ4, CC_102, CC_103, CC_104, CC_105, CC_106, CC_107, CC_108, CC_109, CC_110, CC_111, CC_112, CC_113, CC_114, CC_115, CC_116, CC_117, CC_118, CC_119, HyperVoice_Count, HyperVoice_Phase, KNOWNVAL_MAX} KNOWN_VALUE;
typedef enum {RISING, FALLING, ANY, NONE} DIRECTION;
typedef enum {INVERTED = 0, NORMAL = 1} NOTEOFF_BEHAVIOR;


typedef struct _DLLObject_ DLLObject;

struct _DLLObject_{
    DLLObject * next;
    DLLObject * prev;
    uint32_t uid;
    void * data;
};

struct _MapData_{
    int16_t noteFreq;
    uint8_t targetOT;
    uint8_t flags;
    VMS_BLOCK * VMS_Startblock;        //this is the VMS_Block ID when in flash, and will be changed to be the pointer to the block when loaded into ram
}__attribute__((packed));

struct _MapEntry_{
    uint8_t startNote;
    uint8_t endNote;
    MAPTABLE_DATA data;
}__attribute__((packed));

struct _MapHeader_{
    uint8_t listEntries;
    uint8_t programNumber;
    char name[18];
} __attribute__((packed));

//struct to contain all variables and parameters for the voices
struct _SynthVoice_{
    uint32_t    freqTarget;
    uint32_t    freqCurrent;
    int32_t     freqFactor;
    
    uint32_t    otTarget;
    uint32_t    otCurrent;
    int32_t     otFactor;
    uint32_t    outputOT;
    
    uint32_t    noiseTarget;
    uint32_t    noiseCurrent;
    int32_t     noiseFactor;
    
    uint8_t     currNote;
    uint8_t     hyperVoiceCount;
    uint8_t     hyperVoicePhase;
    int32_t     hyperVoicePhaseFrac;
    uint16_t    hyperVoice_timings[2];
    uint32_t    on;
    uint32_t    noteAge;
    uint8_t     currNoteOrigin;
    int32_t     portamentoParam;
    
    int32_t     circ1;
    int32_t     circ2;
    int32_t     circ3;
    int32_t     circ4;
    
    uint8_t     id;
    MAPTABLE_DATA * map;
    
};

struct _ChannelInfo_{
    uint32_t        bendFactor;   
    
    unsigned        sustainPedal;
    unsigned        damperPedal;
    
    uint16_t        bendMSB;   
    uint16_t        bendLSB;
    float           bendRange;
    
    uint16_t        currOT;
    int32_t         portamentoTime;
    uint32_t        lastFrequency;
    uint8_t         parameters[18];
    
    uint16_t        currVolume;
    uint16_t        currStereoVolume;
    uint8_t         currStereoPosition;
    uint16_t        currVolumeModifier;
    
    uint8_t         currProgramm; 
    MAPTABLE_HEADER * currentMap;
};


struct _SINE_DATA_{
    int32_t currCount;
};

struct _VMS_listDataObject_{
    SynthVoice * targetVoice;
    VMS_BLOCK * block;
    void * data;
    
    DIRECTION thresholdDirection;
    uint32_t nextRunTime;
    uint32_t period;
};

typedef struct{
    KNOWN_VALUE sourceId    : 8;
    int32_t     rangeStart  : 12;
    int32_t     rangeEnd    : 12;
} RangeParameters;

struct _VMS_BLOCK_{
    uint32_t uid;
    VMS_BLOCK * nextBlocks[VMS_MAX_BRANCHES];
    VMS_BLOCK * offBlock;
    NOTEOFF_BEHAVIOR behavior;
    VMS_MODTYPE type;
    KNOWN_VALUE target;
    DIRECTION thresholdDirection;
    int32_t targetFactor;
    int32_t param1;
    int32_t param2;
    int32_t param3;
    uint32_t period;
    uint32_t flags;
};
#endif