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

#include <stdint.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "DutyCompressor.h"
#include "cli_common.h"
#include "ZCDtoPWM.h"
#include "SignalGenerator.h"
#include "tasks/tsk_cli.h"

#define COMPSTATE_ATTAC 0
#define COMPSTATE_SUSTAIN 1
#define COMPSTATE_RELEASE 2

static int32_t currGain = COMP_UNITYGAIN; //the scale of this value affects speeds, so we need to scale to the max volume later
static uint32_t compressorState = 0; 
static uint32_t compressorSustainCount = 0; 
static uint32_t compressorWaitCount = 0; 

static int8_t dsgfhjklbgb = 0;
static int8_t dsgfhjklbgb2 = 0;

void COMP_compress(){
    taskENTER_CRITICAL();
    if(configuration.compressor_attac == 0){
        currGain = COMP_UNITYGAIN;
        return;
    }
    
    uint32_t currDuty = SigGen_getCurrDuty();
    
    //TODO make sure dutycycle value range is correct between implementations
    if((currDuty*10) > configuration.max_tr_duty){
        compressorState = COMPSTATE_ATTAC;
        if((currGain -= configuration.compressor_attac) < 0) currGain = 0;

    }else{
        switch(compressorState){
            case COMPSTATE_ATTAC:
                compressorSustainCount = 0;
            case COMPSTATE_SUSTAIN:
                //did the dutycycle change?
                if(currDuty < configuration.max_tr_duty){
                    if(compressorSustainCount == configuration.compressor_sustain){
                        compressorState = COMPSTATE_RELEASE;
                        compressorSustainCount = 0;
                    }else{
                        compressorState = COMPSTATE_SUSTAIN;
                        compressorSustainCount ++;
                    }
                }else{
                    compressorSustainCount = 0;
                }
                
                break;

            case COMPSTATE_RELEASE:
                if(currGain != COMP_UNITYGAIN){ 
                    if((currGain += configuration.compressor_release) >= COMP_UNITYGAIN) currGain = COMP_UNITYGAIN;
                }
                break;
        }
    }
    taskEXIT_CRITICAL();
}

static void COMP_task(void * params){
    while(1){
        //TODO maybe make this thread safe? Or at least verify that this will not try to compress right in the middle of some voice values being updated
        COMP_compress();
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void Comp_init(){
    xTaskCreate(COMP_task, "COMP", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
}

inline uint32_t Comp_getGain(){
    return currGain;
}

inline uint32_t Comp_getMaxDutyOffset(){
    dsgfhjklbgb2 = 1;
    return configuration.compressor_maxDutyOffset;
}