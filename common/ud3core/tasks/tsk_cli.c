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
#include <stdarg.h>

#include "tsk_cli.h"
#include "tsk_fault.h"
#include "tsk_overlay.h"
#include "tsk_midi.h"
#include "cli_basic.h"
#include "alarmevent.h"
#include "autotune.h"
#include "qcw.h"
#include "helper/debug.h"
#include "helper/nvm.h"

#include "helper/printf.h"

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "TTerm_AC.h"



xTaskHandle UART_Terminal_TaskHandle;
xTaskHandle USB_Terminal_TaskHandle;
xTaskHandle MIN_Terminal_TaskHandle[NUM_MIN_CON];
uint8 tsk_cli_initVar = 0u;

//port_str serial_port;
port_str usb_port;
port_str min_port[NUM_MIN_CON];
port_str null_port;

TERMINAL_HANDLE * usb_handle;
TERMINAL_HANDLE * min_handle[NUM_MIN_CON];

TERMINAL_HANDLE null;
TERMINAL_HANDLE * null_handle = &null;

static const char * AC_start_stop[] = {
    "start",
    "stop"
};

static const char * AC_on_off[] = {
    "off",
    "on"
    
};

static const char * AC_con[] = {
    "info",
    "min",
    "numcon"
};

static const char * AC_ramp[] = {
    "clear",
    "draw",
    "line",
    "point"
};

static const char * AC_alarms[] = {
    "get",
    "reset",
    "roll"
};

static const char * AC_hwGauge[] = {
    "assign",
    "clear",
    "calibrate",
    "endColor",
    "startColor",
    "transition"
};

static const char * AC_display[] = {
    "assign",
    "blue",
    "bus",
    "clear",
    "cyan",
    "gauge0",
    "gauge1",
    "gauge2",
    "gauge3",
    "gauge4",
    "gauge5",
    "green",
    "magenta",
    "off",
    "red",
    "select",
    "synth",
    "white",
    "yellow"
};

static const char * AC_eeprom[] = {
    "load",
    "save"
};

static const char * AC_tune[] = {
    "prim",
    "sec"
};

static const char * AC_nvm[] = {
    "blocks",
    "maps"
};

static const char * min_names[] = {
    "MIN0",
    "MIN1",
    "MIN2",
    "MIN3",
};


/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */

#include "cli_common.h"

#include "tsk_priority.h"
#include "tsk_usb.h"
#include "tsk_hwGauge.h"
#include <project.h>
#include "tsk_eth_common.h"


/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */





/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * This is the main procedure that comprises the task.  Place the code required
 * to preform the desired function within the merge regions of the task procedure
 * to add functionality to the task.
 */

void tsk_cli_TaskProc(void *pvParameters) {
	/*
	 * Add and initialize local variables that are allocated on the Task stack
	 * the the section below.
	 */
	/* `#START TASK_VARIABLES` */

	TERMINAL_HANDLE * handle = pvParameters;


    /* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
     * in the task.
	 */
	/* `#START TASK_INIT_CODE` */

    switch(portM->type) {
        case PORT_TYPE_SERIAL:
            alarm_push(ALM_PRIO_INFO, "TASK: Serial-CLI started", ALM_NO_VALUE);
        break;
        case PORT_TYPE_USB:
            alarm_push(ALM_PRIO_INFO, "TASK: USB-CLI started", ALM_NO_VALUE);
        break;
        case PORT_TYPE_MIN:
            alarm_push(ALM_PRIO_INFO, "TASK: MIN-CLI started", portM->num);
        break;      
    }

    /* `#END` */
    uint8_t c;
    uint8_t len;
	for (;;) {
		/* `#START TASK_LOOP_CODE` */
            len = xStreamBufferReceive(portM->rx, &c,sizeof(c), portMAX_DELAY);
            if (xSemaphoreTake(portM->term_block, portMAX_DELAY)) {
                TERM_processBuffer(&c,len,handle);
                xSemaphoreGive(portM->term_block);
            }

        /* `#END` */
	}
}



