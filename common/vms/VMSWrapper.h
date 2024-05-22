#ifndef VMSW_INC
#define VMSW_INC

#include "VMS.h"

#define VMS_DEFAULT_BLOCKMEM_SIZE  16786

#define VMSW_GET_TIME_MS() ((xTaskGetTickCount() * 1000) / configTICK_RATE_HZ)

void VMSW_init();

void VMSW_stopNote(uint32_t output, uint32_t note, uint32_t channel);
void VMSW_startNote(uint32_t output, uint32_t startBlockID, uint32_t sourceNote, uint32_t sourceChannel, uint32_t targetOnTime, uint32_t targetVolume, uint32_t targetFrequency, uint32_t flags, uint32_t baseVolume, uint32_t baseVelocity);

extern uint32_t VMSW_getSrcChannel(uint32_t output, uint32_t index);

extern const VMS_Block_t * VMSW_getBlockPtr(uint32_t index);

uint32_t VMSW_isAnyVoiceOn();
extern uint32_t VMSW_isVoiceOn(uint32_t output, uint32_t index);
extern uint32_t VMSW_isVoiceActive(uint32_t output, uint32_t index);

void VMSW_panicHandler();
void VMSW_bendHandler(uint32_t channel);
void VMSW_volumeChangeHandler(uint32_t channel);
void VMSW_pulseWidthChangeHandler();

int32_t VMSW_getCurrentFactor(KNOWN_VALUE ID, uint32_t output, uint32_t voiceId);
int32_t VMSW_getKnownValue(KNOWN_VALUE ID, uint32_t output, uint32_t voiceId);
void VMSW_setKnownValue(KNOWN_VALUE ID, int32_t value, uint32_t output, uint32_t voiceId);

uint32_t VMSW_getBlockMemSizeInBlocks();
VMS_VoiceData_t ** VMSW_getVoiceData();
void VMSW_updateBlock(uint32_t id, VMS_Block_t * src);
uint32_t VMSW_writeLegacyBlockWith63Bytes(VMS_LEGAYBLOCK_t * block);

#endif