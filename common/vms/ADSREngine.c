/*
    Copyright (C) 2020,2021 TMax-Electronics.de
   
    This file is part of the MidiStick Firmware.

    the MidiStick Firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    the MidiStick Firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the MidiStick Firmware.  If not, see <https://www.gnu.org/licenses/>.
*/
    
#if PIC32
#include <xc.h>
#endif
#include "FreeRTOS.h"

#include <stdint.h>
#include <math.h>
#include <stdlib.h>
//#include <sys/attribs.h>
#include <string.h>
//#include <GenericTypeDefs.h>

//#include "usb lib/usb.h"
#include "MidiController.h"
//#include "ConfigManager.h"
//#include "UART32.h"
#include "ADSREngine.h"
#include "SignalGenerator.h"
#include "VMSRoutines.h"
#include "helper/vms_types.h"
#include "mapper.h"
#include "DLL.h"
#include "helper/nvm.h"
#include "clock.h"
#include "cli_common.h"
#include "interrupter.h"
//#include "HIDController.h"
#include "TTerm.h"
#include "tasks/tsk_cli.h"

DLLObject * VMS_listHead = 0;
uint8_t * VMSBLOCKS = 0;

//256 value sin lookup table x := [0, pi)
const int32_t SINTABLE[] = {0, 6135, 12270, 18403, 24533, 30660, 36782, 42898, 49008, 55111, 61205, 67290, 73365, 79429, 85480, 91519, 97545, 103555, 109550, 115529, 121490, 127432, 133356, 139259, 145142, 151002, 156840, 162655, 168444, 174209, 179947, 185658, 191341, 196996, 202620, 208214, 213777, 219308, 224805, 230269, 235698, 241091, 246449, 251769, 257051, 262294, 267498, 272662, 277785, 282865, 287904, 292898, 297849, 302755, 307615, 312429, 317196, 321915, 326586, 331207, 335779, 340300, 344770, 349188, 353553, 357865, 362123, 366327, 370475, 374568, 378604, 382583, 386505, 390368, 394173, 397918, 401603, 405228, 408792, 412294, 415734, 419112, 422426, 425677, 428864, 431986, 435043, 438035, 440960, 443819, 446612, 449337, 451994, 454583, 457104, 459556, 461939, 464253, 466496, 468669, 470772, 472803, 474764, 476653, 478470, 480215, 481888, 483488, 485015, 486469, 487851, 489158, 490392, 491552, 492638, 493650, 494588, 495451, 496239, 496953, 497592, 498156, 498645, 499059, 499397, 499661, 499849, 499962, 500000, 499962, 499849, 499661, 499397, 499059, 498645, 498156, 497592, 496953, 496239, 495451, 494588, 493650, 492638, 491552, 490392, 489158, 487851, 486469, 485015, 483488, 481888, 480215, 478470, 476653, 474764, 472803, 470772, 468669, 466496, 464253, 461939, 459556, 457104, 454583, 451994, 449337, 446612, 443819, 440960, 438035, 435043, 431986, 428864, 425677, 422426, 419112, 415734, 412294, 408792, 405228, 401603, 397918, 394173, 390368, 386505, 382583, 378604, 374568, 370475, 366327, 362123, 357865, 353553, 349188, 344770, 340300, 335779, 331207, 326586, 321915, 317196, 312429, 307615, 302755, 297849, 292898, 287904, 282865, 277785, 272662, 267498, 262294, 257051, 251769, 246449, 241091, 235698, 230269, 224805, 219308, 213777, 208214, 202620, 196996, 191341, 185658, 179947, 174209, 168444, 162655, 156840, 151002, 145142, 139259, 133356, 127432, 121490, 115529, 109550, 103555, 97545, 91519, 85480, 79429, 73365, 67290, 61205, 55111, 49008, 42898, 36782, 30660, 24533, 18403, 12270, 6135};

void * lastData = 0;

