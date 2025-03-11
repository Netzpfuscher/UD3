#ifndef SIDFILTER_INC
#define SIDFILTER_INC

#include <stdint.h>
#include <tasks/tsk_sid.h>
    
extern SIDFilterData_t SID_filterData;
    
void SidFilter_updateVoice(uint32_t channel, SIDChannelData_t * channelData);

#endif