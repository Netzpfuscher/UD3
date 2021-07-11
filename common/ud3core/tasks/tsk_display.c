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

#include "cyapicallbacks.h"
#include <cytypes.h>
#include <stdint.h>
#include <stdlib.h>

#include "tsk_display.h"
#include "tsk_midi.h"

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"


xTaskHandle tsk_display_TaskHandle;
uint8 tsk_display_initVar = 0u;





/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */
#include "cli_common.h"
#include "telemetry.h"
#include "tsk_priority.h"
#include "tsk_cli.h"
#include "tsk_analog.h"
#include "tsk_hwGauge.h"

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */

#define DISPLAY_BUS_STATE   0
#define DISPLAY_SYNTH_STATE 1
#define DISPLAY_SYNTHMON    2

DISP_ZONE_t DISP_zones;
static unsigned selectionActive = 0;
static uint32_t selectionStage = 0;
static uint32_t sectionToSelect = 0;
static uint16_t selectionBackup;

void tsk_display_TaskProc(void *pvParameters) {
	/*
	 * Add and initialize local variables that are allocated on the Task stack
	 * the the section below.
	 */
	/* `#START TASK_VARIABLES` */

	/* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */
    
    Disp_Start();
    Disp_MemClear(0);
    Disp_Dim(configuration.enable_display-1);
    Disp_Trigger(1);

	for (;;) {
		vTaskDelay(10 / portTICK_PERIOD_MS);
        if(selectionActive) continue;
        
        Disp_MemClear(0);
        for(uint32_t currZone = 0; currZone < DISP_MAX_ZONES; currZone++){
            if(DISP_zones.zone[currZone].src == DISP_SRC_OFF) continue;
            Disp_DrawLine(DISP_zones.zone[currZone].firstLed, 0, DISP_zones.zone[currZone].lastLed, 0, DISP_getZoneColor(DISP_zones.zone[currZone].src));
        }
        Disp_Trigger(1);
	}
}

uint32_t DISP_getZoneColor(uint8_t src){
    //TERM_printDebug(min_handle[1], "getting src %d\r\n", src);
    switch(src){
    case DISP_SRC_BUS:
        if(tt.n.bus_status.value == BUS_OFF) return Disp_GREEN;
        return Disp_RED;
        
    case DISP_SRC_SYNTH:
        switch(param.synth){
            case SYNTH_MIDI:
                return Disp_OCEAN;
            case SYNTH_SID:
                return Disp_ORANGE;
            default:
                return 0;
        }
        
    case DISP_SRC_HWG0 ... DISP_SRC_HWG5:
        return HWGauge_getGaugeColor(src - DISP_SRC_HWG0);
        break;
        
    case DISP_SRC_WHITE_STATIC:
        return Disp_WHITE;
        
    default:
        return 0;
    }
}


uint32_t smallColorToColor(SMALL_COLOR c){
    return ((c.blue << 19) & 0xff0000) | ((c.green << 10) & 0xff00) | ((c.red << 3) & 0xff);
}