void VMS_init(){
    
    //if an erase has failed previously we'd have a half empty list which would mess stuff up. If we find one we erase everything (might be very slow...)

    VMS_BLOCK * cb = &(((VMS_BLOCK*) NVM_blockMem)[0]);
    if(cb->uid == 0xffffffff || cb->uid == 0){
        uint32_t currIndex = 1;
        for(;currIndex < BLOCKMEM_SIZE / sizeof(VMS_BLOCK); currIndex++){
            cb = &(((VMS_BLOCK*) NVM_blockMem)[currIndex]);
            if(cb->uid != 0xffffffff && cb->uid != 0){
                Midi_setEnabled(0);
                break;
            }
        }
    }

    
    VMS_listHead = createNewDll();
}

void VMS_run(){
    if(VMS_listHead == 0) return;
    if(VMS_listHead->next == VMS_listHead) return;
    
    DLLObject * currObject = VMS_listHead->next;
    
    while(currObject != VMS_listHead){
        VMS_listDataObject * currBlock = currObject->data;
        lastData = currObject;
        currObject = currObject->next;
    
        if(SYS_getTime() > currBlock->nextRunTime){
            if(VMS_calculateValue(currBlock)){
                currBlock->nextRunTime = SYS_getTime() + currBlock->period;
            }
        }
    }
}

inline uint32_t SYS_timeSince(uint32_t start){  //normal us  --> schnellere Zeitbasis
    return ((h_time >> 22) - start);
}

inline uint32_t SYS_getTime(){
    return (h_time >> 22);
}

void VMS_resetVoice(SynthVoice * voice, VMS_BLOCK * startBlock){
    //if(!(startBlock > 0x9d000000 && startBlock < 0xa0010000)) return;  //ptr is valid?
    if(startBlock->uid == 0xffffffff || startBlock->uid == 0) return;  //macht das sinn?
    //if(voice->id > 0) __builtin_software_breakpoint();
    DLLObject * currObject = VMS_listHead->next;
    VMS_listDataObject * data;
    //UART_sendString("reset voice", 1);
    
    while(currObject != VMS_listHead){
        data = currObject->data;
        if(data->targetVoice == voice){
            //UART_print("removed block 0x%08x at 0x%08x\r\n", data->block, data);
            
            switch(data->block->type){
                case VMS_SIN:
                    vPortFree(data->data);
                    break;
                default:
                    break;
            }

            vPortFree(data);
            currObject = currObject->next;
            DLL_remove(currObject->prev);
        }else{
            currObject = currObject->next;
        }
    }
    
    VMS_addBlockToList(startBlock, voice);
}

int32_t VMS_getKnownValue(KNOWN_VALUE ID, SynthVoice * voice){
    //UART_print("get %d\r\n", ID);
    switch(ID){
        case maxOnTime:
            return  (MAX_VOL>>12); //Midi_currCoil->maxOnTime;
        case minOnTime:
            return (MIN_VOL>>12);
        case otCurrent:
            return voice->otCurrent;
        case otTarget:
            return ((voice->otTarget * channelData[voice->currNoteOrigin].currVolume * channelData[voice->currNoteOrigin].currStereoVolume) / 32385);
        case otFactor:
            return voice->otFactor;
        case freqCurrent:
            return voice->freqCurrent;
        case freqTarget:
            return voice->freqTarget;
        case freqFactor:
            return voice->freqFactor;
        case pTime:
            return voice->portamentoParam;
            
        case circ1:
            return voice->circ1;
        case circ2:
            return voice->circ2;
        case circ3:
            return voice->circ3;
        case circ4:
            return voice->circ4;
            
        case HyperVoice_Count:
            return voice->hyperVoiceCount;
        case HyperVoice_Phase:
            return voice->hyperVoicePhaseFrac;
            
        case CC_102 ... CC_119:
            return channelData[voice->currNoteOrigin].parameters[ID - CC_102];
        default:
                    break;
    }
    return 0;
}

