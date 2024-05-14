#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "SignalGenerator.h"

#include "VMS.h"
#include "VMSWrapper.h"

#include "DLL/include/DLL.h"
#include "MidiProcessor.h"
#include "task.h"
#include "semphr.h"
#include "utilH/include/util.h"
#include "tasks/tsk_cli.h"

static DLLObject * VMS_listHead = 0;

static void VMS_removeBlockFromList(VMS_listDataObject * target);
static unsigned VMS_hasReachedThreshold(const VMS_Block_t * block, int32_t factor, int32_t targetFactor, int32_t param1);
//static int32_t VMS_getParam(VMS_Block_t * block, uint32_t voiceId, uint8_t param);
static void VMS_nextBlock(VMS_listDataObject * data, uint32_t blockSet);
static DIRECTION VMS_getThresholdDirection(const VMS_Block_t * block, int32_t param1);
static int32_t VMS_getParameter(uint8_t param, const VMS_Block_t * block, uint32_t voiceId, uint32_t outputId);
static unsigned VMS_calculateValue(VMS_listDataObject * data);

SemaphoreHandle_t vmsSemaphore = NULL;

void VMS_init(){
    //initialise the block list
    vmsSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(vmsSemaphore);
            
    VMS_listHead = DLL_create();
}

void VMS_run(){ 
    //get acces permission to the VMS objects
    VMS_getSemaphore();
    
    //step through the list of blocks
    DLLObject * currObject = VMS_listHead->next;
    
    while(currObject != VMS_listHead){
        VMS_listDataObject * currBlock = currObject->data;
        
        //calculate the next object to be processed before VMS_calculateValue might free the current one
        currObject = currObject->next;
        
        //do we need to run the block?
        if(VMSW_GET_TIME_MS() > currBlock->nextRunTime){
            //update the next run time
            currBlock->nextRunTime = VMSW_GET_TIME_MS() + currBlock->periodMs;
            
            //yes :) lets do so then
            VMS_calculateValue(currBlock);
        }
    }
    
    //and return it for other tasks
    VMS_returnSemaphore();
}

void VMS_getSemaphore(){
    if(vmsSemaphore == NULL) return;
    xSemaphoreTake(vmsSemaphore, portMAX_DELAY);
}

void VMS_returnSemaphore(){
    if(vmsSemaphore == NULL) return;
    xSemaphoreGive(vmsSemaphore);
}

void VMS_removeBlocksWithTargetVoice(uint32_t targetVoice){
    //check for blocks that do something with the targetVoice, if there are any: delete them 
    DLLObject * currObject = VMS_listHead->next;
    
    //step through the list
    while(currObject != VMS_listHead){
        VMS_listDataObject * currBlock = currObject->data;
        
        //calculate the next object to be processed before VMS_calculateValue might free the current one
        currObject = currObject->next;
        
        //does the index match?
        if(currBlock->targetVoiceIndex == targetVoice){
            //yep. Delete that block from ~existence~ the list (kekW)
            VMS_removeBlockFromList(currBlock);
        }
    }
}

uint32_t VMS_isBlockValid(const VMS_Block_t * block){
    //was a NULL pointer given or is the block in unfilled memory?
    if(block == NULL || block->flags == VMS_FLAG_BLOCK_INVALID) return 0;
    
    
    //check for weird PSOC flash being 0 without initialisation
    if(block->flags == 0 && block->type == 0) return 0;
    
    //TODO we should probably make this test more things than just for memory being 0xffffffff
    return 1;
}

void VMS_addBlockToList(const VMS_Block_t * block, uint32_t output, uint32_t voiceId){
    //add a block to the dll
    
    //is it valid?
    if(block == NULL || block->flags == VMS_FLAG_BLOCK_INVALID) return;
    
    //allocate and initialize memory
    VMS_listDataObject * data = pvPortMalloc(sizeof(VMS_listDataObject));
    memset(data, 0, sizeof(VMS_listDataObject));
    
    data->block = block;
    data->targetOutputIndex = output;
    data->targetVoiceIndex = voiceId;
    data->periodMs = block->periodMs;
    data->nextRunTime = VMSW_GET_TIME_MS() + data->block->periodMs;
    
    //does the block have some custom data associated with it? If so allocate that as well
    switch(block->type){
        case VMS_SIN:
            data->data = pvPortMalloc(sizeof(SINE_DATA));
            memset(data->data, 0, sizeof(SINE_DATA));
            break;
        default:
            break;
    }
    
    //add it to the list
    DLL_add(data, VMS_listHead);
}

