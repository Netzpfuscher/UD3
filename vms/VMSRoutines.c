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