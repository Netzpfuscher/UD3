/*
 * File:   VMSWrapper.c
 * Author: Thorb
 *
 * Created on 17 January 2023, 20:25
 */

#include "SignalGenerator.h"
#include "VMS.h"
#include "FreeRTOS.h"
#include "task.h"
#include "VMSWrapper.h"
#include "MidiProcessor.h"
#include "VMSDefaults.h"
#include "NoteMapper.h"
#include "interrupter.h"
#include "cli_common.h"
#include "helper/nvm.h"
#include "TTerm.h"
#include "Tasks/tsk_cli.h"

static VMS_VoiceData_t ** voiceData;

static uint32_t VMSW_getNextVoice(uint32_t note, uint32_t output, uint32_t channel);
static void VMSW_task(void * params);

static uint32_t blockMemSize = VMS_DEFAULT_BLOCKMEM_SIZE;
static uint32_t blockMemBlockCount = VMS_DEFAULT_BLOCKMEM_SIZE/sizeof(VMS_Block_t);

uint32_t VMSW_getBlockMemSizeInBlocks(){
    return blockMemBlockCount;
}

void VMSW_init(){
    //add vms memory to conman
    
    voiceData = pvPortMalloc(sizeof(VMS_VoiceData_t *) * SIGGEN_OUTPUTCOUNT);
    for(uint32_t i = 0; i < SIGGEN_OUTPUTCOUNT; i++){
        voiceData[i] = pvPortMalloc(sizeof(VMS_VoiceData_t) * SIGGEN_VOICECOUNT);
        
        //clear memory
        memset(voiceData[i], 0, sizeof(VMS_VoiceData_t) * SIGGEN_VOICECOUNT);
    }
    
    VMS_init();
    
    xTaskCreate(VMSW_task, "VMS Task", configMINIMAL_STACK_SIZE+256, NULL, tskIDLE_PRIORITY + 3, NULL);
}

static void VMSW_task(void * params){
    while(1){
        //run VMS service
        VMS_run();
        vTaskDelay(1);
    }
}

void VMSW_stopNote(uint32_t output, uint32_t note, uint32_t channel){
    if(output >= SIGGEN_OUTPUTCOUNT) return;
    
    VMS_getSemaphore();
    //find which voice is playing the note (if one is even doing that...)
    for(uint32_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
        //does the channel and the note match? If so => clear the note on bit, VMS should to the rest
        if(voiceData[output][currVoice].on && voiceData[output][currVoice].sourceChannel == channel && voiceData[output][currVoice].sourceNote == note){ 
            voiceData[output][currVoice].on = 0;
            
            //TODO could this mess up portamento?
            voiceData[output][currVoice].freqLast = voiceData[output][currVoice].freqCurrent;
        }
    }
    VMS_returnSemaphore();
}

