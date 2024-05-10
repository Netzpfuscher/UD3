#ifndef MIDIFILTER_INC
#define MIDIFILTER_INC

#include <stdint.h>
    
typedef enum {MIDI_EVT_NULL, MIDI_EVT_NOTEON, MIDI_EVT_NOTEOFF} MidiEventType_t;

typedef struct{
    uint32_t targetOutput;
    
} MidiFilterHandle_t;

MidiFilterHandle_t * MidiFilter_create(uint32_t output);
void MidiFilter_eventHandler(MidiFilterHandle_t * handle, MidiEventType_t evt, uint32_t channel, uint32_t param1, uint32_t param2, uint32_t auxParam);

#endif