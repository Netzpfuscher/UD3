/*
 * VMS - Versatile Modulation System
 *
 * Copyright (c) 2020 Thorben Zethoff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "SignalGenerator.h"
#include "mapper.h"
#include "VMSRoutines.h"
#include "helper/nvm.h"
#include "cli_common.h"
#include "interrupter.h"

#define MAX_PORTAMENTOTIME 100

const struct{
    MAPTABLE_HEADER h0;
    MAPTABLE_ENTRY h0e0;
} DEFMAP = {.h0 = {.programNumber = 0, .listEntries = 1, .name = "DEFMAP"} , .h0e0 = {.startNote = 0, .endNote = 127, .data.VMS_Startblock = (VMS_BLOCK*)&ATTAC, .data.flags = MAP_ENA_DAMPER | MAP_ENA_STEREO | MAP_ENA_VOLUME | MAP_ENA_PITCHBEND | MAP_FREQ_MODE, .data.noteFreq = 0, .data.targetOT = 255}};

void MAPPER_map(uint8_t note, uint8_t velocity, uint8_t channel){
    MAPTABLE_DATA * maps[MIDI_VOICECOUNT];
    uint8_t requestedChannels = MAPPER_getMaps(note, channelData[channel].currentMap, maps);
    if(requestedChannels == 0){
        maps[0] = (MAPTABLE_DATA*)&DEFMAP.h0e0.data;
        requestedChannels = 1;
    }
    while(requestedChannels > 0){
        requestedChannels --;
        uint8_t voice = MAPPER_getNextChannel(note, channel);
        uint32_t currNoteFreq = 0;
        if(maps[requestedChannels]->flags & MAP_FREQ_MODE){
            int16_t currNote = (int16_t) note + maps[requestedChannels]->noteFreq;
            if(currNote > 127 || currNote < 1) return;
            currNoteFreq = Midi_NoteFreq[currNote];
        }else{
            currNoteFreq = maps[requestedChannels]->noteFreq;
        }
        
        if(currNoteFreq < filter.min || currNoteFreq > filter.max) return;
        
        if(maps[requestedChannels]->flags & MAP_ENA_PITCHBEND) currNoteFreq = ((currNoteFreq * channelData[channel].bendFactor) / 20000); else currNoteFreq *= 10;


        int32_t targetOT = ((MAX_VOL>>12) - (MIN_VOL>>12)) * maps[requestedChannels]->targetOT;
        targetOT /= 0xff;

        if(maps[requestedChannels]->flags & MAP_ENA_DAMPER){
            targetOT *= channelData[channel].damperPedal ? 6 : 10;
            targetOT /= 10;
        }

        if(maps[requestedChannels]->flags & MAP_ENA_STEREO){
            targetOT *= channelData[channel].currStereoVolume;
            targetOT /= 0xff;
        }

        if(maps[requestedChannels]->flags & MAP_ENA_VOLUME){
            targetOT *= channelData[channel].currVolume;
            targetOT *= velocity;
            targetOT /= 16129;
        }

        //UART_print("OT=%d;freq=%d;ch=%d;voice=%d\r\n", targetOT, currNoteFreq, channel, voice);
        if(currNoteFreq != 0 && targetOT){
            Midi_voice[voice].currNoteOrigin = channel;

            Midi_voice[voice].noteAge = 0;
            
            Midi_voice[voice].hyperVoiceCount = 0;
            Midi_voice[voice].hyperVoicePhase = 0;

            Midi_voice[voice].currNote = note;
            Midi_voice[voice].on = 1;
            Midi_voice[voice].otTarget = targetOT;
            Midi_voice[voice].otCurrent = 0;

            if(maps[requestedChannels]->flags & MAP_ENA_PORTAMENTO){

                int32_t differenceFactor = (channelData[channel].lastFrequency * 1000) / currNoteFreq;
                if(differenceFactor < 0) differenceFactor = 0;
                if(differenceFactor > 2000000) differenceFactor = 2000000;
                differenceFactor *= 1000;
                Midi_voice[voice].freqFactor = differenceFactor;


                int32_t pTimeTarget = (MAX_PORTAMENTOTIME * channelData[channel].portamentoTime) >> 14;

                //UART_print("differenceFactor = %d (from %d to %d) delta=%d, pTimeTarget = %d, ", differenceFactor, Midi_voice[voice].freqTarget, currNoteFreq, (1000000 - differenceFactor), pTimeTarget);

                if(pTimeTarget == 0){
                    Midi_voice[voice].portamentoParam = (1000000 - differenceFactor);
                }else{
                    Midi_voice[voice].portamentoParam = (1000000 - differenceFactor) / pTimeTarget;
                }
                //UART_print("portamentoParam = %d\r\n", Midi_voice[voice].portamentoParam);
            }else{
                Midi_voice[voice].freqFactor = 1000000;
            }

            Midi_voice[voice].freqTarget = currNoteFreq;
            channelData[channel].lastFrequency = currNoteFreq;

            Midi_voice[voice].noiseCurrent = 0;
            Midi_voice[voice].noiseTarget = 80;
            Midi_voice[voice].noiseFactor = 0;
            SigGen_setNoteTPR(voice, currNoteFreq);

            Midi_voice[voice].map = maps[requestedChannels];

            VMS_resetVoice(&Midi_voice[voice], maps[requestedChannels]->VMS_Startblock);
        }
    }
}

uint8_t MAPPER_getMaps(uint8_t note, MAPTABLE_HEADER * table, MAPTABLE_DATA ** dst){
    uint8_t currScan = 0;
    uint8_t foundCount = 0;
    MAPTABLE_HEADER * temp = table+1;
    MAPTABLE_ENTRY * entries = (MAPTABLE_ENTRY *)temp;
    for(currScan = 0; currScan < table->listEntries && foundCount < MIDI_VOICECOUNT; currScan++){
        if(entries[currScan].endNote >= note && entries[currScan].startNote <= note){
            dst[foundCount++] = &entries[currScan].data;
        }
    }
    return foundCount;
}

uint8_t MAPPER_getNextChannel(uint8_t note, uint8_t channel){
    uint8_t currVoice = 0;
    /*for(currVoice = 0; currVoice < MIDI_VOICECOUNT; currVoice++){
        if(Midi_voice[currVoice].currNote == note && Midi_voice[currVoice].currNoteOrigin == channel && Midi_voice[currVoice].noteAge > 0){
            return currVoice;
        }
    }*/
    
    for(currVoice = 0; currVoice < MIDI_VOICECOUNT; currVoice++){
        if(Midi_isNoteOff(currVoice)){
            return currVoice;
        }
    }
    
    for(currVoice = 0; currVoice < MIDI_VOICECOUNT; currVoice++){
        if(Midi_isNoteDecaying(currVoice)){
            return currVoice;
        }
    }
    
    uint32_t oldestAge = 0;
    uint8_t oldestVoice = 0;
    for(currVoice = 0; currVoice < MIDI_VOICECOUNT; currVoice++){
        if(Midi_voice[currVoice].noteAge > oldestAge){ 
            oldestAge = Midi_voice[currVoice].noteAge;
            oldestVoice = currVoice;
        }
    }
    
    return oldestVoice;
}

