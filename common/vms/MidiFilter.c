/*
 * File:   MidiFilter.c
 * Author: Thorb
 *
 * Created on 12 September 2023, 23:15
 */

#include "NoteMapper.h"
#include "MidiFilter.h"
#include "TTerm.h"
#include "tasks/tsk_cli.h"

/* 
 * filter options:
 * 
 * - per channel mute
 * - delay
 * 
 */

MidiFilterHandle_t * MidiFilter_create(uint32_t output) {
    MidiFilterHandle_t * ret = pvPortMalloc(sizeof(MidiFilterHandle_t));
    
    ret->targetOutput = output;
    
    return ret;
}

void MidiFilter_eventHandler(MidiFilterHandle_t * handle, MidiEventType_t evt, uint32_t channel, uint32_t param1, uint32_t param2, uint32_t auxParam){
    switch(evt) {
        case MIDI_EVT_NOTEON:
            Mapper_noteOnEventHandler(handle->targetOutput, param1, param2, channel, auxParam);
            break;
            
        case MIDI_EVT_NOTEOFF:
            Mapper_noteOffEventHandler(handle->targetOutput, param1, channel);
            break;
        case MIDI_EVT_NULL:
            break;
    }

}
