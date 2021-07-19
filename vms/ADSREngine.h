#include <stdint.h>

#ifndef VMS
#define VMS

#include "MidiController.h"

#define VMS_DIE 0xdeadbeef
#define VMS_PREVIEW_MEMSIZE 15000

extern DLLObject * VMS_listHead;
extern uint8_t * VMSBLOCKS;

uint32_t SYS_timeSince(uint32_t start);
uint32_t SYS_getTime();

int32_t qSin(int32_t x);
unsigned VMS_calculateValue(VMS_listDataObject * data);
void VMS_clear();
void VMS_nextBlock(VMS_listDataObject * data, unsigned blockSet);
void VMS_addBlockToList(VMS_BLOCK * block, SynthVoice * voice);
void VMS_removeBlockFromList(VMS_listDataObject * target);
unsigned VMS_hasReachedThreshold(VMS_BLOCK * block, int32_t factor, unsigned customDirection);
int32_t VMS_getCurrentFactor(KNOWN_VALUE ID, SynthVoice * voice);
void VMS_setKnownValue(KNOWN_VALUE ID, int32_t value, SynthVoice * voice);
int32_t VMS_getKnownValue(KNOWN_VALUE ID, SynthVoice * voice);
void VMS_resetVoice(SynthVoice * voice, VMS_BLOCK * startBlock);
int32_t VMS_getParam(VMS_BLOCK * block, SynthVoice * voice, uint8_t param);
void VMS_run();
void VMS_init();

#endif