void MAPPER_programChangeHandler(uint8_t channel, uint8_t program){
    channelData[channel].currProgramm = program;
    channelData[channel].currentMap = MAPPER_findHeader(program);
}

MAPTABLE_HEADER * MAPPER_findHeader(uint8_t prog){
    MAPTABLE_HEADER * currHeader = (MAPTABLE_HEADER *) NVM_mapMem;
    while(currHeader->listEntries != 0 && currHeader->listEntries != 0xff){
        if(currHeader->programNumber == prog) return currHeader;
        
        uint8_t* calc = (uint8_t*)currHeader;
        calc += sizeof(MAPTABLE_HEADER) + sizeof(MAPTABLE_ENTRY) * currHeader->listEntries;
        currHeader = (MAPTABLE_HEADER *)calc;  //WARNING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        
        //currHeader += sizeof(MAPTABLE_HEADER) + sizeof(MAPTABLE_ENTRY) * currHeader->listEntries;
        
        //if(((uint32_t) currHeader - (uint32_t) NVM_mapMem) >= MAPMEM_SIZE) break;
    }
    //return default maptable
    return (MAPTABLE_HEADER *) &DEFMAP;
}

MAPTABLE_HEADER * cache = 0;
uint8_t cachedIndex = 0;

MAPTABLE_HEADER * MAPPER_getHeader(uint8_t index){
    MAPTABLE_HEADER * currHeader = (MAPTABLE_HEADER *) NVM_mapMem;
    uint8_t currIndex = 0;
    while(currHeader->listEntries != 0 && currHeader->listEntries != 0xff){
        if(currIndex == index){
            cache = currHeader;
            cachedIndex = index;
            return currHeader;
        }
        currHeader += sizeof(MAPTABLE_HEADER) + sizeof(MAPTABLE_ENTRY) * currHeader->listEntries;
        currIndex ++;
        if(((uint32_t) currHeader - (uint32_t) NVM_mapMem) >= MAPMEM_SIZE) break;
    }
    return 0;
}

