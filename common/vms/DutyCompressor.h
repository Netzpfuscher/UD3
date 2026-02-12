/**
 * @file DutyCompressor.h
 * @brief Duty cycle compressor for audio output limiting
 *
 * Provides dynamic gain reduction to prevent duty cycle from exceeding limits.
 * Implements attack/sustain/release envelope for smooth compression.
 */

#ifndef COMP_INC
#define COMP_INC

#include <stdint.h>

#define COMP_UNITYGAIN INT16_MAX //!< Unity gain value (0 dB)

/**
 * @brief Initialize duty cycle compressor
 */
void Comp_init();

/**
 * @brief Get current compressor gain
 *
 * @return Current gain value (0 to COMP_UNITYGAIN)
 */
extern uint32_t Comp_getGain();

/**
 * @brief Get current compressor state
 *
 * @return Compressor state (0=attack, 1=sustain, 2=release)
 */
extern uint32_t Comp_getState();

/**
 * @brief Get maximum duty cycle offset
 *
 * @return Maximum duty offset value
 */
extern uint32_t Comp_getMaxDutyOffset();

#endif