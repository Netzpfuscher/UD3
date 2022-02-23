/*
 * UD3
 *
 * Copyright (c) 2018 Jens Kerrinnes
 * Copyright (c) 2015 Steve Ward
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

#include <stdint.h>
#include <device.h>
#include "tsk_priority.h"
#include "tsk_cli.h"
#include "cli_common.h"
#include <stdlib.h>
#include "telemetry.h"
#include "alarmevent.h"
#include "helper/PCA9685.h"
#include "tsk_hwGauge.h"


HWGauge hwGauges = {.gauge[0 ... (NUM_HWGAUGE-1)].calMax = 0xfff};
xTaskHandle tsk_hwGauge_TaskHandle;

static unsigned tsk_hwGauge_initVar = 0;

static unsigned calibrationActive = 0;
static uint32_t gaugeToCalibrate = 0;
static uint32_t calibrationStage = 0;
static HWGauge calibrationGaugeBackup;

static PCA9685* pca_ptr = NULL;

static int32_t gaugeValues[NUM_HWGAUGE];

void tsk_hwGauge_init(){
    if (tsk_hwGauge_initVar != 1) {

		/*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_hwGauge_proc, "HWGauge-Svc", STACK_I2C, NULL, PRIO_I2C, &tsk_hwGauge_TaskHandle);
		tsk_hwGauge_initVar = 1;
	}
}

uint8_t callback_hwGauge(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    TERM_printDebug(min_handle[1], "HWGauge cfg callback");
    
    for(uint8_t currGauge = 0; currGauge < NUM_HWGAUGE; currGauge++){
        if(hwGauges.gauge[currGauge].version > HWGAUGE_CURRVERSION) continue; //TODO better do something...
        
        if(hwGauges.gauge[currGauge].version < HWGAUGE_CURRVERSION){
            memset(&hwGauges.gauge[currGauge], 0, sizeof(HWGauge_s));
            hwGauges.gauge[currGauge].calMax = 0xfff;
        }
        
        hwGauges.gauge[currGauge].src = NULL;
        if(hwGauges.gauge[currGauge].tele_hash == 0) continue;
        for(uint8_t w=0;w<N_TELE;w++){
            if(djb_hash(tt.a[w].name) == hwGauges.gauge[currGauge].tele_hash){
                hwGauges.gauge[currGauge].src = &tt.a[w];
                break;
            }
        }
    }
    
    return 1;
}

void tsk_hwGauge_proc(){
    I2C_Start();
    
    pca_ptr = PCA9685_new(0x40);
    if(pca_ptr==NULL) alarm_push(ALM_PRIO_CRITICAL,"PCA9685: Malloc failed", ALM_NO_VALUE);
    
    alarm_push(ALM_PRIO_INFO,"TASK: I2C started", ALM_NO_VALUE);
    
    while(1){
        if(!calibrationActive){
            for(uint32_t currGauge = 0; currGauge < NUM_HWGAUGE; currGauge++){
                gaugeValues[currGauge] = HWGauge_scaleTelemetry(hwGauges.gauge[currGauge].src, hwGauges.gauge[currGauge].scalMin, hwGauges.gauge[currGauge].scalMax, 0);
                HWGauge_setValue(currGauge,gaugeValues[currGauge]);
            }
        }
        
        vTaskDelay(4);//pdMS_TO_TICKS(1000));//
    }
}

uint32_t HWGauge_getGaugeColor(uint8_t id){
    if(id > NUM_HWGAUGE) return Disp_GREEN;
    
    //TERM_printDebug(min_handle[1], "getting color of gauge %d:\r\n", id);
    
    if(hwGauges.gauge[id].colorTransition == 0xffff){
        uint32_t red   = ((hwGauges.gauge[id].colorB.red * gaugeValues[id]) + (hwGauges.gauge[id].colorA.red * (0xfff-gaugeValues[id]))) / 512;
        uint32_t green = ((hwGauges.gauge[id].colorB.green * gaugeValues[id]) + (hwGauges.gauge[id].colorA.green * (0xfff-gaugeValues[id]))) / 1024;
        uint32_t blue  = ((hwGauges.gauge[id].colorB.blue * gaugeValues[id]) + (hwGauges.gauge[id].colorA.blue * (0xfff-gaugeValues[id]))) / 512;
        if(red > 0xff)      red = 0xff;
        if(green > 0xff)    green = 0xff;
        if(blue > 0xff)     blue = 0xff;
        uint32_t ret = ((blue << 16) & 0xff0000) | ((green << 8) & 0xff00) | ((red) & 0xff);
        //TERM_printDebug(min_handle[1], "\t using dynamic transition gives 0x%06x\r\n", ret);
        return ret;
    }else{
        if(gaugeValues[id] > hwGauges.gauge[id].colorTransition){
            uint32_t ret = smallColorToColor(hwGauges.gauge[id].colorB);
            //TERM_printDebug(min_handle[1], "\t using normal transition gives > 0x%06x\r\n", ret);
            return ret;
        }else{
            uint32_t ret = smallColorToColor(hwGauges.gauge[id].colorA);
            //TERM_printDebug(min_handle[1], "\t using normal transition gives < 0x%06x\r\n", ret);
            return ret;
        }
    }
        
    return 0xff000000;    
}

int32_t HWGauge_scaleTelemetry(TELE * src, int32_t cMin, int32_t cMax, unsigned allowNegative){
    if(src == NULL) return 0;
    int32_t teleScaled = ((src->value - (cMin * src->divider)) * 4096) / (int32_t) ((cMax - cMin) * src->divider);
    //TERM_printDebug(min_handle[1], "Calcing %s = %d (at 0x%08x) => %d, start %d, end %d\r\n", src->name, src->value, &(src->value), teleScaled, cMin, cMax);
    if(teleScaled < 0 && !allowNegative) return 0;
    if(teleScaled > 0xfff) teleScaled = 0xfff;
    return teleScaled;
}

void HWGauge_setValue(uint32_t number, int32_t value){
    if(pca_ptr == NULL) return;
    int32_t diff = hwGauges.gauge[number].calMax - hwGauges.gauge[number].calMin;
    if(diff == 0){
        PCA9685_setPWM(pca_ptr,number,value);
        return;
    }
    
    int32_t writeVal = hwGauges.gauge[number].calMin + ((diff * value) >> 12);
    PCA9685_setPWM(pca_ptr,number,writeVal);
}

static uint8_t CMD_hwGaugeCalibrate_handleInput(TERMINAL_HANDLE * handle, uint16_t c){
    switch(c){
        case 'q':
        case 0x03:
            vStreamBufferDelete(handle->currProgram->inputStream);
            vPortFree(handle->currProgram);
            TERM_removeProgramm(handle);
            calibrationActive = 0;
            memcpy(&hwGauges.gauge[gaugeToCalibrate], &calibrationGaugeBackup, sizeof(HWGauge));
            ttprintf("\r\n\n%sCalibration Canceled!%s\r\n\n", TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_RED), TERM_getVT100Code(_VT100_RESET, 0));
            return TERM_CMD_EXIT_SUCCESS;
        
        case '+':
            if(calibrationStage == 1){
                if(hwGauges.gauge[gaugeToCalibrate].calMax < 0xfff) hwGauges.gauge[gaugeToCalibrate].calMax ++;
                HWGauge_setValue(gaugeToCalibrate, 0xfff);
                ttprintf("Maximum = %d     \r", hwGauges.gauge[gaugeToCalibrate].calMax);
            }else if(calibrationStage == 0){
                if(hwGauges.gauge[gaugeToCalibrate].calMin < 0xfff) hwGauges.gauge[gaugeToCalibrate].calMin ++;
                HWGauge_setValue(gaugeToCalibrate, 0);
                ttprintf("Minumum = %d     \r", hwGauges.gauge[gaugeToCalibrate].calMin);
            }
            
            return TERM_CMD_CONTINUE;
        
        case '-':
            if(calibrationStage == 1){
                if(hwGauges.gauge[gaugeToCalibrate].calMax > 0) hwGauges.gauge[gaugeToCalibrate].calMax --;
                HWGauge_setValue(gaugeToCalibrate, 0xfff);
                ttprintf("Maximum = %d     \r", hwGauges.gauge[gaugeToCalibrate].calMax);
            }else if(calibrationStage == 0){
                if(hwGauges.gauge[gaugeToCalibrate].calMin > 0) hwGauges.gauge[gaugeToCalibrate].calMin --;
                HWGauge_setValue(gaugeToCalibrate, 0);
                ttprintf("Minumum = %d     \r", hwGauges.gauge[gaugeToCalibrate].calMin);
            }
            
            return TERM_CMD_CONTINUE;
        
        case '0':
            if(calibrationStage == 1){
                hwGauges.gauge[gaugeToCalibrate].calMax = 0;
                HWGauge_setValue(gaugeToCalibrate, 0xfff);
                ttprintf("Maximum = %d     \r", hwGauges.gauge[gaugeToCalibrate].calMax);
            }else if(calibrationStage == 0){
                hwGauges.gauge[gaugeToCalibrate].calMin = 0;
                HWGauge_setValue(gaugeToCalibrate, 0);
                ttprintf("Minumum = %d     \r", hwGauges.gauge[gaugeToCalibrate].calMin);
            }
            
            return TERM_CMD_CONTINUE;
        
        case '*':
            if(calibrationStage == 1){
                hwGauges.gauge[gaugeToCalibrate].calMax = 0xfff;
                HWGauge_setValue(gaugeToCalibrate, 0xfff);
                ttprintf("Maximum = %d     \r", hwGauges.gauge[gaugeToCalibrate].calMax);
            }else if(calibrationStage == 0){
                hwGauges.gauge[gaugeToCalibrate].calMin = 0xfff;
                HWGauge_setValue(gaugeToCalibrate, 0);
                ttprintf("Minumum = %d     \r", hwGauges.gauge[gaugeToCalibrate].calMin);
            }
            
            return TERM_CMD_CONTINUE;
            
        case '\r':
            if(calibrationStage == 1){
                HWGauge_setValue(gaugeToCalibrate, 0);
                vStreamBufferDelete(handle->currProgram->inputStream);
                vPortFree(handle->currProgram);
                TERM_removeProgramm(handle);
                calibrationActive = 0;
                ttprintf("Calibration saved succesfully\r\n");
                //Todo successfully safe calibration
            }else if(calibrationStage == 0){
                HWGauge_setValue(gaugeToCalibrate, 0xfff);
                calibrationStage = 1;
                ttprintf("\r\n\nUse '+', '-', '0', '*' to set the gauge to the MAX reading. Press ENTER to continue\r\n");
                ttprintf("Maximum = %d     \r", hwGauges.gauge[gaugeToCalibrate].calMax);
            }
            return TERM_CMD_CONTINUE;
            
        default:
            return TERM_CMD_CONTINUE;
    }
}

uint8_t CMD_hwGauge(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    uint8_t returnCode = TERM_CMD_EXIT_SUCCESS;
    
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf("Usage: hwGauge assign     [number] [name] ([range_min] [range_max])\r\n");
        ttprintf("       hwGauge clear      [number]\r\n");
        ttprintf("       hwGauge calibrate  [number]\r\n");
        ttprintf("       hwGauge startColor [number] [r] [g] [b]\r\n");
        ttprintf("       hwGauge endColor   [number] [r] [g] [b]\r\n");
        ttprintf("       hwGauge transition [number] [val]\r\n");
        ttprintf("       number = 0-%d\r\n", NUM_HWGAUGE);
        return returnCode;
    }
	

	if(strcmp(args[0], "assign") == 0 && (argCount == 3 || argCount == 5)){
        uint32_t gauge = atoi(args[1]);
        if(gauge < NUM_HWGAUGE){
            hwGauges.gauge[gauge].src = NULL;
            for(uint8_t w=0;w<N_TELE;w++){
                if(strcmp(args[2],tt.a[w].name)==0){
                    hwGauges.gauge[gauge].src = &tt.a[w];
                    hwGauges.gauge[gauge].tele_hash = djb_hash(args[2]);
                    if(argCount == 5){
                        hwGauges.gauge[gauge].scalMin = atoi(args[3]);
                        hwGauges.gauge[gauge].scalMax = atoi(args[4]);
                    }else{
                        hwGauges.gauge[gauge].scalMin = tt.a[w].min;
                        hwGauges.gauge[gauge].scalMax = tt.a[w].max;
                    }
                    ttprintf("Assigned gauge %d to \"%s\" from %d to %d\r\n", gauge, tt.a[w].name, hwGauges.gauge[gauge].scalMin, hwGauges.gauge[gauge].scalMax);
                    break;
                }
            }
            if(hwGauges.gauge[gauge].src == NULL){
                ttprintf("Telemetry with name \"%s\" could not be found!\r\n", args[2]);
            }
            return returnCode;
        }
	}
    
	if(strcmp(args[0], "clear") == 0 && argCount == 2){
        uint32_t gauge = atoi(args[1]);
        if(gauge < NUM_HWGAUGE){
            hwGauges.gauge[gauge].src = NULL;
            hwGauges.gauge[gauge].tele_hash = 0;
            ttprintf("Cleared assignment of gauge %d\r\n", gauge);
            return returnCode;
        }
	}
    
	if(strcmp(args[0], "startColor") == 0 && argCount == 5){
        uint32_t gauge = atoi(args[1]);
        if(gauge < NUM_HWGAUGE){
            hwGauges.gauge[gauge].colorA.red =      atoi(args[2]);
            hwGauges.gauge[gauge].colorA.green =    atoi(args[3]);
            hwGauges.gauge[gauge].colorA.blue =     atoi(args[4]);
            ttprintf("set start color of gauge %d to (r=%d,g=%d,b=%d)\r\n", gauge, hwGauges.gauge[gauge].colorA.red, hwGauges.gauge[gauge].colorA.green, hwGauges.gauge[gauge].colorA.blue);
            return returnCode;
        }
    }
    
	if(strcmp(args[0], "endColor") == 0 && argCount == 5){
        uint32_t gauge = atoi(args[1]);
        if(gauge < NUM_HWGAUGE){
            hwGauges.gauge[gauge].colorB.red =      atoi(args[2]);
            hwGauges.gauge[gauge].colorB.green =    atoi(args[3]);
            hwGauges.gauge[gauge].colorB.blue =     atoi(args[4]);
            ttprintf("set end color of gauge %d to (r=%d,g=%d,b=%d)\r\n", gauge, hwGauges.gauge[gauge].colorB.red, hwGauges.gauge[gauge].colorB.green, hwGauges.gauge[gauge].colorB.blue);
            return returnCode;
        }
    }
    
	if(strcmp(args[0], "transition") == 0 && argCount == 3){
        uint32_t gauge = atoi(args[1]);
        if(gauge < NUM_HWGAUGE){
            uint32_t transition = atoi(args[2]);
            if(transition > 0xfff) transition = 0xffff;
            hwGauges.gauge[gauge].colorTransition = transition;
            ttprintf("set transition value of gauge %d to %d\r\n", gauge, hwGauges.gauge[gauge].colorTransition);
            return returnCode;
        }
    }
    
	if(strcmp(args[0], "calibrate") == 0 && argCount == 2){
        uint32_t gauge = atoi(args[1]);
        if(gauge < NUM_HWGAUGE){
            TermProgram * prog = pvPortMalloc(sizeof(TermProgram));
            prog->inputHandler = CMD_hwGaugeCalibrate_handleInput;
            TERM_sendVT100Code(handle, _VT100_RESET, 0); TERM_sendVT100Code(handle, _VT100_CURSOR_POS1, 0);
            returnCode = TERM_CMD_EXIT_PROC_STARTED;
            memcpy(&calibrationGaugeBackup, &hwGauges.gauge[gauge], sizeof(HWGauge));
            calibrationActive = 1;
            gaugeToCalibrate = gauge;
            TERM_attachProgramm(handle, prog);
            calibrationStage = 0;
            HWGauge_setValue(gauge, 0);
            ttprintf("\r\n\nCalibrating hardware gauge #%d\r\n", gauge);
            ttprintf("Use '+', '-', '0', '*' to set the gauge to the MIN reading. Press ENTER to continue\r\n");
            ttprintf("Minumum = %d     \r", hwGauges.gauge[gaugeToCalibrate].calMin);
            return returnCode;
        }
	}
    
    ttprintf("Usage: hwGauge assign [number] [name]\r\n");
    ttprintf("       hwGauge clear [number]\r\n");
    ttprintf("       hwGauge calibrate [number]\r\n");
    ttprintf("       number = 0-%d\r\n", NUM_HWGAUGE);
    
    return TERM_CMD_EXIT_ERROR;
}