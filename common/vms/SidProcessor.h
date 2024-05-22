#ifndef SIDPROC_INC
#define SIDPROC_INC
    
#include <stdint.h>
#include "tasks/tsk_sid.h"
    
    
void SidProcessor_resetSid();
void SidProcessor_runADSR();
SIDChannelData_t * SID_getChannelData();    
const char * SID_getWaveName(SIDChannelData_t * data);
void SidProcessor_handleFrame(SIDFrame_t * frame);
    
#endif