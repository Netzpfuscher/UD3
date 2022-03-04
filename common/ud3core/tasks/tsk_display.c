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

// This is the interface to the StripLights component (See the General tab in TopDesign.cysch).
// This component is configured for a strip of WS2812 LEDs.  Various data sources can
// be mapped to the LED's to indicate their status.

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
#include "tsk_priority.h"
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

// TODO: I think this data needs to be saved to eeprom?
DISP_ZONE_t DISP_zones;
static bool selectionActive = false;    // 
static uint32_t selectionStage = 0;     // Stage 0=set zones[sectionToSelect].firstLed, stage 1 sets .lastLed
static uint32_t sectionToSelect = 0;    // The zone currently being configured
static uint16_t selectionBackup;        // Cached original data in case user cancels configuration

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
    
    // The enable_display parameter is overloaded to enable the display AND control the 
    // dimming.  It appears that a value of 0 is handled outside this function, and values 
    // of 1 to 6 are handled here.
    Disp_Dim(configuration.enable_display-1);   // Dims the display from 0 (no dimming) to 4 (max dimming).
    Disp_Trigger(1);

	for (;;) {
		vTaskDelay(10 / portTICK_PERIOD_MS);
        if(selectionActive) continue;
        
        // Clear the display and draw each zone of lights.  A zone is a series of consecutive LED's along the strip.
        Disp_MemClear(0);
        for(uint32_t currZone = 0; currZone < DISP_MAX_ZONES; currZone++){
            if(DISP_zones.zone[currZone].src == DISP_SRC_OFF) continue;
            Disp_DrawLine(DISP_zones.zone[currZone].firstLed, 0, DISP_zones.zone[currZone].lastLed, 0, DISP_getZoneColor(DISP_zones.zone[currZone].src));
        }
        Disp_Trigger(1);
	}
}