/* ------------------------------------------------------------------------ */
void tsk_cli_Start(void) {
/*
	 * Insert task global memeory initialization here. Since the OS does not
	 * initialize ANY global memory, execute the initialization here to make
	 * sure that your task data is properly 
	 */
/* `#START TASK_GLOBAL_INIT` */

/* `#END` */

	if (tsk_cli_initVar != 1) {

        usb_port.type = PORT_TYPE_USB;
        usb_port.term_mode = PORT_TERM_VT100;
        usb_port.term_send_alarms = false;
        usb_port.term_block = xSemaphoreCreateBinary();
        usb_port.rx = xStreamBufferCreate(STREAMBUFFER_RX_SIZE,1);
        usb_port.tx = xStreamBufferCreate(STREAMBUFFER_TX_SIZE,64);
        xSemaphoreGive(usb_port.term_block);
        
        usb_handle = TERM_createNewHandle(stream_printf,&usb_port,pdTRUE,&TERM_cmdListHead,NULL,"usb");

        TERM_addCommand(CMD_signals, "signals","For debugging",0,&TERM_cmdListHead);
           
        TERM_addCommandConstAC(CMD_tr, "tr", "Transient [start/stop]", AC_start_stop,&TERM_cmdListHead);
        TERM_addCommandConstAC(CMD_con, "con","Prints the connections",AC_con,&TERM_cmdListHead);
        TERM_addCommandConstAC(CMD_alarms, "alarms","Alarms [get/roll/reset]",AC_alarms,&TERM_cmdListHead);
        TERM_addCommand(CMD_SynthMon, "synthmon","Synthesizer status",0,&TERM_cmdListHead);
        TERM_addCommand(CMD_bootloader, "bootloader","Enters the bootloader",0,&TERM_cmdListHead);
        TERM_addCommandConstAC(CMD_bus, "bus","bus [on/off]",AC_on_off,&TERM_cmdListHead);
        TERM_addCommand(CMD_calib, "calib","Calibrate Vdriver",0,&TERM_cmdListHead);
        TERM_addCommand(CMD_config_get, "config_get","Internal use",0,&TERM_cmdListHead);
        TERM_addCommand(CMD_features, "features","Get supported features",0,&TERM_cmdListHead);
        TERM_addCommandConstAC(CMD_eeprom, "eeprom","Save/Load config [load/save]",AC_eeprom,&TERM_cmdListHead);
        TERM_addCommand(CMD_udkill, "kill","Stops the output",0,&TERM_cmdListHead);
        TERM_addCommand(CMD_load_defaults, "load_default","Loads the default parameters",0,&TERM_cmdListHead);

        TERM_addCommandAC(TERM_addCommand(CMD_get, "get", "Usage get [param]", 0, &TERM_cmdListHead), complete_parameter_name, NULL);
        TERM_addCommandAC(TERM_addCommand(CMD_set, "set", "Usage set [param] [value]", 0, &TERM_cmdListHead), complete_parameter_name, NULL);

        TERM_addCommand(CMD_oneshot, "oneshot","oneshot <ontime> <volume>, volume from 0 to 32767",0,&TERM_cmdListHead);
        
        TERM_addCommandConstAC(CMD_nvm, "nvm","NVM-Test Func", AC_nvm,&TERM_cmdListHead); 
        
        TERM_addCommandConstAC(CMD_qcw, "qcw","QCW [start/stop]",AC_start_stop,&TERM_cmdListHead);
        TERM_addCommand(CMD_relay, "relay","Switch user relay 3/4",0,&TERM_cmdListHead);
        TERM_addCommand(CMD_pwm, "pwm","Set user pwm 3/4",0,&TERM_cmdListHead);
        TERM_addCommand(CMD_reset, "reset","Resets UD3",0,&TERM_cmdListHead);
        TERM_addCommand(CMD_status, "status","Displays coil status",0,&TERM_cmdListHead);
        TERM_addCommand(CMD_tterm, "tterm","Changes terminal mode",0,&TERM_cmdListHead);
        TERM_addCommandConstAC(CMD_tune, "tune","Autotune [prim/sec]",AC_tune,&TERM_cmdListHead);
        TERM_addCommand(CMD_telemetry, "telemetry","Telemetry options",0,&TERM_cmdListHead);
        TERM_addCommandConstAC(CMD_ramp, "ramp","Write QCW ramp",AC_ramp,&TERM_cmdListHead);
        TERM_addCommand(CMD_debug, "debug","Debug mode",0,&TERM_cmdListHead);
        TERM_addCommand(CMD_ntc, "ntc","Calibrate NTC iDAC",0,&TERM_cmdListHead);
        TERM_addCommandConstAC(CMD_hwGauge, "hwGauge","adjust hardware gauge parameters",AC_hwGauge,&TERM_cmdListHead);
        TERM_addCommand(CMD_hwrev, "hwrev","Display hardware revision",0,&TERM_cmdListHead);
        TERM_addCommand(CMD_midi_inject, "midi","Inject a MIDI message",0,&TERM_cmdListHead);

        for(uint8_t i=0;i<NUM_MIN_CON;i++){
            min_port[i].type = PORT_TYPE_MIN;
            min_port[i].num = i;
            min_port[i].term_mode = PORT_TERM_VT100;
            min_port[i].term_send_alarms = false;
            min_port[i].term_block = xSemaphoreCreateBinary();
            min_port[i].rx = xStreamBufferCreate(STREAMBUFFER_RX_SIZE,1);
            min_port[i].tx = xStreamBufferCreate(STREAMBUFFER_TX_SIZE,256);
            xSemaphoreGive(min_port[i].term_block);
            
            min_handle[i] = TERM_createNewHandle(stream_printf,&min_port[i],pdTRUE,&TERM_cmdListHead,NULL,min_names[i]);
            
            xTaskCreate(tsk_cli_TaskProc, min_names[i], STACK_TERMINAL, min_handle[i], PRIO_TERMINAL, &MIN_Terminal_TaskHandle[i]);
        }
        

		/*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/

		xTaskCreate(tsk_cli_TaskProc, "USB-CLI", STACK_TERMINAL,  usb_handle, PRIO_TERMINAL, &USB_Terminal_TaskHandle);
		tsk_cli_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
