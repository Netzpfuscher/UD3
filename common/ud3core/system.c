
#include "system.h"
#include "hardware.h"


uint32_t SYS_getCPULoadFine(TaskStatus_t * taskStats, uint32_t taskCount, uint32_t sysTime){
    if(sysTime<500) return 0;
    uint32_t currTask = 0;
    for(;currTask < taskCount; currTask++){
        if(strlen(taskStats[currTask].pcTaskName) == 4 && strcmp(taskStats[currTask].pcTaskName, "IDLE") == 0){
            return configTICK_RATE_HZ - ((taskStats[currTask].ulRunTimeCounter) / (sysTime/configTICK_RATE_HZ));
        }
    }
    return -1;
}

const char * SYS_getTaskStateString(eTaskState state){
    switch(state){
        case eRunning:
            return "running";
        case eReady:
            return "ready";
        case eBlocked:
            return "blocked";
        case eSuspended:
            return "suspended";
        case eDeleted:
            return "deleted";
        default:
            return "invalid";
    }
}

uint8_t SYS_detect_hw_rev(void){
#if IS_QFN
    return 0;
#else
    uint8_t bits = 0;
    
    bits |= VB0_Read() ? 0 : 1;
    bits |= (VB1_Read() ? 0 : 1) << 1;
    bits |= (VB2_Read() ? 0 : 1) << 2;
    bits |= (VB3_Read() ? 0 : 1) << 3;
    bits |= (VB4_Read() ? 0 : 1) << 4;
    bits |= (VB5_Read() ? 0 : 1) << 5;

    return bits;   
#endif
}

char * SYS_get_rev_string(uint8_t rev){
    
    switch(rev){
    case 0:
        return "3.0 or 3.1a";
    case 1:
        return "3.1b";
    case 2:
        return "3.1c";
    default:
        return "unknown";
    }
}