/* ------------------------------------------------------------------------ */
void tsk_display_Start(void) {
	/*
	 * Insert task global memeory initialization here. Since the OS does not
	 * initialize ANY global memory, execute the initialization here to make
	 * sure that your task data is properly 
	 */
	/* `#START TASK_GLOBAL_INIT` */
    
    

	/* `#END` */

	if (tsk_display_initVar != 1) {

		/*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_display_TaskProc, "Display-Svc", STACK_DISPLAY, NULL, PRIO_DISPLAY, &tsk_display_TaskHandle);
		tsk_display_initVar = 1;
	}
}

static void DISP_drawSel(){
    Disp_MemClear(0);
    Disp_DrawLine(DISP_zones.zone[sectionToSelect].firstLed, 0, DISP_zones.zone[sectionToSelect].lastLed, 0, Disp_WHITE);
    Disp_Trigger(1);
}

static uint8_t CMD_displaySelect_handleInput(TERMINAL_HANDLE * handle, uint16_t c){
    switch(c){
        case 'q':
        case 0x03:
            vPortFree(handle->currProgram);
            TERM_removeProgramm(handle);
            selectionActive = 0;
            DISP_zones.data[sectionToSelect] = selectionBackup;
            ttprintf("\r\n\n%sSelection Canceled!%s\r\n\n", TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_RED), TERM_getVT100Code(_VT100_RESET, 0));
            return TERM_CMD_EXIT_SUCCESS;
        
        case '+':
            if(selectionStage == 1){
                if(DISP_zones.zone[sectionToSelect].lastLed < 0x3f) DISP_zones.zone[sectionToSelect].lastLed ++;
                DISP_drawSel();
                ttprintf("End = %d     \r", DISP_zones.zone[sectionToSelect].lastLed);
            }else if(selectionStage == 0){
                if(DISP_zones.zone[sectionToSelect].firstLed < 0x3f) DISP_zones.zone[sectionToSelect].firstLed ++;
                DISP_drawSel();
                ttprintf("Start = %d     \r", DISP_zones.zone[sectionToSelect].firstLed);
            }
            
            return TERM_CMD_CONTINUE;
        
        case '-':
            if(selectionStage == 1){
                if(DISP_zones.zone[sectionToSelect].lastLed > 0) DISP_zones.zone[sectionToSelect].lastLed --;
                DISP_drawSel();
                ttprintf("End = %d     \r", DISP_zones.zone[sectionToSelect].lastLed);
            }else if(selectionStage == 0){
                if(DISP_zones.zone[sectionToSelect].firstLed > 0) DISP_zones.zone[sectionToSelect].firstLed --;
                DISP_drawSel();
                ttprintf("Start = %d     \r", DISP_zones.zone[sectionToSelect].firstLed);
            }
            
            return TERM_CMD_CONTINUE;
        
        case '0':
            if(selectionStage == 1){
                DISP_zones.zone[sectionToSelect].lastLed = 0;
                DISP_drawSel();
                ttprintf("End = %d     \r", DISP_zones.zone[sectionToSelect].lastLed);
            }else if(selectionStage == 0){
                DISP_zones.zone[sectionToSelect].firstLed = 0;
                DISP_drawSel();
                ttprintf("Start = %d     \r", DISP_zones.zone[sectionToSelect].firstLed);
            }
            
            return TERM_CMD_CONTINUE;
        
        case '*':
            if(selectionStage == 1){
                DISP_zones.zone[sectionToSelect].lastLed = 0x3f;
                DISP_drawSel();
                ttprintf("End = %d     \r", DISP_zones.zone[sectionToSelect].lastLed);
            }else if(selectionStage == 0){
                DISP_zones.zone[sectionToSelect].firstLed = 0x3f;
                DISP_drawSel();
                ttprintf("Start = %d     \r", DISP_zones.zone[sectionToSelect].firstLed);
            }
            
            return TERM_CMD_CONTINUE;
            
        case '\r':
            if(selectionStage == 1){
                vPortFree(handle->currProgram);
                TERM_removeProgramm(handle);
                selectionActive = 0;
                ttprintf("Calibration saved succesfully\r\n");
                //Todo successfully safe calibration
            }else if(selectionStage == 0){
                selectionStage = 1;
                ttprintf("\r\n\nUse '+', '-', '0', '*' to set the gauge to the MAX reading. Press ENTER to continue\r\n");
                ttprintf("End = %d     \r", DISP_zones.zone[sectionToSelect].lastLed);
            }
            return TERM_CMD_CONTINUE;
            
        default:
            return TERM_CMD_CONTINUE;
    }
}

uint8_t CMD_display(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    uint8_t returnCode = TERM_CMD_EXIT_SUCCESS;
    
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf("Usage: display assign [number] [src]\r\n");
        ttprintf("       display clear  [number]\r\n");
        ttprintf("       display select [number]\r\n");
        return returnCode;
    }
	

	if(strcmp(args[0], "assign") == 0 && (argCount == 3)){
        uint8_t section = atoi(args[1]);
        uint8_t src = atoi(args[2]);
        if(section < DISP_MAX_ZONES || src <= 0xf){
            DISP_zones.zone[section].src = src;
            ttprintf("Assigned section %d to src %d\r\n", section, src);
            return TERM_CMD_EXIT_SUCCESS;
        }
	}
    
	if(strcmp(args[0], "clear") == 0 && argCount == 2){
        uint8_t section = atoi(args[1]);
        if(section < DISP_MAX_ZONES){
            DISP_zones.zone[section].src = DISP_SRC_OFF;
            ttprintf("cleared assignment of section %d\r\n", section);
            return TERM_CMD_EXIT_SUCCESS;
        }
	}
    
	if(strcmp(args[0], "select") == 0 && argCount == 2){
        uint32_t section = atoi(args[1]);
        if(section < DISP_MAX_ZONES){
            TermProgram * prog = pvPortMalloc(sizeof(TermProgram));
            prog->inputHandler = CMD_displaySelect_handleInput;
            TERM_sendVT100Code(handle, _VT100_RESET, 0); TERM_sendVT100Code(handle, _VT100_CURSOR_POS1, 0);
            returnCode = TERM_CMD_EXIT_PROC_STARTED;
            selectionBackup = DISP_zones.data[sectionToSelect];
            selectionActive = 1;
            sectionToSelect = section;
            TERM_attachProgramm(handle, prog);
            selectionStage = 0;
            DISP_drawSel();
            ttprintf("\r\n\nCalibrating hardware gauge #%d\r\n", section);
            ttprintf("Use '+', '-', '0', '*' to set the gauge to the MIN reading. Press ENTER to continue\r\n");
            ttprintf("Start = %d     \r", DISP_zones.zone[sectionToSelect].firstLed);
            return returnCode;
        }
	}
    
    ttprintf("Usage: hwGauge assign [number] [name]\r\n");
    ttprintf("       hwGauge clear [number]\r\n");
    ttprintf("       hwGauge calibrate [number]\r\n");
    ttprintf("       number = 0-%d\r\n", NUM_HWGAUGE);
    return TERM_CMD_EXIT_ERROR;
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