void VMS_setKnownValue(KNOWN_VALUE ID, int32_t value, SynthVoice * voice){
    switch(ID){
        //all other values can't be written by VMS
        default:
            return;
            
        case otCurrent:
            voice->otCurrent = value;
            break;
        case onTime:
            if(value == 0){
                voice->otFactor = 0;
                voice->otCurrent = 0;
            }else{
                voice->otFactor = value;
                voice->otCurrent = (voice->otTarget * value) / 1000000 + (MIN_VOL>>12);
                //UART_print("OT is now %d", Midi_voice[0].otCurrent);
            }
            SigGen_limit();
            break;
        case freqCurrent:
            voice->freqCurrent = value;
            break;
        case frequency:
            voice->freqFactor = value;
            //UART_print("factor target = %d;  ", value);
            value >>= 4;                                                //we need to pre scale the factor to prevent 32bit integer overflow
            //UART_print("factor target >>= %d;  ", value);
            voice->freqCurrent = (voice->freqTarget * value) / 62500;  //62500 is 1000000 >> 4
            //UART_print("value = %d;  \r\n", voice->freqCurrent);       //we need to pre scale the factor to prevent 32bit integer overflow
            
            SigGen_setNoteTPR(voice->id, voice->freqCurrent);
            break;
        case noise:
            voice->noiseFactor = value;
            voice->noiseCurrent = (voice->noiseTarget * value) / 1000000;
            SigGen_limit();
            break;
            
        case circ1:
            voice->circ1 = value;
            break;
        case circ2:
            voice->circ2 = value;
            break;
        case circ3:
            voice->circ3 = value;
            break;
        case circ4:
            voice->circ4 = value;
            break;

        case HyperVoice_Count:
            if(value == 1000000) voice->hyperVoiceCount = 1;
            if(value == 2000000) voice->hyperVoiceCount = 2;
            //UART_print("HPV count now %d\r\n", voice->hyperVoiceCount);
            SigGen_setNoteTPR(voice->id, voice->freqCurrent);
            break;
        case HyperVoice_Phase:
            voice->hyperVoicePhaseFrac = value;
            voice->hyperVoicePhase = value / 977;  //map to 0 - 0xff
            SigGen_setNoteTPR(voice->id, voice->freqCurrent);
            break;
    }
}

int32_t VMS_getParam(VMS_BLOCK * block, SynthVoice * voice, uint8_t param){
    switch(param){
        case 1:
            return (block->param1 > 0 && block->param1 < KNOWNVAL_MAX) ? VMS_getKnownValue(block->param1, voice) : block->param1;
        case 2:
            return (block->param2 > 0 && block->param2 < KNOWNVAL_MAX) ? VMS_getKnownValue(block->param2, voice) : block->param2;
    }
    return 0;
}

int32_t VMS_getCurrentFactor(KNOWN_VALUE ID, SynthVoice * voice){
    switch(ID){
        default:
            return 0;
        case onTime:
            return voice->otFactor;
        case frequency:
            return voice->freqFactor;
        case noise:
            return voice->noiseFactor;
            
        case circ1:
            return voice->circ1;
        case circ2:
            return voice->circ2;
        case circ3:
            return voice->circ3;
        case circ4:
            return voice->circ4;
            
        case HyperVoice_Count:
            return (HyperVoice_Count == 2) ? 2000000 : 1000000;
        case HyperVoice_Phase:
            return voice->hyperVoicePhaseFrac;
    }
    return 0;
}

unsigned VMS_hasReachedThreshold(VMS_BLOCK * block, int32_t factor, int32_t targetFactor, int32_t param1){
    if(block->type == VMS_JUMP) return 1;
    
    switch(VMS_getThresholdDirection(block, param1)){
        case RISING:
            return factor >= targetFactor;
        case FALLING:
            return factor <= targetFactor;
        case NONE:
            return 0;
        default:
            return 0;
            break;
    }
}

