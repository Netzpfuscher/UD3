#ifndef VMS_INC
#define VMS_INC

#include <stdint.h>

typedef struct _SINE_DATA_ SINE_DATA;
typedef struct _VMS_listDataObject_ VMS_listDataObject;
typedef struct __attribute__ ((packed)) _VMS_BLOCK_ VMS_Block_t;
typedef struct _VMS_VoiceData_t_ VMS_VoiceData_t;

typedef enum {TONE_NORMAL, TONE_NOISE, TONE_SINGE_SHOT} ToneType;
typedef enum {VMS_INVALID, VMS_EXP, VMS_EXP_INV, VMS_LIN, VMS_SIN, VMS_JUMP} VMS_MODTYPE;
typedef enum {maxOnTime, minOnTime, onTime, volumeCurrent, volumeTarget, volumeFactor, frequency, freqCurrent, freqTarget, freqFactor, noise, pTime, circ1, circ2, circ3, circ4, CC_102, CC_103, CC_104, CC_105, CC_106, CC_107, CC_108, CC_109, CC_110, CC_111, CC_112, CC_113, CC_114, CC_115, CC_116, CC_117, CC_118, CC_119, HyperVoice_Count, HyperVoice_Phase, HyperVoice_Volume, KNOWNVAL_MAX, ANNOYINGPAD=0xffff} KNOWN_VALUE;
typedef enum {RISING, FALLING, ANY, NONE} DIRECTION;
typedef enum {INVERTED = 0, NORMAL = 1} NOTEOFF_BEHAVIOR;

#define VMS_MAX_BRANCHES 4
#define VMS_CIRC_PARAM_COUNT 4 //kinda non dynamic :/

#define VMS_FLAG_BLOCK_INVALID              0xffffffff
#define VMS_FLAG_ISVARIABLE_PARAM1          0x00000001
#define VMS_FLAG_ISVARIABLE_PARAM2          0x00000002
#define VMS_FLAG_ISVARIABLE_PARAM3          0x00000004
#define VMS_FLAG_ISVARIABLE_TARGETFACTOR    0x00000008
#define VMS_FLAG_ISBLOCKPERSISTENT          0x80000000  //this flag tells the interpreter, that a block is a sustain block, and must not be deleted one run
#define VMS_DIE                             0xdeadbeef

#define VMS_BLOCKID_INVALID                 0xffff
#define VMS_BLOCKID32_INVALID               0xffffffff
#define VMS_BLOCKID_DEFATTACK               0xfff0
#define VMS_BLOCKID_DEFSUSTAIN              0xfff1
#define VMS_BLOCKID_DEFRELEASE              0xfff2

#define VMS_BLOCKSET_NEXTBLOCKS 1
#define VMS_BLOCKSET_OFFBLOCK 0

#define VMS_VERSION 2
#define VMS_UNITY_FACTOR 1000000

struct _SINE_DATA_{
    int32_t currCount;
};

struct _VMS_VoiceData_t_{
    uint32_t    freqTarget;
    uint32_t    freqCurrent;
    int32_t     freqFactor;
    uint32_t    freqLast;   //for portamento
    
    uint32_t    onTimeTarget;
    uint32_t    onTimeCurrent;
    int32_t     onTimeFactor;
    
    uint32_t    volumeTarget;
    uint32_t    volumeCurrent;
    int32_t     volumeFactor;
    
    uint32_t    noiseTarget;
    uint32_t    noiseCurrent;
    int32_t     noiseFactor;
    
    uint32_t    hypervoiceCount;
    uint32_t    hypervoiceVolumeFactor;
    uint32_t    hypervoiceVolumeCurrent;
    uint32_t    hypervoicePhase;
    
    uint32_t    portamentoTarget;
    int32_t     portamentoCurrent;
    
    uint32_t    on;
    
    uint32_t    flags;
    uint32_t    baseVolume;
    uint32_t    baseVelocity;
    
    uint32_t    startTime;
    uint8_t     sourceChannel;
    uint8_t     sourceNote;
    
    int32_t     circ[VMS_CIRC_PARAM_COUNT];
    
    uint32_t    voiceIndex;
};

struct _VMS_listDataObject_{
    uint32_t targetOutputIndex;
    uint32_t targetVoiceIndex;
    VMS_Block_t * block;
    void * data;
    
    DIRECTION thresholdDirection;
    uint32_t nextRunTime;
    uint32_t periodMs;
};

typedef struct{
    KNOWN_VALUE sourceId    : 8;
    int32_t     rangeStart  : 12;
    int32_t     rangeEnd    : 12;
} RangeParameters;

struct _VMS_BLOCK_ {
    //next block indices
    uint16_t nextBlocks[VMS_MAX_BRANCHES];
    uint16_t offBlock;
    
    //modulation data
    NOTEOFF_BEHAVIOR behavior : 8;
    VMS_MODTYPE type : 8;
    KNOWN_VALUE target :  16;
    
    int32_t targetFactor;
    int32_t param1;
    int32_t param2;
    int32_t param3;
    
    uint32_t periodMs;
    uint32_t flags;
};

//WARNING: don't remove the packed attribute. This allow accesses to this datatype to be not aligned to word boundaries, which is essential for readout from the usb command buffer
typedef struct  __attribute__ ((packed)) {
    uint32_t uid;
    uint32_t nextBlocks[VMS_MAX_BRANCHES];    //NOTE: used to be VMS_BLOCK*, but since that is changed to an ID during write it is acceptable to instead use uint32_t
    uint32_t offBlock;
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
} VMS_LEGAYBLOCK_t;

void VMS_removeBlocksWithTargetVoice(uint32_t targetVoice);
uint32_t VMS_isBlockValid(VMS_Block_t * block);
void VMS_killBlocks();
void VMS_addBlockToList(VMS_Block_t * block, uint32_t output, uint32_t voiceId);
void VMS_run();
void VMS_getSemaphore();
void VMS_returnSemaphore();
void VMS_init();

void VMS_convertToLegacyBlock(VMS_LEGAYBLOCK_t * dst, VMS_Block_t * src);
void VMS_convertFromLegacyBlock(VMS_Block_t * dst, VMS_LEGAYBLOCK_t * src);
void VMS_convertFromLegacyBlockWith63Bytes(VMS_Block_t * dst, VMS_LEGAYBLOCK_t * src);

#endif