void VMS_killBlocks(){
    //step through the list and KILL EVERYTHING
    DLLObject * currObject = VMS_listHead->next;
    
    while(currObject != VMS_listHead){
        VMS_listDataObject * currBlock = (VMS_listDataObject *) currObject->data;
        
        //calculate the next object to be processed before VMS_calculateValue might free the current one
        currObject = currObject->next;
        
        VMS_removeBlockFromList(currBlock);
    }
}

static unsigned VMS_hasReachedThreshold(const VMS_Block_t * block, int32_t factor, int32_t targetFactor, int32_t param1){
    //check if the block has reached its target value
    
    //is it a VMS_JUMP block? If so just return 1, as it immediately sets the target value anyway
    if(block->type == VMS_JUMP) return 1;
    
    //add a special case for compatibility with old vms blocks:
    //is this an old portamento block? That was a linear one with parameter 1 (aka m/steepness) variable and sourced from ptime
    //first check if p1 is even variable
    if(block->flags & VMS_FLAG_ISVARIABLE_PARAM1){
        //yes => load range parameters
        RangeParameters * range = (RangeParameters *)&block->param1;
        
        //is the source pTime? If so just say we reached the end of the block
        if(range->sourceId == pTime) return 1;
    }
    
    //check for threshold direction and return accordingly
    switch(VMS_getThresholdDirection(block, param1)){
        case RISING:
            return factor >= targetFactor;
        case FALLING:
            return factor <= targetFactor;
        case NONE:
            return 0;
        case ANY:
            return 0;
    }
    
    return 1;
}

//remove and free data of a block
static void VMS_removeBlockFromList(VMS_listDataObject * target){
    //UART_print("removing block 0x%08x\r\n", target);
    //does the block have custom data?
    switch(target->block->type){
        case VMS_SIN:
            vPortFree(target->data);
            break;
        default:
            break;
    }
    
    //remove the block from the list
    DLL_removeData(target, VMS_listHead);
    
    //free the data
    vPortFree(target);
}

static void VMS_nextBlock(VMS_listDataObject * data, uint32_t blockSet){
    //UART_print("loading next block for data 0x%08x\r\n", data);
    //load the next block
    
    uint32_t outputId = data->targetOutputIndex;
    uint32_t voiceId = data->targetVoiceIndex;
    const VMS_Block_t * block = data->block;
    
    //which one of the two sets do we need?
    if(blockSet == VMS_BLOCKSET_NEXTBLOCKS){
        //check if our block is a sustain block
        if(block->flags & VMS_FLAG_ISBLOCKPERSISTENT){
            //yes! that means it will not be removed just by the threshold being reached. Return so VMS_removeBlockFromList will not be called and the block can keep running
            //also we must make sure no next blocks get loaded as the list will explode otherwise ;)
            return;
        }
        
        //step through the nextBlock indices
        for(uint32_t c = 0; c < VMS_MAX_BRANCHES; c++){ //kekW
            //is there another block to load?
            if(block->nextBlocks[c] != VMS_BLOCKID_INVALID){
                //add new block to the list
                VMS_addBlockToList(VMSW_getBlockPtr(block->nextBlocks[c]), outputId, voiceId);
            }else{
                break;
            }
        }
    }else{
        //offblocks... no need to check for VMS_SUSTAIN_BLOCK_UNTIL_NOTEOFF, as a inverted note-event (so noteOff for a normal block or noteOn for an inverted one) always kills the block
        if(block->offBlock != 0){
            //add new block to the list
            VMS_addBlockToList(VMSW_getBlockPtr(block->offBlock), outputId, voiceId);
        }
    }
    
    //remove the current block. This won't get called if return is called in the block->nextBlocks[c] == VMS_SUSTAIN_BLOCK_UNTIL_NOTEOFF statement earlier!
    VMS_removeBlockFromList(data);
}