// Returns the color corresponding to the specified src.  src is one of the DISP_SRC_ values.
uint32_t DISP_getZoneColor(enum DISP_SRC src){
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

// TODO: This is only used in tsk_hwGauge.c to convert a gauge color to a packed RGBA color.  Perhaps
// this and the SMALL_COLOR declaration should be moved to the tsk_hwGauge.* files instead of spreading
// it here?
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

// Used during configuration to draw a single zone at sectionToSelect
static void DISP_drawSel(){
    Disp_MemClear(0);
    Disp_DrawLine(DISP_zones.zone[sectionToSelect].firstLed, 0, DISP_zones.zone[sectionToSelect].lastLed, 0, Disp_WHITE);
    Disp_Trigger(1);
}

// Configures the upper and lower limits (the range of LED's) of the selected zone.
static uint8_t CMD_displaySelect_handleInput(TERMINAL_HANDLE * handle, uint16_t c){
    switch(c){
        case 'q':   // quit (discards changes)
        case 0x03:
            vPortFree(handle->currProgram);
            TERM_removeProgramm(handle);
            selectionActive = false;
            DISP_zones.data[sectionToSelect] = selectionBackup; // Discard changes, restore original
            ttprintf("\r\n\n%sSelection Canceled!%s\r\n\n", TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_RED), TERM_getVT100Code(_VT100_RESET, 0));
            return TERM_CMD_EXIT_SUCCESS;
        
        case '+':   // Increase the first or last LED index
            if(selectionStage == 1){
                if(DISP_zones.zone[sectionToSelect].lastLed < 0x3f) DISP_zones.zone[sectionToSelect].lastLed ++;
                ttprintf("End = %d     \r", DISP_zones.zone[sectionToSelect].lastLed);
            }else if(selectionStage == 0){
                if(DISP_zones.zone[sectionToSelect].firstLed < 0x3f) DISP_zones.zone[sectionToSelect].firstLed ++;
                ttprintf("Start = %d     \r", DISP_zones.zone[sectionToSelect].firstLed);
            }
            DISP_drawSel();            
            return TERM_CMD_CONTINUE;
        
        case '-':   // Decrease the first or last LED index
            if(selectionStage == 1){
                if(DISP_zones.zone[sectionToSelect].lastLed > 0) DISP_zones.zone[sectionToSelect].lastLed --;
                ttprintf("End = %d     \r", DISP_zones.zone[sectionToSelect].lastLed);
            }else if(selectionStage == 0){
                if(DISP_zones.zone[sectionToSelect].firstLed > 0) DISP_zones.zone[sectionToSelect].firstLed --;
                ttprintf("Start = %d     \r", DISP_zones.zone[sectionToSelect].firstLed);
            }
            DISP_drawSel();            
            return TERM_CMD_CONTINUE;
        
        case '0':   // Set the first or last LED index to 0
            if(selectionStage == 1){
                DISP_zones.zone[sectionToSelect].lastLed = 0;
                ttprintf("End = %d     \r", DISP_zones.zone[sectionToSelect].lastLed);
            }else if(selectionStage == 0){
                DISP_zones.zone[sectionToSelect].firstLed = 0;
                ttprintf("Start = %d     \r", DISP_zones.zone[sectionToSelect].firstLed);
            }
            DISP_drawSel();            
            return TERM_CMD_CONTINUE;
        
        case '*':   // Set the first or last LED index to 64 (the max supported for now)
            if(selectionStage == 1){
                DISP_zones.zone[sectionToSelect].lastLed = 0x3f;
                ttprintf("End = %d     \r", DISP_zones.zone[sectionToSelect].lastLed);
            }else if(selectionStage == 0){
                DISP_zones.zone[sectionToSelect].firstLed = 0x3f;
                ttprintf("Start = %d     \r", DISP_zones.zone[sectionToSelect].firstLed);
            }
            DISP_drawSel();        
            return TERM_CMD_CONTINUE;
            
        case '\r':  // Advance to next step.  First step configures the zone min, second step configures the zone max, third saves changes and exits
            if(selectionStage == 1){
                vPortFree(handle->currProgram);
                TERM_removeProgramm(handle);
                selectionActive = false;
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

void DISP_usage(TERMINAL_HANDLE * handle){
    ttprintf("Usage: display assign [number] [src]\r\n");
    ttprintf("       display clear  [number]\r\n");
    ttprintf("       display select [number]\r\n");
    ttprintf("       number = 0-%d\r\n", DISP_MAX_ZONES);
}

// Interactively configures the first and last LED for the zones.
uint8_t CMD_display(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    uint8_t returnCode = TERM_CMD_EXIT_SUCCESS;
    
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        DISP_usage(handle);
        return returnCode;
    }

    // Assign the source of the data (one of the DISP_SRC_ values) to zone[number]
	if(strcmp(args[0], "assign") == 0 && (argCount == 3)){
        uint8_t section = atoi(args[1]);
        uint8_t src = atoi(args[2]);
        if(section < DISP_MAX_ZONES && src <= DISP_SRC_COUNT){
            DISP_zones.zone[section].src = src;
            ttprintf("Assigned section %d to src %d\r\n", section, src);
            return TERM_CMD_EXIT_SUCCESS;
        }
	}
    
    // Clear zone[number] (disconnects all data sources from the zone)
	if(strcmp(args[0], "clear") == 0 && argCount == 2){
        uint8_t section = atoi(args[1]);
        if(section < DISP_MAX_ZONES){
            DISP_zones.zone[section].src = DISP_SRC_OFF;
            ttprintf("cleared assignment of section %d\r\n", section);
            return TERM_CMD_EXIT_SUCCESS;
        }
	}
    
    // Selects a zone for interactive configuration (assignment of the LED range)
	if(strcmp(args[0], "select") == 0 && argCount == 2){
        uint32_t section = atoi(args[1]);
        if(section < DISP_MAX_ZONES){
            TermProgram * prog = pvPortMalloc(sizeof(TermProgram));
            prog->inputHandler = CMD_displaySelect_handleInput;
            TERM_sendVT100Code(handle, _VT100_RESET, 0); TERM_sendVT100Code(handle, _VT100_CURSOR_POS1, 0);
            returnCode = TERM_CMD_EXIT_PROC_STARTED;
            selectionBackup = DISP_zones.data[sectionToSelect]; // In case user cancels calibration
            selectionActive = true;
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

    DISP_usage(handle);
    return TERM_CMD_EXIT_ERROR;
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
