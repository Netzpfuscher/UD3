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

HWGauge hwGauges[NUM_HWGAUGE] = {[0 ... (NUM_HWGAUGE-1)].calMax = 0xfff};
xTaskHandle tsk_hwGauge_TaskHandle;

static unsigned tsk_hwGauge_initVar = 0;

static unsigned calibrationActive = 0;
static uint32_t gaugeToCalibrate = 0;
static uint32_t calibrationStage = 0;
static HWGauge calibrationGaugeBackup;

static PCA9685* pca_ptr = NULL;

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

void tsk_hwGauge_proc(){
    I2C_Start();
    
    pca_ptr = PCA9685_new(0x40);
    if(pca_ptr==NULL) alarm_push(ALM_PRIO_CRITICAL, "PCA9685: Malloc failed", ALM_NO_VALUE);
    
    alarm_push(ALM_PRIO_INFO, "TASK: I2C started", ALM_NO_VALUE);
    
    while(1){
        if(!calibrationActive){
            for(uint32_t currGauge = 0; currGauge < NUM_HWGAUGE; currGauge++){
                HWGauge_setValue(currGauge,HWGauge_scaleTelemetry(hwGauges[currGauge].src, 0));
            }
        }
        
        vTaskDelay(4);//pdMS_TO_TICKS(1000));//
    }
}

int32_t HWGauge_scaleTelemetry(TELE * src, unsigned allowNegative){
    if(src == NULL) return 0;
    int32_t teleScaled = ((src->value - src->min) * 4096) / (src->max * src->divider);
    //TERM_printDebug(min_handle[1], "Calcing %s = %d (at 0x%08x) => %d\r\n", src->name, src->value, &(src->value), teleScaled);
    if(teleScaled < 0 && !allowNegative) return 0;
    return teleScaled;
}

void HWGauge_setValue(uint32_t number, int32_t value){
    if(pca_ptr == NULL) return;
    int32_t diff = hwGauges[number].calMax - hwGauges[number].calMin;
    if(diff == 0){
        PCA9685_setPWM(pca_ptr,number,value);
        return;
    }
    
    int32_t writeVal = hwGauges[number].calMin + ((diff * value) >> 12);
    PCA9685_setPWM(pca_ptr,number,writeVal);
}

static uint8_t CMD_hwGaugeCalibrate_handleInput(TERMINAL_HANDLE * handle, uint16_t c){
    switch(c){
        case 'q':
        case 0x03:
            vPortFree(handle->currProgram);
            TERM_removeProgramm(handle);
            calibrationActive = 0;
            memcpy(&hwGauges[gaugeToCalibrate], &calibrationGaugeBackup, sizeof(HWGauge));
            ttprintf("\r\n\n%sCalibration Canceled!%s\r\n\n", TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_RED), TERM_getVT100Code(_VT100_RESET, 0));
            return TERM_CMD_EXIT_SUCCESS;
        
        case '+':
            if(calibrationStage == 1){
                if(hwGauges[gaugeToCalibrate].calMax < 0xfff) hwGauges[gaugeToCalibrate].calMax ++;
                HWGauge_setValue(gaugeToCalibrate, 0xfff);
                ttprintf("Maximum = %d     \r", hwGauges[gaugeToCalibrate].calMax);
            }else if(calibrationStage == 0){
                if(hwGauges[gaugeToCalibrate].calMin < 0xfff) hwGauges[gaugeToCalibrate].calMin ++;
                HWGauge_setValue(gaugeToCalibrate, 0);
                ttprintf("Minumum = %d     \r", hwGauges[gaugeToCalibrate].calMin);
            }
            
            return TERM_CMD_CONTINUE;
        
        case '-':
            if(calibrationStage == 1){
                if(hwGauges[gaugeToCalibrate].calMax > 0) hwGauges[gaugeToCalibrate].calMax --;
                HWGauge_setValue(gaugeToCalibrate, 0xfff);
                ttprintf("Maximum = %d     \r", hwGauges[gaugeToCalibrate].calMax);
            }else if(calibrationStage == 0){
                if(hwGauges[gaugeToCalibrate].calMin > 0) hwGauges[gaugeToCalibrate].calMin --;
                HWGauge_setValue(gaugeToCalibrate, 0);
                ttprintf("Minumum = %d     \r", hwGauges[gaugeToCalibrate].calMin);
            }
            
            return TERM_CMD_CONTINUE;
        
        case '0':
            if(calibrationStage == 1){
                hwGauges[gaugeToCalibrate].calMax = 0;
                HWGauge_setValue(gaugeToCalibrate, 0xfff);
                ttprintf("Maximum = %d     \r", hwGauges[gaugeToCalibrate].calMax);
            }else if(calibrationStage == 0){
                hwGauges[gaugeToCalibrate].calMin = 0;
                HWGauge_setValue(gaugeToCalibrate, 0);
                ttprintf("Minumum = %d     \r", hwGauges[gaugeToCalibrate].calMin);
            }
            
            return TERM_CMD_CONTINUE;
        
        case '*':
            if(calibrationStage == 1){
                hwGauges[gaugeToCalibrate].calMax = 0xfff;
                HWGauge_setValue(gaugeToCalibrate, 0xfff);
                ttprintf("Maximum = %d     \r", hwGauges[gaugeToCalibrate].calMax);
            }else if(calibrationStage == 0){
                hwGauges[gaugeToCalibrate].calMin = 0xfff;
                HWGauge_setValue(gaugeToCalibrate, 0);
                ttprintf("Minumum = %d     \r", hwGauges[gaugeToCalibrate].calMin);
            }
            
            return TERM_CMD_CONTINUE;
            
        case '\r':
            if(calibrationStage == 1){
                HWGauge_setValue(gaugeToCalibrate, 0);
                vPortFree(handle->currProgram);
                TERM_removeProgramm(handle);
                calibrationActive = 0;
                ttprintf("Calibration saved succesfully\r\n");
                //Todo successfully safe calibration
            }else if(calibrationStage == 0){
                HWGauge_setValue(gaugeToCalibrate, 0xfff);
                calibrationStage = 1;
                ttprintf("\r\n\nUse '+', '-', '0', '*' to set the gauge to the MAX reading. Press ENTER to continue\r\n");
                ttprintf("Maximum = %d     \r", hwGauges[gaugeToCalibrate].calMax);
            }
            return TERM_CMD_CONTINUE;
            
        default:
            return TERM_CMD_CONTINUE;
    }
}