MAPTABLE_ENTRY * MAPPER_getEntry(uint8_t header, uint8_t index){
    MAPTABLE_HEADER * table;
    if(cachedIndex == header && cache != 0){
        table = cache;
    }else{
        table = MAPPER_getHeader(header);
    }
    
    MAPTABLE_ENTRY * entries = (MAPTABLE_ENTRY *) ((uint32_t) table + sizeof(MAPTABLE_HEADER));
    uint8_t currScan = 0;
    for(currScan = 0; currScan < table->listEntries; currScan++){
        if(currScan == index){
            return &entries[currScan];
        }
    }
    return 0;
}

void MAPPER_volumeHandler(uint8_t channel){
    uint8_t currChannel = 0;
    for(;currChannel < 4; currChannel ++){
        if(Midi_voice[currChannel].currNoteOrigin == channel && Midi_voice[currChannel].on){
            MAPTABLE_DATA * map = Midi_voice[currChannel].map;
            
        
            int32_t targetOT = ((MAX_VOL>>12) - (MIN_VOL>>12)) * map->targetOT;
            targetOT /= 0xff;

            if(map->flags & MAP_ENA_DAMPER){
                targetOT *= channelData[channel].damperPedal ? 6 : 10;
                targetOT /= 10;
            }

            if(map->flags & MAP_ENA_STEREO){
                targetOT *= channelData[channel].currStereoVolume;
                targetOT /= 0xff;
            }

            if(map->flags & MAP_ENA_VOLUME){
                targetOT *= channelData[channel].currVolume;
                targetOT /= 127;
            }
            
            Midi_voice[currChannel].otTarget = targetOT;
            
            if(Midi_voice[currChannel].otFactor > 0){
                Midi_voice[currChannel].otCurrent = (Midi_voice[currChannel].otTarget * Midi_voice[currChannel].otFactor) / 1000000 + (MIN_VOL>>12);
            }
        }
    }
}

void MAPPER_bendHandler(uint8_t channel){
    uint8_t currChannel = 0;
    for(;currChannel < 4; currChannel ++){
        if(Midi_voice[currChannel].currNoteOrigin == channel && Midi_voice[currChannel].otCurrent > 0){
            MAPTABLE_DATA * map = Midi_voice[currChannel].map;
            if((map->flags & MAP_ENA_PITCHBEND) == 0) continue;
            
            uint32_t currNoteFreq = 0;
            if(map->flags & MAP_FREQ_MODE){
                int16_t currNote = (int16_t) Midi_voice[currChannel].currNote + map->noteFreq;
                if(currNote > 127 || currNote < 1) return;
                currNoteFreq = Midi_NoteFreq[currNote];
            }else{
                currNoteFreq = map->noteFreq;
            }
            currNoteFreq = ((currNoteFreq * channelData[channel].bendFactor) / 20000);
            
            Midi_voice[currChannel].freqTarget = currNoteFreq;
            Midi_voice[currChannel].freqCurrent = (Midi_voice[currChannel].freqTarget * (Midi_voice[currChannel].freqFactor >> 4)) / 62500;  //62500 is 1000000 >> 4
            
            SigGen_setNoteTPR(currChannel, Midi_voice[currChannel].freqCurrent);
        }
    }
}

void MAPPER_handleMapWrite(){
    uint8_t currChannel = 0;
    for(currChannel = 0; currChannel < 16; currChannel++){
        MAPPER_programChangeHandler(currChannel, channelData[currChannel].currProgramm);
    }
}