void VMS_removeBlockFromList(VMS_listDataObject * target){
    DLLObject * currObject = VMS_listHead->next;
    if(currObject == VMS_listHead) return;
    
    VMS_listDataObject * data = currObject->data;
    
    while(data != target){
        currObject = currObject->next;
        if(currObject == VMS_listHead) return;
        data = currObject->data;
    }
    
    switch(target->block->type){
        case VMS_SIN:
            //UART_print("DS (0x%08x) ", target->data);
            vPortFree(target->data);
            break;
        default:
            break;
    }
    vPortFree(data);
    DLL_remove(currObject);
}

void VMS_addBlockToList(VMS_BLOCK * block, SynthVoice * voice){
    if(block == 0) return;
    
    VMS_listDataObject * data = pvPortMalloc(sizeof(VMS_listDataObject));
    memset(data, 0, sizeof(VMS_listDataObject));
    
    data->block = block;
    data->targetVoice = voice;
    data->period = block->period;
    data->nextRunTime = SYS_getTime();

    switch(block->type){
        case VMS_SIN:
            data->data = pvPortMalloc(sizeof(SINE_DATA));
            memset(data->data, 0, sizeof(SINE_DATA));
            break;
        default:
            break;
    }
    DLL_add(data, VMS_listHead);
}

void VMS_nextBlock(VMS_listDataObject * data, unsigned blockSet){
    SynthVoice * voice = data->targetVoice;
    VMS_BLOCK * block = data->block;
    
    if(blockSet){
        uint8_t c = 0;
        for(; c < VMS_MAX_BRANCHES; c++){
            if(block->nextBlocks[c] == VMS_DIE){
                return;
            }
            if(ptr_is_in_ram(block->nextBlocks[c]) || ptr_is_in_flash(block->nextBlocks[c])) VMS_addBlockToList(block->nextBlocks[c], voice);
        }
    }else{
        if(block->offBlock != 0){
            VMS_addBlockToList(block->offBlock, voice);
        }
    }
    VMS_removeBlockFromList(data);
}

void VMS_clear(){
    if(VMS_listHead == 0) return;
    DLLObject * currObject = VMS_listHead->next;
    VMS_listDataObject * data;
    
    while(currObject != VMS_listHead){
        data = currObject->data;
        if(ptr_is_freeable(data)){  //Free when is in RAM
            switch(data->block->type){
                case VMS_SIN:
                    vPortFree(data->data);
                    break;
                default:
                    break;
            }

            vPortFree(data);
        }
        currObject = currObject->next;
        DLL_remove(currObject->prev);
    }
}

