
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "DLL/include/DLL.h"
#include "NoteMapper.h"
#include "MidiProcessor.h"
#include "SignalGenerator.h"
#include "VMS.h"
#include "VMSWrapper.h"
#include "helper/nvm.h"
#include "interrupter.h"
#include "cli_common.h"
#include "tasks/tsk_cli.h"

static MAPTABLE_HEADER_t * channelMap[MIDI_CHANNELCOUNT];
static uint32_t Mapper_getNextVoice(uint8_t note, uint8_t channel);
static uint32_t mapMemorySize = MAPPER_MAPMEM_SIZE;

const struct{
    MAPTABLE_HEADER_t h0;
    MAPTABLE_ENTRY_t h0e0;
} DEFMAP = {.h0 = {.programNumberStart = 0, .programNumberEnd = 0x7f, .listEntries = 1} , .h0e0 = {.startNote = 0, .endNote = 127, .data.startblockID = VMS_BLOCKID_DEFATTACK, .data.flags = MAP_ENA_DAMPER | MAP_ENA_STEREO | MAP_ENA_VOLUME | MAP_ENA_PITCHBEND | MAP_FREQ_MODE, .data.noteFreq = 0, .data.targetOT = MAPPER_VOLUME_MAX}};

void Mapper_init(){
    memset(channelMap, 0, sizeof(MAPTABLE_ENTRY_t*) * MIDI_CHANNELCOUNT);
}

static uint32_t isHeaderValid(MAPTABLE_HEADER_t * header){
    return header->listEntries != 0 && header->listEntries != 0xff;
}

static void loadNewProgramm(uint32_t channel, uint32_t programm){
    //unlike in the MidiBox implementation the map list will always remain in flash instead of being loaded into ram
    //TERM_printDebug(min_handle[1], "loading mapentry\r\n");               
    
    //find map in NVM
    channelMap[channel] = NULL;
    
    //go through all headers in the table and find the one we need
    MAPTABLE_HEADER_t * currHeader = (MAPTABLE_HEADER_t *) NVM_mapMem;
    uint32_t currOffset = 0;
    while(1){
        
        //is the map uninialized?
        if(currHeader->listEntries == 0 || currHeader->listEntries == 0xff){
            //yes map is not initialized => we reached the end
            break;
            
        //does this header match the program number we are looking for?
        }else if(currHeader->programNumberStart <= programm && currHeader->programNumberEnd >= programm){
            //yes!  assign it to the channel
            channelMap[channel] = currHeader;
            break;
        }else if(currHeader->programNumberStart == 0xff && currHeader->programNumberEnd == 0xff){
            //we reached the end of usable data without finding anything :(
            break;
        }

        //no, update offset to the next header
        currOffset += sizeof(MAPTABLE_HEADER_t) + currHeader->listEntries * sizeof(MAPTABLE_ENTRY_t);
        
        if(currOffset >= MAPMEM_SIZE) break;
        
        //and assign the new pointer
        currHeader = (MAPTABLE_HEADER_t *) &NVM_mapMem[currOffset];
    }

    //did we find the required map? If not: set the default map
    if(channelMap[channel] == NULL) channelMap[channel] = (MAPTABLE_HEADER_t *) &DEFMAP;        
}

const char * Mapper_getProgrammName(uint32_t channel){
    //is a map loaded on the channel?
    if(channelMap[channel] == NULL){
        //nope => return NULL
        return NULL;
    }else{
        return channelMap[channel]->name;
    }
}

void Mapper_bendHandler(uint32_t channel){
    VMSW_bendHandler(channel);
}

void Mapper_volumeChangeHandler(uint32_t channel){
    VMSW_volumeChangeHandler(channel);
}

void Mapper_noteOffEventHandler(uint32_t output, uint32_t note, uint32_t channel){
    VMSW_stopNote(output, note, channel);
}

void Mapper_programmChangeHandler(uint32_t channel, uint32_t programm){
    loadNewProgramm(channel, programm);
}