static unsigned VMS_calculateValue(VMS_listDataObject * data){
    //run VMS calculations
    uint32_t outputId = data->targetOutputIndex;
    uint32_t voiceId = data->targetVoiceIndex;
    const VMS_Block_t * block = data->block;
    
    //first check if the block needs to be killed because of a noteOff/noteOn event
    if(block->behavior == NORMAL){
        if(MidiProcessor_getCCValue(VMSW_getSrcChannel(outputId, voiceId), MIDI_CC_SUSTAIN_PEDAL) < 0x40 && !VMSW_isVoiceOn(outputId, voiceId)){
            VMS_nextBlock(data, 0);
            return 0;
        }
    }
    
    //get parameters for calculation. Could be computed from KnownValues
    int32_t param1 = VMS_getParameter(1, block, outputId, voiceId);
    int32_t param2 = VMS_getParameter(2, block, outputId, voiceId);
    int32_t param3 = VMS_getParameter(3, block, outputId, voiceId);
    int32_t targetFactor = VMS_getParameter(4, block, outputId, voiceId);
    
    //get current factor
    int32_t currFactor = VMSW_getCurrentFactor(block->target, outputId, voiceId);
    
    SINE_DATA * d = NULL;
    int32_t currFactorDiff;
    
    //calculate values
    switch(block->type){
        case VMS_EXP:
            //is the exp function increasing the value (aka. is the coefficient larger than 1)?
            if(param1 > 1000 && currFactor < 1000){
                //yes! now we need to make sure, that starting from a very low currentFactor doesn't take forever (or even infinitely long if currFactor == 0)
                //set factor to 1000 (0.001)
                currFactor = 1000;
            }
            currFactor = (currFactor * param1) / 1000;
            
            if(param1 < 5000 && currFactor < 5000){
                currFactor = 0;
            }
            
            break;
            
        case VMS_EXP_INV:
            currFactorDiff = abs(targetFactor - currFactor);
            currFactorDiff = (currFactorDiff * param1) / 1000;
            if(currFactorDiff < 10000){
                currFactorDiff = 0;
            }
            currFactor = targetFactor - currFactorDiff;
            break;
            
        case VMS_LIN:
            currFactor += param1;
            
            break;
        case VMS_SIN:
            d = (SINE_DATA *) data->data;
            d->currCount += param3;
            if(d->currCount > 0xff) d->currCount = -0xff;
            currFactor = ((param1 * qSin(d->currCount)) / 1000);
            currFactor += param2;
            if(currFactor < 0) currFactor = 0;
            break;
        case VMS_JUMP:
            currFactor = targetFactor;
            break;
        case VMS_INVALID:
            break;
    }
    
    //did the factor reach the designated target threshold?
    if(VMS_hasReachedThreshold(block, currFactor, targetFactor, param1)){
        //yeah, load the next block
        currFactor = targetFactor;
        VMS_nextBlock(data, 1);
        VMSW_setKnownValue(block->target, currFactor, outputId, voiceId);
        return 0;
    }
    
    //and finally write the new value to the buffer
    VMSW_setKnownValue(block->target, currFactor, outputId, voiceId);
    return 1;
}

