/**
 * @file VMSDefaults.h
 * @brief Default VMS envelope blocks
 *
 * Provides default attack, sustain, and release envelope blocks for voices
 * that don't have custom VMS programming. These create a simple instant-on,
 * sustain, instant-off envelope.
 */

#ifndef VMSDEF_INC
#define VMSDEF_INC

#include "VMS.h"

/**
 * @brief Default release envelope block
 *
 * Instantly sets volume to 0 when triggered.
 */
extern const VMS_Block_t VMS_DEFAULT_RELEASE;

/**
 * @brief Default sustain envelope block
 *
 * Holds volume at maximum (1000000) indefinitely until note-off.
 * Marked persistent to prevent automatic removal.
 */
extern const VMS_Block_t VMS_DEFAULT_SUSTAIN;

/**
 * @brief Default attack envelope block
 *
 * Instantly sets volume to maximum (1000000) and chains to sustain block.
 */
extern const VMS_Block_t VMS_DEFAULT_ATTAC;

#endif