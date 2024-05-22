#ifndef COMP_INC
#define COMP_INC

#include <stdint.h>

#define COMP_UNITYGAIN INT16_MAX

void Comp_init();
extern uint32_t Comp_getGain();
extern uint32_t Comp_getState();
extern uint32_t Comp_getMaxDutyOffset();

#endif