void VMSW_startNote(uint32_t output, uint32_t startBlockID, uint32_t sourceNote, uint32_t sourceChannel, uint32_t targetOnTime, uint32_t targetVolume, uint32_t targetFrequency, uint32_t flags, uint32_t baseVolume, uint32_t baseVelocity){
    //would the note even be on after this? If not just go back without doing anything
    if(targetFrequency == 0 || targetVolume == 0) return;
    
    //TODO remove this hack once portamento works...
    flags &= ~MAP_ENA_PORTAMENTO;
    
    //find voice to play the sound
    uint32_t targetVoice = VMSW_getNextVoice(sourceNote, output, sourceChannel);
    
    VMS_VoiceData_t * vData = &(voiceData[output][targetVoice]);
    
    //yes, first remove all VMS blocks, then disable the siggen output
    VMS_getSemaphore();
    VMS_removeBlocksWithTargetVoice(targetVoice);
    SigGen_setVoiceVMS(targetVoice, 0, 0, 0, 0, 0, 0, 0);
    VMS_returnSemaphore();
    
    //reset voice data
    uint32_t freqLast = vData->freqLast;
    memset(&voiceData[output][targetVoice], 0, sizeof(VMS_VoiceData_t));
    
    //this is already a factor, no factor value needed. Current value <=> currentFactor
    vData->freqLast = freqLast;
    vData->portamentoTarget = 0;
    vData->portamentoCurrent = (flags & MAP_ENA_PORTAMENTO) ? VMS_UNITY_FACTOR : 0;
    
    vData->freqTarget = targetFrequency;
    
    //apply frequency effects
    uint32_t newFrequency = targetFrequency;
    
    //pitchbend
    if(flags & MAP_ENA_PITCHBEND) newFrequency = MIDIPROC_SCALE_PITCHBEND(newFrequency, vData->sourceChannel);
    
    //portamento, check if we need to apply it first though
    if(flags & MAP_ENA_PORTAMENTO){ 
        int32_t difference = (int32_t) freqLast - (int32_t) vData->freqTarget;
        difference = (difference * (vData->portamentoCurrent>>4)) / 62500; 
        newFrequency += difference;
    }
    
    //and finally apply that frequency
    vData->freqCurrent = newFrequency;
    vData->freqFactor = 1000000;
    
    vData->volumeTarget = targetVolume;
    
    //TODO should we perhaps start this off at 0?
    vData->onTimeTarget = targetOnTime;
    vData->onTimeCurrent = targetOnTime;
    vData->onTimeFactor = 1000000;
    
    vData->noiseTarget = 35000; //TODO make this better? idk a fixed value here might be bad especially for low notes, as the period change is relatively minor in relation to the overall period, maybe make this depended on the freq
    
    vData->startTime = xTaskGetTickCount();
    vData->sourceChannel = sourceChannel;
    vData->sourceNote = sourceNote;
    
    vData->hypervoiceVolumeFactor = VMS_UNITY_FACTOR;
    
    //get the current volume from the factor. We need to scale this to prevent overflow :/
    if(vData->volumeTarget == MAX_VOL){
        //volume is max, no need to scale the volume, just write a scaled down version of the factor into the voice
        vData->hypervoiceVolumeCurrent = vData->hypervoiceVolumeFactor >> 4; //LOSSY the axual divisor should be 15.258...
    }else{
        vData->hypervoiceVolumeCurrent = (((uint32_t)vData->hypervoiceVolumeFactor>>4) *  vData->volumeTarget) / 62500; //62500 = 1000000 >> 4
    }
    
    vData->hypervoiceCount = 0;
    vData->hypervoicePhase = 0;
    
    vData->voiceIndex = targetVoice;
    
    vData->flags = flags;
    vData->baseVelocity = baseVelocity;
    vData->baseVolume = baseVolume;
    
    vData->on = 1;
    
    VMS_getSemaphore();
    
    SigGen_setVoiceVMS(targetVoice, 0, 0, 0, 0, 0, 0, 0);
    SigGen_setHyperVoiceVMS(targetVoice, 0, 0, 0, 0);
    
    VMS_addBlockToList(VMSW_getBlockPtr(startBlockID), output, targetVoice);
    VMS_returnSemaphore();
}

inline uint32_t VMSW_getSrcChannel(uint32_t output, uint32_t voice){
    return voiceData[output][voice].sourceChannel;
}

inline uint32_t VMSW_isVoiceOn(uint32_t output, uint32_t voice){
    return voiceData[output][voice].on;
}

inline uint32_t VMSW_isVoiceActive(uint32_t output, uint32_t voice){
    return voiceData[output][voice].volumeCurrent > 0 || voiceData[output][voice].on;
}

uint32_t VMSW_isAnyVoiceOn(){
    for(uint32_t j = 0; j < SIGGEN_OUTPUTCOUNT; j++){
        for(uint32_t i = 0; i < SIGGEN_VOICECOUNT; i++){
            if(voiceData[j][i].on > 0) return 1;
        }
    }
    return 0;
}

inline VMS_Block_t * VMSW_getBlockPtr(uint32_t index){
    if(index == VMS_BLOCKID_DEFATTACK) return &VMS_DEFAULT_ATTAC;
    if(index == VMS_BLOCKID_DEFSUSTAIN) return &VMS_DEFAULT_SUSTAIN;
    if(index == VMS_BLOCKID_DEFRELEASE) return &VMS_DEFAULT_RELEASE;
    
    if(index >= blockMemBlockCount) return NULL;
    
    if(!VMS_isBlockValid(&NVM_blocks[index])) return NULL;
    
    //TODO readd block list pointer to nvm... it just makes things easier :3
    return &NVM_blocks[index];
}

