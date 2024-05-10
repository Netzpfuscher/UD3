#ifndef COMP_INC
#define COMP_INC

#include <stdint.h>

void Comp_init();
extern inline uint32_t Comp_getGain();
extern inline uint32_t Comp_getMaxDutyOffset();

#endif