//TODO evaluate refactoring :3
static int32_t VMS_getParameter(uint8_t param, const VMS_Block_t * block, uint32_t outputId, uint32_t voiceId){
    //load a parameter from the block in flash, or compute it from a known value if the flag is set for it
    
    RangeParameters * range = NULL;//&block->param1;
    
    //is the parameter we are trying to read variable? If not we can just return the parameter itself. Otherwise assign the RangeParameters variable
    switch(param){
        case 1:
            if(block->flags & VMS_FLAG_ISVARIABLE_PARAM1){
                range = (RangeParameters *)&block->param1;
            }else{
                return block->param1;
            }
            break;
            
        case 2:
            if(block->flags & VMS_FLAG_ISVARIABLE_PARAM1){
                range = (RangeParameters *)&block->param2;
            }else{
                return block->param2;
            }
            break;
            
        case 3:
            if(block->flags & VMS_FLAG_ISVARIABLE_PARAM1){
                range = (RangeParameters *)&block->param3;
            }else{
                return block->param3;
            }
            break;
            
        case 4:
            if(block->flags & VMS_FLAG_ISVARIABLE_PARAM1){
                range = (RangeParameters *)&block->targetFactor;
            }else{
                return block->targetFactor;
            }
            break;
    }
    
    //catch a out of range parameter access, or any other problem that could case a NULL pointer access
    if(range == NULL) return 0;
    
    KNOWN_VALUE source = range->sourceId;
    int32_t slope = range->rangeEnd - range->rangeStart;
    
    int32_t ret = 0;
    
    //read out value and scale accordingly
    if(source == freqCurrent){    //does the parameter selected support range mapping?
        ret = range->rangeStart + (slope * VMSW_getKnownValue(freqCurrent, outputId, voiceId)) / 20000;
        
    }else if(source >= circ1 && source <= circ4){    //does the parameter selected support range mapping?
        ret = range->rangeStart + (slope * (VMSW_getKnownValue(source, outputId, voiceId) >> 4)) / 62500;
        
    }else if(source >= CC_102 && source <= CC_119){    //does the parameter selected support range mapping?
        ret = range->rangeStart + (slope * VMSW_getKnownValue(source, outputId, voiceId)) / 127;
        
    //any other parameter don't need custom scaling. We can just return them right away
    }else{
        return VMSW_getKnownValue(source, outputId, voiceId);
    }
    
    //is the parameter we are trying to read variable? If not we can just return the parameter itself. Otherwise assign the RangeParameters variable
    switch(param){
        case 1:
            if(source == freqCurrent){
                if(block->type == VMS_EXP || block->type == VMS_EXP_INV){
                    return ret;
                }else if(block->type == VMS_SIN){
                    return ret * 2;
                }else{
                    return ret * 1000;
                }

            }else if(source >= circ1 && source <= circ4){
                if(block->type == VMS_EXP || block->type == VMS_EXP_INV){
                    return ret;
                }else if(block->type == VMS_SIN){
                    return ret * 2;
                }else{
                    return ret * 1000;
                }

            }else if(source >= CC_102 && source <= CC_119){
                if(block->type == VMS_EXP || block->type == VMS_EXP_INV){
                    return ret;
                }else if(block->type == VMS_SIN){
                    return ret * 2;
                }else{
                    return ret * 1000;
                }
            }
            break;
            
        case 2:
        case 4:
            return ret * 1000;
            
        case 3:
            return ret;
    }
    return 0;
}

static DIRECTION VMS_getThresholdDirection(const VMS_Block_t * block, int32_t param1){
    //get the direction in which the threshold needs to be checked depending on the type of block, and the parameter 1
    switch(block->type){
        case VMS_EXP:
            if(param1 > 1000){
                return RISING;
            }else{
                return FALLING;
            }
            break;
        case VMS_EXP_INV:
            if(param1 > 1000){
                return FALLING;
            }else{
                return RISING;
            }
            break;
        case VMS_LIN:
            if(param1 > 0){
                return RISING;
            }else{
                return FALLING;
            }
            break;
        case VMS_SIN:
            return NONE;
            break;
        default:
            return 0;
    }
}