int32_t VMSW_getKnownValue(KNOWN_VALUE ID, uint32_t output, uint32_t voiceId){
    //UART_print("get %d\r\n", ID);
    switch(ID){
        case maxOnTime:
            return configuration.max_tr_pw;
        case minOnTime:
            return 0;
        case volumeCurrent:
            return voiceData[output][voiceId].volumeCurrent;
        case volumeTarget:
            return voiceData[output][voiceId].volumeTarget;
        case volumeFactor:
            return voiceData[output][voiceId].volumeFactor;
        case freqCurrent:
            return voiceData[output][voiceId].freqCurrent;
        case freqTarget:
            return voiceData[output][voiceId].freqTarget;
        case freqFactor:
            return voiceData[output][voiceId].freqFactor;
        case pTime:
            //TODO!
            return 0;
            
        case circ1 ... circ4:
            return voiceData[output][voiceId].circ[ID - circ1];
            
        case HyperVoice_Volume:
            return voiceData[output][voiceId].hypervoiceVolumeFactor;
            
        case HyperVoice_Count:
            return voiceData[output][voiceId].hypervoiceCount;
            
        case HyperVoice_Phase:
            return voiceData[output][voiceId].hypervoicePhase;  //TODO? map to 0 - 0xff
            
        case CC_102 ... CC_119:
            return MidiProcessor_getCCValue(VMSW_getSrcChannel(output, voiceId), ID - CC_102);
    }
    return 0;
}

void VMSW_setKnownValue(KNOWN_VALUE ID, int32_t value, uint32_t output, uint32_t voiceId){
    //precalculate voice data pointer, as this might be repeated multiple times in the following code otherwise
    VMS_VoiceData_t * vData = &(voiceData[output][voiceId]);
    
    switch(ID){
        //all other values can't be written by VMS
        default:
            return;
            
        case onTime:
            //update volume. ontime is no longer used up until the very last stage
            vData->onTimeFactor = value;
            
            vData->onTimeCurrent = (((uint32_t)value>>4) *  vData->onTimeTarget) / 62500; //62500 = 1000000 >> 4
            
            //update siggen values
            //TODO implement burst into VMS?
            //TODO add custom hypervoice pulsewidth?
            SigGen_setVoiceVMS(voiceId, 1, vData->onTimeCurrent, vData->volumeCurrent, vData->freqCurrent, vData->noiseCurrent, 0, 0);
            if(vData->hypervoiceCount > 0) SigGen_setHyperVoiceVMS(voiceId, vData->hypervoiceCount / 1000000, vData->onTimeCurrent, vData->hypervoiceVolumeCurrent, vData->hypervoicePhase / 977);
            break;
            
            break;
        
        case volumeCurrent:
            //update volume. ontime is no longer used up until the very last stage
            vData->volumeFactor = value;
            
            //get the current volume from the factor. We need to scale this to prevent overflow :/
            if(vData->volumeTarget == MAX_VOL){
                //volume is max, no need to scale the volume, just write a scaled down version of the factor into the voice
                vData->volumeCurrent = value >> 4; //LOSSY the axual divisor should be 15.258...
            }else{
                vData->volumeCurrent = (((uint32_t)value>>4) *  vData->volumeTarget) / 62500; //62500 = 1000000 >> 4
            }
            
            if(vData->volumeCurrent == MAX_VOL){
                //volume is max, no need to scale the volume, just write a scaled down version of the factor into the voice
                vData->hypervoiceVolumeCurrent = vData->hypervoiceVolumeFactor >> 4; //LOSSY the axual divisor should be 15.258...
            }else{
                vData->hypervoiceVolumeCurrent = (((uint32_t)vData->hypervoiceVolumeFactor>>4) *  vData->volumeCurrent) / 62500; //62500 = 1000000 >> 4
            }
            
            //update siggen values
            SigGen_setVoiceVMS(voiceId, 1, vData->onTimeCurrent, vData->volumeCurrent, vData->freqCurrent, vData->noiseCurrent, 0, 0);
            break;
            
        case freqCurrent:
            vData->freqCurrent = value;
            break;
            
        case frequency:
            vData->freqFactor = value;
            
            //calculate current frequency
            uint32_t newFrequency = vData->freqTarget;
            
            //apply frequency effects
            
            //pitchbend
            
            if(vData->flags & MAP_ENA_PITCHBEND) newFrequency = MIDIPROC_SCALE_PITCHBEND(newFrequency, vData->sourceChannel);
            
            //portamento, check if we need to apply it first though
            if(vData->portamentoCurrent != 0){ 
                int32_t difference = (int32_t) vData->freqLast - (int32_t) vData->freqTarget;
                difference = (difference * (vData->portamentoCurrent>>4)) / 62500; 
                newFrequency += difference;
            }
            
            //and finally, the currentfreqFactor
            if(value != 1000000) newFrequency = (newFrequency * (value>>4)) / 62500;
            
            vData->freqCurrent = newFrequency;
            
            SigGen_setVoiceVMS(voiceId, 1, vData->onTimeCurrent, vData->volumeCurrent, vData->freqCurrent, vData->noiseCurrent, 0, 0);
            break;
            
        case noise:
            vData->noiseFactor = value;
            vData->noiseCurrent = (((uint32_t)value>>4) *  vData->noiseTarget) / 62500; //62500 = 1000000 >> 4
            
            SigGen_setVoiceVMS(voiceId, 1, vData->onTimeCurrent, vData->volumeCurrent, vData->freqCurrent, vData->noiseCurrent, 0, 0);
            break;
            
        case circ1 ... circ4:
            vData->circ[ID - circ1] = value;
            break;

        case HyperVoice_Count:
            vData->hypervoiceCount = value;
            
            SigGen_setHyperVoiceVMS(voiceId, vData->hypervoiceCount / 1000000, vData->onTimeCurrent, vData->hypervoiceVolumeCurrent, vData->hypervoicePhase / 977);
            break;

        case HyperVoice_Volume:
            vData->hypervoiceVolumeFactor = value;
            
            //get the current volume from the factor. We need to scale this to prevent overflow :/
            if(vData->volumeCurrent == MAX_VOL){
                //volume is max, no need to scale the volume, just write a scaled down version of the factor into the voice
                vData->hypervoiceVolumeCurrent = value >> 4; //LOSSY the axual divisor should be 15.258...
            }else{
                vData->hypervoiceVolumeCurrent = (((uint32_t)value>>4) *  vData->volumeCurrent) / 62500; //62500 = 1000000 >> 4
            }
            
            SigGen_setHyperVoiceVMS(voiceId, vData->hypervoiceCount / 1000000, vData->onTimeCurrent, vData->hypervoiceVolumeCurrent, vData->hypervoicePhase / 977);
            break;
            
        case HyperVoice_Phase:
            vData->hypervoicePhase = value;  //map to 0 - 0xff
            
            SigGen_setHyperVoiceVMS(voiceId, vData->hypervoiceCount / 1000000, vData->onTimeCurrent, vData->hypervoiceVolumeCurrent, vData->hypervoicePhase / 977);
            break;
    }
}

