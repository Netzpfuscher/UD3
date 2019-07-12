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

#include "tsk_overlay.h"
#include "tsk_fault.h"
#include "alarmevent.h"

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "helper/teslaterm.h"

/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */
#include "cli_common.h"
#include "telemetry.h"
#include "tsk_analog.h"
#include "tsk_uart.h"
#include <project.h>
#include "helper/printf.h"
#include "ZCDtoPWM.h"
#include "cli_basic.h"




/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */
uint8_t chart;

void tsk_overlay_chart_stop(){
    chart=0;
}
void tsk_overlay_chart_start(){
    chart=1;
}

void show_overlay_100ms(port_str *ptr){

    if(ptr->term_mode == PORT_TERM_VT100){
        char buffer[50];
        int ret=0;
    	Term_Save_Cursor(ptr);
    	Term_Disable_Cursor(ptr);

    	uint8_t row_pos = 1;
    	uint8_t col_pos = 90;
    	//Term_Erase_Screen(ptr);
    	Term_Box(row_pos, col_pos, row_pos + 11, col_pos + 25, ptr);
    	Term_Move_Cursor(row_pos + 1, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Bus Voltage:       %4iV", telemetry.bus_v);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 2, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Battery Voltage:   %4iV", telemetry.batt_v);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 3, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Temp 1:          %4i *C", telemetry.temp1);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 4, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Temp 2:          %4i *C", telemetry.temp2);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 5, col_pos + 1, ptr);
        SEND_CONST_STRING("Bus status: ", ptr);

    	switch (telemetry.bus_status) {
    	case BUS_READY:
            SEND_CONST_STRING("       Ready", ptr);
    		break;
    	case BUS_CHARGING:
            SEND_CONST_STRING("    Charging", ptr);
    		break;
    	case BUS_OFF:
            SEND_CONST_STRING("         OFF", ptr);
    		break;
        case BUS_BATT_UV_FLT:
            SEND_CONST_STRING("  Battery UV", ptr);
    		break;
        case BUS_TEMP1_FAULT:
            SEND_CONST_STRING("  Temp fault", ptr);
    		break;
    	}

    	Term_Move_Cursor(row_pos + 6, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Average power:     %4iW", telemetry.avg_power);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 7, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Average Current: %4i.%iA", telemetry.batt_i / 10, telemetry.batt_i % 10);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 8, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Primary Current:   %4iA", telemetry.primary_i);
        send_buffer((uint8_t*)buffer,ret,ptr);
        
        Term_Move_Cursor(row_pos + 9, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "MIDI voices:         %1i/4", telemetry.midi_voices);
        send_buffer((uint8_t*)buffer,ret,ptr);
        
        Term_Move_Cursor(row_pos + 10, col_pos + 1, ptr);
        if(telemetry.fres==-1){
            ret = snprintf(buffer, sizeof(buffer), "Fres:        no feedback");
        }else{
    	    ret = snprintf(buffer, sizeof(buffer), "Fres:          %4i.%ikHz", telemetry.fres / 10, telemetry.fres % 10);
        }
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Restore_Cursor(ptr);
    	Term_Enable_Cursor(ptr);
    
    }else{
        #if GAUGE0_SLOW==0
        send_gauge(0, GAUGE0_VAR, ptr);
        #endif
        #if GAUGE1_SLOW==0
        send_gauge(1, GAUGE1_VAR, ptr);
        #endif
        #if GAUGE2_SLOW==0
        send_gauge(2, GAUGE2_VAR, ptr);
        #endif
        #if GAUGE3_SLOW==0
        send_gauge(3, GAUGE3_VAR, ptr);
        #endif
        #if GAUGE4_SLOW==0
        send_gauge(4, GAUGE4_VAR, ptr);
        #endif
        #if GAUGE5_SLOW==0
        send_gauge(5, GAUGE5_VAR, ptr);
        #endif
        #if GAUGE6_SLOW==0
        send_gauge(6, GAUGE6_VAR, ptr);
        #endif

        if(ptr->term_mode==PORT_TERM_MQTT){
             ALARMS temp;
            if(alarm_pop(&temp)==pdPASS){
                send_event(&temp,ptr);
            }
        }else{
            if(chart){
                send_chart(0, CHART0_VAR, ptr);
                send_chart(1, CHART1_VAR, ptr);
                send_chart(2, CHART2_VAR, ptr);
                send_chart(3, CHART3_VAR, ptr);
            
                send_chart_draw(ptr);
            }
        }
    }
	
}

void show_overlay_400ms(port_str *ptr) {
    if(ptr->term_mode == PORT_TERM_TT){
        #if GAUGE0_SLOW==1
        send_gauge(0, GAUGE0_VAR, ptr);
        #endif
        #if GAUGE1_SLOW==1
        send_gauge(1, GAUGE1_VAR, ptr);
        #endif
        #if GAUGE2_SLOW==1
        send_gauge(2, GAUGE2_VAR, ptr);
        #endif
        #if GAUGE3_SLOW==1
        send_gauge(3, GAUGE3_VAR, ptr);
        #endif
        #if GAUGE4_SLOW==1
        send_gauge(4, GAUGE4_VAR, ptr);
        #endif
        #if GAUGE5_SLOW==1
        send_gauge(5, GAUGE5_VAR, ptr);
        #endif
        #if GAUGE6_SLOW==1
        send_gauge(6, GAUGE6_VAR, ptr);
        #endif
        
        if(ptr->term_mode!=PORT_TERM_MQTT){
            send_status(telemetry.bus_status!=BUS_OFF,
                        tr_running!=0,
                        configuration.ps_scheme!=AC_NO_RELAY_BUS_SCHEME,
                        blocked,
                        ptr);
        }
    }
}



/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * This is the main procedure that comprises the task.  Place the code required
 * to preform the desired function within the merge regions of the task procedure
 * to add functionality to the task.
 */
void tsk_overlay_TaskProc(void *pvParameters) {
	/*
	 * Add and initialize local variables that are allocated on the Task stack
	 * the the section below.
	 */
	/* `#START TASK_VARIABLES` */
    
    uint8_t cnt=0;
    port_str *port = pvParameters;

	/* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */

	/* `#END` */
    switch(port->term_mode){
        case PORT_TERM_TT:
            alarm_push(ALM_PRIO_INFO,warn_task_TT_overlay, port->num);
        break;
        case PORT_TERM_MQTT:
            alarm_push(ALM_PRIO_INFO,warn_task_MQTT_overlay, port->num);
        break;
        case PORT_TERM_VT100:
            alarm_push(ALM_PRIO_INFO,warn_task_VT100_overlay, ALM_NO_VALUE);
        break; 
    }

	    
    for (;;) {
		/* `#START TASK_LOOP_CODE` */
        xSemaphoreTake(port->term_block, portMAX_DELAY);
        show_overlay_100ms(pvParameters);
        if(cnt<3){
            cnt++;
        }else{
            cnt=0;
            show_overlay_400ms(pvParameters);
        }
        xSemaphoreGive(port->term_block);
        
		/* `#END` */
        if(port->term_mode==PORT_TERM_VT100){
		    vTaskDelay(500 / portTICK_PERIOD_MS);
        }else{
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
	}
}