//convert a block to or from the old format, needed for communication with the legacy software :/
void VMS_convertToLegacyBlock(VMS_LEGAYBLOCK_t * dst, VMS_Block_t * src){
    //this needs to be set by the calling function as the index of the block that this is coming from
    dst->uid = 0;   
    
    //set the indices of the next blocks. This would have been a pointer whenever the block was in flash, but unlinked to be an index for transfer to the config software, thats why we can just use the index we use now directly
    for(uint32_t i = 0; i < VMS_MAX_BRANCHES; i ++){
        //is the block a sustain block? If so we need to do some magic number foo... yay -.-
        if(src->flags & VMS_FLAG_ISBLOCKPERSISTENT){
            //set all nextBlocks to VMS_DIE (a magic number), which used to indicate a sustain block
            dst->nextBlocks[i] = VMS_DIE;
        }else{
            if(src->nextBlocks[i] == 0){
                dst->nextBlocks[i] = src->nextBlocks[i];
            }else if(src->nextBlocks[i] == VMS_BLOCKID_INVALID){
                dst->nextBlocks[i] = VMS_BLOCKID32_INVALID;
            }else{
                dst->nextBlocks[i] = src->nextBlocks[i]+1;
            }
        }
    }
    
    //same with the offBlock pointer
    if(src->offBlock == 0){
        dst->offBlock = src->offBlock;
    }else if(src->offBlock == VMS_BLOCKID_INVALID){
        dst->offBlock = VMS_BLOCKID32_INVALID;
    }else{
        dst->offBlock = src->offBlock+1;
    }
    
    //these stayed the same, though some data widths are different so no memcpy
    dst->behavior = src->behavior;
    dst->type = src->type;
    dst->target = src->target;
    
    dst->targetFactor = src->targetFactor;
    dst->param1 = src->param1;
    dst->param2 = src->param2;
    dst->param3 = src->param3;
    
    //TODO maybe mask out the newly added flags to not confuse the old software?
    dst->flags = src->flags;
    
    //this was a legacy option even in the old software... should not be necessary anymore
    dst->thresholdDirection = NONE;
    
    //convert to the new time scale
    dst->period = src->periodMs * 1000;
    
}

/*void VMS_convertFromLegacyBlock(VMS_Block_t * dst, VMS_LEGAYBLOCK_t * src){
    VMS_convertFromLegacyBlockWith63Bytes(dst, src);
    
    //copy the remaining byte into the flags, but make sure we don't overwrite any of our newly added ones
    //TODO verify this idea... maybe we don't even need this. This function is dead code anyway btw :/
    dst->flags &= 0x7fffffff;
    dst->flags |= src->flags & 0x7fffffff;
}*/

void VMS_convertFromLegacyBlockWith63Bytes(VMS_Block_t * dst, VMS_LEGAYBLOCK_t * src){
    uint32_t isSustainBlock = 0;
    
    //set the indices of the next blocks. This would have been a pointer whenever the block was in flash, but unlinked to be an index for transfer to the config software, thats why we can just use the index we use now directly
    for(uint32_t i = 0; i < VMS_MAX_BRANCHES; i ++){
        //did we already find one VMS DIE ID?
        if(isSustainBlock){
            //yes, just write the nextBlock ID as invalid and go on
            dst->nextBlocks[i] = VMS_BLOCKID_INVALID;
            continue;
        }
        
        //check for the magic number for sustain blocks
        if(src->nextBlocks[i] == VMS_DIE){
            isSustainBlock = 1;
            dst->nextBlocks[i] = VMS_BLOCKID_INVALID;
            continue;
        }
        
        if(src->nextBlocks[i] == 0 || src->nextBlocks[i] == VMS_BLOCKID32_INVALID){
            dst->nextBlocks[i] = VMS_BLOCKID_INVALID;
        }else{
            dst->nextBlocks[i] = src->nextBlocks[i]-1;
        }
    }
    
    //same with the offBlock pointer
    if(src->offBlock == 0 || src->offBlock == VMS_BLOCKID_INVALID){
        dst->offBlock = src->offBlock & 0xffff;
    }else{
        dst->offBlock = src->offBlock-1;
    }
    
    //these stayed the same, though some data widths are different so no memcpy
    dst->behavior = src->behavior;
    dst->type = src->type;
    dst->target = src->target;
    
    dst->targetFactor = src->targetFactor;
    dst->param1 = src->param1;
    dst->param2 = src->param2;
    dst->param3 = src->param3;
    
    //here is the workaround. We convert the uint32_t flags field to an array of four uint8_t, then only access the first three of that
    uint8_t * flagsBytes = (uint8_t *)&src->flags;
    dst->flags = flagsBytes[0] | (flagsBytes[1] << 8) | (flagsBytes[2] << 16);
    
    //did we get a sustain block?
    if(isSustainBlock){
        //yes, that means that the block is a sustain block. Set the flag to reflect this
        dst->flags |= VMS_FLAG_ISBLOCKPERSISTENT;
    }
    
    //convert to the new time scale and
    dst->periodMs = src->period / 1000;
}