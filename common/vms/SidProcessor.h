#ifndef SIDPROC_INC
#define SIDPROC_INC
    
#include <stdint.h>
#include "tasks/tsk_sid.h"
    
    
void SidProcessor_resetSid();
void SidProcessor_runADSR();
void SidProcessor_handleFrame(SIDFrame_t * frame);
    
#endif