unsigned VMS_calculateValue(VMS_listDataObject * data){
    lastData = data;
    SynthVoice * voice = data->targetVoice;
    VMS_BLOCK * block = data->block;
    
    if(block->behavior == NORMAL){
        if(!channelData[voice->currNoteOrigin].sustainPedal && !voice->on){
            VMS_nextBlock(data, 0);
            return 0;
        }
    }else{
        if(voice->on){
            VMS_nextBlock(data, 0);
            return 0;
        }
    }
    
    int32_t param1 = VMS_getParameter(1, block, voice);
    int32_t param2 = VMS_getParameter(2, block, voice);
    int32_t param3 = VMS_getParameter(3, block, voice);
    int32_t targetFactor = VMS_getParameter(4, block, voice);
    
    //if(block->flags & VMS_FLAG_ISVARIABLE_PARAM1) UART_print("param1 = %d\r\n", param1);
    
    int32_t currFactor = VMS_getCurrentFactor(block->target, voice);
    
    SINE_DATA * d = (SINE_DATA *) data->data;
    int32_t currFactorDiff;
    
    switch(block->type){
        case VMS_EXP:
            if(param1 > 1000 && currFactor < 1000){
                currFactor = 1000;
            }
            currFactor = (currFactor * param1) / 1000;
            
            if(param1 < 1000 && currFactor < 1000){
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
            //if(block->target == frequency) UART_print("frequency lin: factor = %d, param1 = %d\r\n", currFactor, param1);
            break;
        case VMS_SIN:
            d->currCount += param3;
            if(d->currCount > 0xff) d->currCount = -0xff;
            currFactor = ((param1 * qSin(d->currCount)) / 1000);
            currFactor += param2;
            if(currFactor < 0) currFactor = 0;
            break;
        case VMS_JUMP:
            currFactor = targetFactor;
            break;
    }
    
    if(VMS_hasReachedThreshold(block, currFactor, targetFactor, param1)){
        currFactor = targetFactor;
        VMS_nextBlock(data, 1);
        VMS_setKnownValue(block->target, currFactor, voice);
        return 0;
    }
    
    VMS_setKnownValue(block->target, currFactor, voice);
    return 1;
}

int32_t VMS_getParameter(uint8_t param, VMS_BLOCK * block, SynthVoice * voice){
    switch(param){
    case 1:
        if(block->flags & VMS_FLAG_ISVARIABLE_PARAM1){
            KNOWN_VALUE source = block->param1 & 0xff;
            if(source == freqCurrent){    //does the parameter selected support range mapping?
                RangeParameters * range = (RangeParameters*)&block->param1;
                int32_t slope = range->rangeEnd - range->rangeStart;
                int32_t ret = range->rangeStart + (slope * VMS_getKnownValue(freqCurrent, voice)) / 20000;
                if(block->type == VMS_EXP || block->type == VMS_EXP_INV){
                    return ret;
                }else if(block->type == VMS_SIN){
                    return ret * 2;
                }else{
                    return ret * 1000;
                }
            }else if(source >= circ1 && source <= circ4){    //does the parameter selected support range mapping?
                RangeParameters * range = (RangeParameters*)&block->param1;
                int32_t slope = range->rangeEnd - range->rangeStart;
                int32_t ret = range->rangeStart + (slope * (VMS_getKnownValue(source, voice) >> 4)) / 62500;
                if(block->type == VMS_EXP || block->type == VMS_EXP_INV){
                    return ret;
                }else if(block->type == VMS_SIN){
                    return ret * 2;
                }else{
                    return ret * 1000;
                }
            }else if(source >= CC_102 && source <= CC_119){    //does the parameter selected support range mapping?
                RangeParameters * range = (RangeParameters*)&block->param1;
                int32_t slope = range->rangeEnd - range->rangeStart;
                int32_t ret = range->rangeStart + (slope * VMS_getKnownValue(source, voice)) / 127;
                if(block->type == VMS_EXP || block->type == VMS_EXP_INV){
                    return ret;
                }else if(block->type == VMS_SIN){
                    return ret * 2;
                }else{
                    return ret * 1000;
                }
            }else{
                int32_t kv = VMS_getKnownValue(source, voice);
                return kv;
            }
        }else{
            return block->param1;
        }
        break;
        
    case 2:
        if(block->flags & VMS_FLAG_ISVARIABLE_PARAM2){
            KNOWN_VALUE source = block->param2 & 0xff;
            if(source == freqCurrent){    //does the parameter selected support range mapping?
                RangeParameters * range = (RangeParameters*)&block->param2;
                int32_t slope = range->rangeEnd - range->rangeStart;
                int32_t ret = range->rangeStart + (slope * VMS_getKnownValue(freqCurrent, voice)) / 20000;
                return ret * 1000;
            }else if(source >= circ1 && source <= circ4){    //does the parameter selected support range mapping?
                RangeParameters * range = (RangeParameters*)&block->param2;
                int32_t slope = range->rangeEnd - range->rangeStart;
                int32_t ret = range->rangeStart + (slope * (VMS_getKnownValue(source, voice) >> 4)) / 625;
                return ret * 10;
            }else if(source >= CC_102 && source <= CC_119){    //does the parameter selected support range mapping?
                RangeParameters * range = (RangeParameters*)&block->param2;
                int32_t slope = range->rangeEnd - range->rangeStart;
                int32_t ret = range->rangeStart + (slope * VMS_getKnownValue(source, voice)) / 127;
                return ret * 1000;
            }else{
                return VMS_getKnownValue(source & 0xff, voice);
            }
        }else{
            return block->param2;
        }
        break;
        
        case 3:
        if(block->flags & VMS_FLAG_ISVARIABLE_PARAM3){
            KNOWN_VALUE source = block->param3 & 0xff;
            if(source == freqCurrent){    //does the parameter selected support range mapping?
                RangeParameters * range = (RangeParameters*)&block->param3;
                int32_t slope = range->rangeEnd - range->rangeStart;
                int32_t ret = range->rangeStart + (slope * VMS_getKnownValue(freqCurrent, voice)) / 20000;
                return ret;
            }else if(source >= circ1 && source <= circ4){    //does the parameter selected support range mapping?
                RangeParameters * range = (RangeParameters*)&block->param3;
                int32_t slope = range->rangeEnd - range->rangeStart;
                int32_t ret = range->rangeStart + (slope * (VMS_getKnownValue(source, voice) >> 4)) / 62500;
                return ret;
            }else if(source >= CC_102 && source <= CC_119){    //does the parameter selected support range mapping?
                RangeParameters * range = (RangeParameters*)&block->param3;
                int32_t slope = range->rangeEnd - range->rangeStart;
                int32_t ret = range->rangeStart + (slope * VMS_getKnownValue(source, voice)) / 127;
                return ret;
            }else{
                return VMS_getKnownValue(source & 0xff, voice);
            }
        }else{
            return block->param3;
        }
        break;
        
        
    case 4: //target value
        if(block->flags & VMS_FLAG_ISVARIABLE_TARGETFACTOR){
            KNOWN_VALUE source = block->targetFactor & 0xff;
            if(source == freqCurrent){    //does the parameter selected support range mapping?
                RangeParameters * range = (RangeParameters*)&block->targetFactor;
                int32_t slope = range->rangeEnd - range->rangeStart;
                int32_t ret = range->rangeStart + (slope * VMS_getKnownValue(freqCurrent, voice)) / 20000;
                return ret * 1000;
            }else if(source >= circ1 && source <= circ4){    //does the parameter selected support range mapping?
                RangeParameters * range = (RangeParameters*)&block->targetFactor;
                int32_t slope = range->rangeEnd - range->rangeStart;
                int32_t ret = range->rangeStart + (slope * (VMS_getKnownValue(source, voice) >> 4)) / 62500;
                return ret * 10;
            }else if(source >= CC_102 && source <= CC_119){    //does the parameter selected support range mapping?
                RangeParameters * range = (RangeParameters*)&block->targetFactor;
                int32_t slope = range->rangeEnd - range->rangeStart;
                int32_t ret = range->rangeStart + (slope * VMS_getKnownValue(source, voice)) / 127;
                return ret * 1000;
            }else{
                return VMS_getKnownValue(source & 0xff, voice);
            }
        }else{
            return block->targetFactor;
        }
        break;
    }
    return 0;
}

int32_t qSin(int32_t x){
    int32_t xt = abs(x) % 0xff;
    
    if(xt == 0) return 0;
    return (x > 0) ? SINTABLE[xt] : -SINTABLE[xt];
}

DIRECTION VMS_getThresholdDirection(VMS_BLOCK * block, int32_t param1){
    switch(block->type){
        case VMS_EXP:
            if(param1 > 1000){
                return RISING;
            }else{
                return FALLING;
            }
            //UART_print("exp: param = %d => %s\r\n", param1, (data->thresholdDirection == RISING) ? "rising" : "falling");
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
            //UART_print("LIN: param = %d => %s\r\n", param1, (data->thresholdDirection == RISING) ? "rising" : "falling");
            break;
        case VMS_SIN:
            return NONE;
            break;
        default:
            return 0;
    }
}