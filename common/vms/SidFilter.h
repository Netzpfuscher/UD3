/**
 * @file SidFilter.h
 * @brief SID voice filtering and output processing
 *
 * Processes SID channel data to generate output signals with volume scaling,
 * ADSR envelope application, and high-pass voice (HPV) generation for square waves.
 * Applies per-channel volume, master volume, noise volume, and frequency-dependent
 * scaling.
 */

#ifndef SIDFILTER_INC
#define SIDFILTER_INC

#include <stdint.h>
#include <tasks/tsk_sid.h>

/**
 * @brief SID filter global data
 *
 * External reference to global SID filter configuration and state.
 */
extern SIDFilterData_t SID_filterData;

/**
 * @brief Update voice output based on SID channel data
 *
 * Processes SID channel state (frequency, envelope, waveform flags) and applies
 * filtering, volume scaling, and high-pass voice generation. Updates signal
 * generator with resulting frequency, volume, and on-time.
 *
 * @param channel SID channel number (0-2)
 * @param channelData Pointer to channel data structure with frequency, envelope, and flags
 */
void SidFilter_updateVoice(uint32_t channel, SIDChannelData_t *channelData);

#endif