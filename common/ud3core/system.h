#if !defined(system_H)
#define system_H

#include "FreeRTOS.h"
#include "task.h"
    

uint32_t SYS_getCPULoadFine(TaskStatus_t * taskStats, uint32_t taskCount, uint32_t sysTime);
const char * SYS_getTaskStateString(eTaskState state);    


uint8_t SYS_detect_hw_rev(void);
char * SYS_get_rev_string(uint8_t rev);    
    
#endif