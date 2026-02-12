/**
 * @file VMSWrapper.h
 * @brief VMS integration wrapper for signal generator
 *
 * Provides high-level interface between VMS engine and signal generator.
 * Manages voice allocation, note start/stop, parameter updates, and VMS
 * block memory. Bridges MIDI/mapper layer with VMS modulation system.
 */

#ifndef VMSW_INC
#define VMSW_INC

#include "VMS.h"

#define VMS_DEFAULT_BLOCKMEM_SIZE 16786 //!< Default VMS block memory size in bytes

/**
 * @brief Get current system time in milliseconds
 *
 * Converts FreeRTOS ticks to milliseconds.
 */
#define VMSW_GET_TIME_MS() ((xTaskGetTickCount() * 1000) / configTICK_RATE_HZ)

/**
 * @brief Initialize VMS wrapper
 *
 * Allocates voice data arrays, initializes VMS core, and creates VMS task.
 * Must be called once at system startup.
 */
void VMSW_init();

/**
 * @brief Stop a note
 *
 * Triggers note-off blocks for all voices playing the specified note on
 * the given channel and output.
 *
 * @param output Output index
 * @param note MIDI note number
 * @param channel MIDI channel number
 */
void VMSW_stopNote(uint32_t output, uint32_t note, uint32_t channel);

/**
 * @brief Start a new note
 *
 * Allocates a voice and starts VMS block execution with specified parameters.
 * Initializes voice data with target frequency, volume, on-time, and starts
 * attack envelope.
 *
 * @param output Output index
 * @param startBlockID VMS block ID to start with (attack envelope)
 * @param sourceNote MIDI note number
 * @param sourceChannel MIDI channel number
 * @param targetOnTime Target pulse on-time
 * @param targetVolume Target volume
 * @param targetFrequency Target frequency (dHz)
 * @param flags Voice flags
 * @param baseVolume Base volume from mapper
 * @param baseVelocity MIDI velocity
 */
void VMSW_startNote(uint32_t output, uint32_t startBlockID, uint32_t sourceNote, uint32_t sourceChannel, uint32_t targetOnTime, uint32_t targetVolume, uint32_t targetFrequency, uint32_t flags, uint32_t baseVolume, uint32_t baseVelocity);

/**
 * @brief Get source MIDI channel for voice
 *
 * @param output Output index
 * @param index Voice index
 * @return MIDI channel number (0-15)
 */
extern uint32_t VMSW_getSrcChannel(uint32_t output, uint32_t index);

/**
 * @brief Get VMS block pointer by ID
 *
 * Retrieves pointer to VMS block from NVM or returns default block.
 *
 * @param index Block ID
 * @return Pointer to VMS block, or default block if invalid
 */
extern const VMS_Block_t *VMSW_getBlockPtr(uint32_t index);

/**
 * @brief Check if any voice is active
 *
 * Scans all outputs and voices for any active voice.
 *
 * @return Non-zero if any voice is on, 0 if all silent
 */
uint32_t VMSW_isAnyVoiceOn();

/**
 * @brief Check if specific voice is on
 *
 * @param output Output index
 * @param index Voice index
 * @return Non-zero if voice is on, 0 if off
 */
extern uint32_t VMSW_isVoiceOn(uint32_t output, uint32_t index);

/**
 * @brief Check if voice is actively producing output
 *
 * Voice is active if on and has blocks referencing it.
 *
 * @param output Output index
 * @param index Voice index
 * @return Non-zero if voice is active, 0 otherwise
 */
extern uint32_t VMSW_isVoiceActive(uint32_t output, uint32_t index);

/**
 * @brief Handle MIDI panic/all-notes-off
 *
 * Stops all voices and clears VMS blocks.
 */
void VMSW_panicHandler();

/**
 * @brief Handle pitch bend change
 *
 * Updates frequencies for all voices on the specified MIDI channel.
 *
 * @param channel MIDI channel number (0-15)
 */
void VMSW_bendHandler(uint32_t channel);

/**
 * @brief Handle volume change
 *
 * Updates volumes for all voices on the specified MIDI channel.
 *
 * @param channel MIDI channel number (0-15)
 */
void VMSW_volumeChangeHandler(uint32_t channel);

/**
 * @brief Handle pulse width change
 *
 * Updates pulse width parameters for all active voices.
 */
void VMSW_pulseWidthChangeHandler();

/**
 * @brief Get current modulation factor for parameter
 *
 * Returns the current factor value (rate of change) for a modulated parameter.
 *
 * @param ID Parameter ID (KNOWN_VALUE enum)
 * @param output Output index
 * @param voiceId Voice index
 * @return Current factor value
 */
int32_t VMSW_getCurrentFactor(KNOWN_VALUE ID, uint32_t output, uint32_t voiceId);

/**
 * @brief Get known parameter value
 *
 * Retrieves current value of a voice parameter by ID.
 *
 * @param ID Parameter ID (KNOWN_VALUE enum)
 * @param output Output index
 * @param voiceId Voice index
 * @return Parameter value
 */
int32_t VMSW_getKnownValue(KNOWN_VALUE ID, uint32_t output, uint32_t voiceId);

/**
 * @brief Set known parameter value
 *
 * Directly sets a voice parameter value by ID.
 *
 * @param ID Parameter ID (KNOWN_VALUE enum)
 * @param value New parameter value
 * @param output Output index
 * @param voiceId Voice index
 */
void VMSW_setKnownValue(KNOWN_VALUE ID, int32_t value, uint32_t output, uint32_t voiceId);

/**
 * @brief Get block memory size in block count
 *
 * @return Number of VMS blocks that fit in NVM block memory
 */
uint32_t VMSW_getBlockMemSizeInBlocks();

/**
 * @brief Get voice data array
 *
 * Returns pointer to first output's voice data array.
 *
 * @return Pointer to VMS_VoiceData_t array
 */
VMS_VoiceData_t *VMSW_getVoiceData();

/**
 * @brief Update VMS block in NVM
 *
 * Writes updated block data to NVM at specified block ID.
 *
 * @param id Block ID to update
 * @param src Pointer to source block data
 */
void VMSW_updateBlock(uint32_t id, VMS_Block_t *src);

/**
 * @brief Write legacy block with 63-byte format
 *
 * Converts and writes a legacy 63-byte VMS block to NVM.
 *
 * @param block Pointer to legacy block structure
 * @return Block ID where written
 */
uint32_t VMSW_writeLegacyBlockWith63Bytes(VMS_LEGAYBLOCK_t *block);

#endif