int32_t VMSW_getCurrentFactor(KNOWN_VALUE ID, uint32_t output, uint32_t voiceId){
    switch(ID){
        default:
            return 0;
        case onTime:
            return voiceData[output][voiceId].volumeFactor;
        case frequency:
            return voiceData[output][voiceId].freqFactor;
        case noise:
            return voiceData[output][voiceId].noiseFactor;
            
        case circ1 ... circ4:
            return voiceData[output][voiceId].circ[ID - circ1];
            
        case HyperVoice_Count:
            return voiceData[output][voiceId].hypervoiceCount;
        case HyperVoice_Volume:
            return voiceData[output][voiceId].hypervoiceVolumeFactor;
        case HyperVoice_Phase:
            return voiceData[output][voiceId].hypervoicePhase;
    }
    return 0;
}

static uint32_t VMSW_getNextVoice(uint32_t note, uint32_t output, uint32_t channel){
    //find the best matching voice to play the next note. Going through criteriums best to worst
    
    /*//is any note decaying that already was already playing this note before?
    for(uint8_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
        if(!voiceData[currVoice].on && voiceData[currVoice].sourceNote == note){
            return currVoice;
        }
    }*/
    
    //is any voice completely off at the moment?
    for(uint8_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
        if(!VMSW_isVoiceActive(output, currVoice)){
            return currVoice;
        }
    }
    
    //Find the oldest voice to kill
    uint32_t oldestAge = 0;
    uint32_t oldestVoice = 0;
    for(uint8_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
        uint32_t currNoteAge = xTaskGetTickCount() - voiceData[output][currVoice].startTime;
        if(currNoteAge > oldestAge){ 
            oldestAge = currNoteAge;
            oldestVoice = currVoice;
        }
    }
    
    return oldestVoice;
}