uint8_t CMD_hwGauge(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    uint8_t returnCode = TERM_CMD_EXIT_SUCCESS;
    
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf("Usage: hwGauge assign [number] [name]\r\n");
        ttprintf("       hwGauge clear [number]\r\n");
        ttprintf("       hwGauge calibrate [number]\r\n");
        ttprintf("       number = 0-%d\r\n", NUM_HWGAUGE);
        return returnCode;
    }
	

	if(strcmp(args[0], "assign") == 0 && argCount == 3){
        uint32_t gauge = atoi(args[1]);
        if(gauge < NUM_HWGAUGE){
            hwGauges[gauge].src = NULL;
            for(uint8_t w=0;w<N_TELE;w++){
                if(strcmp(args[2],tt.a[w].name)==0){
                    hwGauges[gauge].src = &tt.a[w];
                    ttprintf("Assigned gauge %d to \"%s\"\r\n", gauge, tt.a[w].name);
                    break;
                }
            }
            if(hwGauges[gauge].src == NULL){
                ttprintf("Telemetry with name \"%s\" could not be found!\r\n", args[2]);
            }
            return returnCode;
        }
	}
    
	if(strcmp(args[0], "clear") == 0 && argCount == 2){
        uint32_t gauge = atoi(args[1]);
        if(gauge < NUM_HWGAUGE){
            hwGauges[gauge].src = NULL;
            ttprintf("Cleared assignment of gauge %d\r\n", gauge);
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
            memcpy(&calibrationGaugeBackup, &hwGauges[gauge], sizeof(HWGauge));
            calibrationActive = 1;
            gaugeToCalibrate = gauge;
            TERM_attachProgramm(handle, prog);
            calibrationStage = 0;
            HWGauge_setValue(gauge, 0);
            ttprintf("\r\n\nCalibrating hardware gauge #%d\r\n", gauge);
            ttprintf("Use '+', '-', '0', '*' to set the gauge to the MIN reading. Press ENTER to continue\r\n");
            ttprintf("Minumum = %d     \r", hwGauges[gaugeToCalibrate].calMin);
            return returnCode;
        }
	}
    
    ttprintf("Usage: hwGauge assign [number] [name]\r\n");
    ttprintf("       hwGauge clear [number]\r\n");
    ttprintf("       hwGauge calibrate [number]\r\n");
    ttprintf("       number = 0-%d\r\n", NUM_HWGAUGE);
    return TERM_CMD_EXIT_ERROR;
}