/**
 * @file MidiFilter.h
 * @brief MIDI event filtering and routing
 *
 * Filters MIDI events and routes them to the note mapper for frequency conversion.
 * Provides infrastructure for future filtering capabilities like per-channel mute
 * and event delay.
 */

#ifndef MIDIFILTER_INC
#define MIDIFILTER_INC

#include <stdint.h>

/**
 * @brief MIDI event types
 *
 * Defines the types of MIDI events that can be processed by the filter.
 */
typedef enum {
	MIDI_EVT_NULL,    //!< No event
	MIDI_EVT_NOTEON,  //!< Note on event
	MIDI_EVT_NOTEOFF  //!< Note off event
} MidiEventType_t;

/**
 * @brief MIDI filter handle structure
 *
 * Contains the state and configuration for a MIDI filter instance.
 */
typedef struct {
	uint32_t targetOutput; //!< Target output index for routing filtered events
} MidiFilterHandle_t;

/**
 * @brief Create a MIDI filter instance
 *
 * Allocates and initializes a new MIDI filter handle.
 *
 * @param output Target output index for routing events
 * @return Pointer to newly created filter handle, NULL on allocation failure
 */
MidiFilterHandle_t *MidiFilter_create(uint32_t output);

/**
 * @brief Handle incoming MIDI events
 *
 * Routes MIDI events to the appropriate note mapper handler based on event type.
 * Currently supports note on/off events. Future versions may add filtering
 * capabilities like per-channel mute or event delay.
 *
 * @param handle Filter instance handle
 * @param evt MIDI event type (note on, note off, or null)
 * @param channel MIDI channel number (0-15)
 * @param param1 First parameter (typically note number for note events)
 * @param param2 Second parameter (typically velocity for note on events)
 * @param auxParam Auxiliary parameter for extended event information
 */
void MidiFilter_eventHandler(MidiFilterHandle_t *handle, MidiEventType_t evt, uint32_t channel, uint32_t param1, uint32_t param2, uint32_t auxParam);

#endif