void VMSW_panicHandler(){
    VMS_getSemaphore();
    VMS_killBlocks();
    
    //also kill all voices
    for(uint32_t currOutput = 0; currOutput < SIGGEN_OUTPUTCOUNT; currOutput++){
        for(uint8_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
            voiceData[currOutput][currVoice].on = 0;
            voiceData[currOutput][currVoice].volumeCurrent = 0;
            voiceData[currOutput][currVoice].freqCurrent = 0;
            voiceData[currOutput][currVoice].freqLast = 0; //TODO: make sure this won't make portamento sound shit
        }
    }
    VMS_returnSemaphore();
}

void VMSW_bendHandler(uint32_t channel){
    VMS_getSemaphore();
    //check all notes. Find which ones need to be updated
    for(uint32_t currOutput = 0; currOutput < SIGGEN_OUTPUTCOUNT; currOutput++){
        for(uint8_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
            //precalculate voice data pointer, as this might be repeated multiple times in the following code otherwise
            VMS_VoiceData_t * vData = &(voiceData[currOutput][currVoice]);
            
            if(vData->sourceChannel == channel && (vData->on > 0)){
                //call VMSW_setKnownValue to trigger a value recalculation

                //is pitchbend enabled on the current voice? If not just skip calculations
                if(!(vData->flags & MAP_ENA_PITCHBEND)) continue;
                
                VMSW_setKnownValue(frequency, vData->freqFactor, currOutput, currVoice);
            }
        }
    }
    VMS_returnSemaphore();
}

void VMSW_volumeChangeHandler(uint32_t channel){
    TERM_printDebug(min_handle[1], "volume change ch=%d vol=%d\r\n", channel, MidiProcessor_getVolume(channel));
    
    VMS_getSemaphore();
    
    for(uint32_t currOutput = 0; currOutput < SIGGEN_OUTPUTCOUNT; currOutput++){
        for(uint8_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
            //precalculate voice data pointer, as this might be repeated multiple times in the following code otherwise
            VMS_VoiceData_t * vData = &(voiceData[currOutput][currVoice]);
            
            //is the current block originating from this channel
            if(vData->sourceChannel == channel && (vData->on > 0)){
                volatile uint32_t targetVolume = vData->baseVolume;

                if(vData->flags & MAP_ENA_STEREO){
                    targetVolume *= MidiProcessor_getStereoVolume(channel);
                    targetVolume /= MIDI_VOLUME_MAX;
                }

                if(vData->flags & MAP_ENA_VOLUME){
                    targetVolume *= MidiProcessor_getVolume(channel);
                    targetVolume /= MIDI_VOLUME_MAX;

                    targetVolume *= vData->baseVelocity;
                    targetVolume /= MIDI_VOLUME_MAX;
                }
                
                //TODO we actually need to figure out if the map scaled this volume...
                
                //did we actually change something?
                if(vData->volumeTarget != targetVolume){
                    //yes! update parameters
                    vData->volumeTarget = targetVolume;
                    
                    //call VMSW_setKnownValue to trigger a value recalculation
                    VMSW_setKnownValue(volumeCurrent, vData->volumeFactor, currOutput, currVoice);
                }

            }
        }
    }
    
    VMS_returnSemaphore();
}

void VMSW_pulseWidthChangeHandler(){
    VMS_getSemaphore();
    
    for(uint32_t currOutput = 0; currOutput < SIGGEN_OUTPUTCOUNT; currOutput++){
        for(uint8_t currVoice = 0; currVoice < SIGGEN_VOICECOUNT; currVoice++){
            //precalculate voice data pointer, as this might be repeated multiple times in the following code otherwise
            VMS_VoiceData_t * vData = &(voiceData[currOutput][currVoice]);
            
            
            //if the voice is off then we won't touch it. 
            //TODO perhaps we should? This means decaying notes won't be affected by pw changes
            if(!vData->on) continue;
            
            volatile uint32_t targetPw = param.pw;

            //did we actually change something?
            if(vData->onTimeTarget != targetPw){
                //yes! update parameters
                vData->onTimeTarget = targetPw;
                
                //call VMSW_setKnownValue to trigger a value recalculation
                VMSW_setKnownValue(onTime, vData->onTimeFactor, currOutput, currVoice);
            }
        }
    }
    
    VMS_returnSemaphore();
}