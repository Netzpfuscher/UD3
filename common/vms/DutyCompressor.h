#ifndef COMP_INC
#define COMP_INC

#include <stdint.h>

void Comp_init();
extern uint32_t Comp_getGain();
extern uint32_t Comp_getMaxDutyOffset();

#endif