void Mapper_noteOnEventHandler(uint32_t output, uint32_t note, uint32_t velocity, uint32_t channel, uint32_t programm){
    //does the map loaded still match the one required?
    if(channelMap[channel] == 0 || (channelMap[channel]->programNumberStart > programm || channelMap[channel]->programNumberEnd < programm)){
        //reload the new data
        loadNewProgramm(channel, programm);
    }
    
    uint32_t requestedVoiceCount = 0;
    MAPTABLE_ENTRY_t * entries[SIGGEN_VOICECOUNT];
    
    //find entry matching our note
    MAPTABLE_ENTRY_t * currEntry = NULL;
    for(uint32_t currEntryIdx = 0; currEntryIdx < channelMap[channel]->listEntries; currEntryIdx++){
        //get pointer to current entry
        currEntry = MAPPER_ENTRY_FROM_HEADER(channelMap[channel], currEntryIdx);
          
        //does the note fall within the range of the current header entry?
        if(note >= currEntry->startNote && note <= currEntry->endNote){
            //yes :) add entry to the list of ones to process
            entries[requestedVoiceCount++] = currEntry;
        }else{
            //no, make sure to NULL the pointer, incase the loop ends after this entry. We need to know if we stopped the loop because we found the entry or because the list ended
            currEntry = NULL;
        }
    }
    
    //did we find at least one entry?
    if(requestedVoiceCount == 0){
        //no, this means no entry deals with the note currently played, aka it will be silent
        return;
    }
    
    //yes! now prepare the data for VMS
    for(uint32_t i = 0; i < requestedVoiceCount; i++){
        MAPTABLE_DATA_t *data = &(entries[i]->data);
        
        //TODO maybe add an offset to this in the map?
        uint32_t targetOnTime_us = param.pw;
    
        volatile uint32_t targetVolume = (MAX_VOL * data->targetOT) / MAPPER_VOLUME_MAX;
        uint32_t baseVolume = targetVolume;
        
        volatile uint32_t targetFrequency = 0;


        //check which volume effects are enabled and apply them
        if(data->flags & MAP_ENA_DAMPER){
            targetVolume *= MidiProcessor_getDamperVolume(channel);
            targetVolume /= MIDI_VOLUME_MAX;
        }

        if(data->flags & MAP_ENA_STEREO){
            targetVolume *= MidiProcessor_getStereoVolume(channel);
            targetVolume /= MIDI_VOLUME_MAX;
        }

        if(data->flags & MAP_ENA_VOLUME){
            targetVolume *= MidiProcessor_getVolume(channel);
            targetVolume /= MIDI_VOLUME_MAX;
                    
            targetVolume *= velocity;
            targetVolume /= MIDI_VOLUME_MAX;
        }

        if(data->flags & MAP_ENA_EQ){
            //meh this needs to happen after VMS
        }

        //check for note frequency effects

        //get note frequency from map entry and scale to tenths
        if((data->flags & MAP_FREQ_MODE) == FREQ_MODE_OFFSET){
            //octave offset mode
            int16_t currNote = note + data->noteFreq;
            if(currNote > 127 || currNote < 1) return;
            targetFrequency = MidiProcessor_noteToFrequency_dHz(currNote);
        }else{
            //fixed frequency mode
            targetFrequency = data->noteFreq * 10;
        }

        if(data->flags & MAP_ENA_PORTAMENTO){
            /*
             * portamento will be handled differently in this version:
             * 
             * instead of using a VMS_LIN block with a custom slope to shift the frequency around we will instead add another frequency offset: portamentoFactor
             * this will scale the frequency between frequencyTarget and lastFrequency across a value range between 1 and 0
             * 
             * A new portamento block will then be a VMS_LIN of portamentoFactor
             */
            
            //TODO ?
        }
        
        VMSW_startNote(output, data->startblockID, note, channel, targetOnTime_us, targetVolume, targetFrequency, data->flags, baseVolume, velocity);
    }
}

void MAPPER_convertToLegayMapEntry(LegayMapEntry_t * dst, MAPTABLE_ENTRY_t * src){
    if(src == NULL || dst == NULL) return;
    
    dst->startNote = src->startNote;
    dst->endNote = src->endNote;
    
    dst->data.flags = src->data.flags;
    dst->data.noteFreq = src->data.noteFreq;
    dst->data.startblockID = src->data.startblockID + 1;
    dst->data.targetOT = src->data.targetOT;
}

void MAPPER_convertFromLegayMapEntry(MAPTABLE_ENTRY_t * dst, LegayMapEntry_t * src){
    if(src == NULL || dst == NULL) return;
    
    dst->startNote = src->startNote;
    dst->endNote = src->endNote;
    
    dst->data.flags = src->data.flags;
    dst->data.noteFreq = src->data.noteFreq;
    dst->data.startblockID = src->data.startblockID - 1;
    dst->data.targetOT = src->data.targetOT;
}

void MAPPER_convertToLegayMapHeader(LegayMapHeader_t * dst, MAPTABLE_HEADER_t * src){
    if(src == NULL || dst == NULL) return;
    
    dst->listEntries = src->listEntries;
    dst->programNumberStart = src->programNumberStart;
    memcpy(dst->name, src->name, MAPPER_MAP_NAME_LENGTH);
}

void MAPPER_convertFromLegayMapHeader(MAPTABLE_HEADER_t * dst, LegayMapHeader_t * src){
    if(src == NULL || dst == NULL) return;
    
    dst->listEntries = src->listEntries;
    dst->programNumberStart = src->programNumberStart;
    dst->programNumberEnd = src->programNumberStart;
    memcpy(dst->name, src->name, MAPPER_MAP_NAME_LENGTH);
}