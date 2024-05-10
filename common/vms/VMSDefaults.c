#include "VMS.h"

const VMS_Block_t VMS_DEFAULT_ATTAC = {
    .nextBlocks[0] = VMS_BLOCKID_DEFSUSTAIN,
    .nextBlocks[1] = VMS_BLOCKID_INVALID,
    .offBlock = VMS_BLOCKID_DEFRELEASE,
    .behavior = NORMAL,
    
    .target = volumeCurrent,
    .targetFactor = 1000000,
    .type = VMS_JUMP,
};

const VMS_Block_t VMS_DEFAULT_SUSTAIN = {
    .nextBlocks[0] = VMS_BLOCKID_INVALID,
    .offBlock = VMS_BLOCKID_DEFRELEASE,
    .behavior = NORMAL,
    
    .flags = VMS_FLAG_ISBLOCKPERSISTENT,
    .target = volumeCurrent,
    .targetFactor = 1000000,
    .type = VMS_JUMP
};

const VMS_Block_t VMS_DEFAULT_RELEASE = {
    .nextBlocks[0] = VMS_BLOCKID_INVALID,
    .behavior = INVERTED,
    
    .target = volumeCurrent,
    .targetFactor = 0,
    .type = VMS_JUMP
};