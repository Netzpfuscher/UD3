/**
 * @file SidProcessor.h
 * @brief SID chip emulation and ADSR envelope processing
 *
 * Emulates the Commodore 64 SID (Sound Interface Device) chip's audio synthesis.
 * Processes SID frames to update voice frequencies, waveform flags, and ADSR
 * envelope states. Runs ADSR envelope generator for all three SID voices.
 */

#ifndef SIDPROC_INC
#define SIDPROC_INC

#include <stdint.h>
#include "tasks/tsk_sid.h"

/**
 * @brief Reset SID processor state
 *
 * Resets all SID channels to silent state by clearing flags and frequencies.
 * Also flushes the SID frame buffer queue.
 */
void SidProcessor_resetSid();

/**
 * @brief Run ADSR envelope generators
 *
 * Updates envelope state (attack, decay, sustain, release) for all SID channels.
 * Should be called periodically (typically at MIDI_ISR_Hz rate) to advance
 * envelope phases.
 */
void SidProcessor_runADSR();

/**
 * @brief Get SID channel data array
 *
 * Returns pointer to the internal channel data array for all SID voices.
 * Used by SidFilter and telemetry to access current voice state.
 *
 * @return Pointer to array of N_SIDCHANNEL channel data structures
 */
SIDChannelData_t *SID_getChannelData();

/**
 * @brief Get waveform name for channel
 *
 * Returns human-readable name of the current waveform based on channel flags.
 *
 * @param data Pointer to channel data structure
 * @return Waveform name string ("NOISE", "SQUARE", "SAWTOOTH", "TRIANGLE", or "?")
 */
const char *SID_getWaveName(SIDChannelData_t *data);

/**
 * @brief Handle incoming SID frame
 *
 * Processes a SID frame from the queue, updating all channel parameters
 * (frequency, waveform, ADSR, pulse width, filter). If frame is NULL,
 * triggers SID reset due to timeout.
 *
 * @param frame Pointer to SID frame structure, or NULL for timeout reset
 */
void SidProcessor_handleFrame(SIDFrame_t *frame);
    
#endif