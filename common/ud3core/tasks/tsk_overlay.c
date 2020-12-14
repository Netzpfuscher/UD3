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
#include <stdlib.h>

#include "tsk_overlay.h"
#include "tsk_fault.h"
#include "alarmevent.h"

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "helper/teslaterm.h"
#include "tasks/tsk_priority.h"
#include "tasks/tsk_cli.h"
#include "tsk_min.h"

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
#include "tsk_cli.h"
#include <project.h>
#include "helper/printf.h"
#include "ZCDtoPWM.h"



/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */
uint8_t chart;

TELEMETRY tt;

void init_telemetry(){
    
    tt.n.i2t_i.name = "Fuse";
    tt.n.i2t_i.value = 0;
    tt.n.i2t_i.min = 0;
    tt.n.i2t_i.max = 100;
    tt.n.i2t_i.offset = 0;
    tt.n.i2t_i.unit = TT_UNIT_PERCENT;
    tt.n.i2t_i.divider = 1;
    tt.n.i2t_i.high_res = pdFALSE;
    tt.n.i2t_i.resend_time = TT_FAST;
    tt.n.i2t_i.chart = TT_NO_TELEMETRY;
    tt.n.i2t_i.gauge = 6;
    
    tt.n.fres.name = "Fres";
    tt.n.fres.value = 0;
    tt.n.fres.min = 0;
    tt.n.fres.max = 500;
    tt.n.fres.offset = 0;
    tt.n.fres.unit = TT_UNIT_Hz;
    tt.n.fres.divider = 1000;
    tt.n.fres.high_res = pdTRUE;
    tt.n.fres.resend_time = TT_SLOW;
    tt.n.fres.chart = TT_NO_TELEMETRY;
    tt.n.fres.gauge = TT_NO_TELEMETRY;
    
    tt.n.duty.name = "Dutycycle";
    tt.n.duty.value = 0;
    tt.n.duty.min = 0;
    tt.n.duty.max = 100;
    tt.n.duty.offset = 0;
    tt.n.duty.unit = TT_UNIT_PERCENT;
    tt.n.duty.divider = 10;
    tt.n.duty.high_res = pdTRUE;
    tt.n.duty.resend_time = TT_FAST;
    tt.n.duty.chart = TT_NO_TELEMETRY;
    tt.n.duty.gauge = TT_NO_TELEMETRY;
    
    tt.n.driver_v.name = "Driver_Voltage";
    tt.n.driver_v.value = 0;
    tt.n.driver_v.min = 0;
    tt.n.driver_v.max = 30;
    tt.n.driver_v.offset = 0;
    tt.n.driver_v.unit = TT_UNIT_V;
    tt.n.driver_v.divider = 1000;
    tt.n.driver_v.high_res = pdTRUE;
    tt.n.driver_v.resend_time = TT_SLOW;
    tt.n.driver_v.chart = TT_NO_TELEMETRY;
    tt.n.driver_v.gauge = TT_NO_TELEMETRY;
    
    tt.n.bus_status.name = "Bus_State";
    tt.n.bus_status.value = 0;
    tt.n.bus_status.min = 0;
    tt.n.bus_status.max = 7;
    tt.n.bus_status.offset = 0;
    tt.n.bus_status.unit = TT_UNIT_NONE;
    tt.n.bus_status.divider = 1;
    tt.n.bus_status.high_res = pdFALSE;
    tt.n.bus_status.resend_time =TT_SLOW;
    tt.n.bus_status.chart = TT_NO_TELEMETRY;
    tt.n.bus_status.gauge = TT_NO_TELEMETRY;
    
    tt.n.batt_v.name = "Voltage2";
    tt.n.batt_v.value = 0;
    tt.n.batt_v.min = 0;
    tt.n.batt_v.offset = 0;
    tt.n.batt_v.unit = TT_UNIT_V;
    tt.n.batt_v.divider = 1;
    tt.n.batt_v.high_res = pdFALSE;
    tt.n.batt_v.resend_time = TT_FAST;
    tt.n.batt_v.chart = TT_NO_TELEMETRY;
    tt.n.batt_v.gauge = TT_NO_TELEMETRY;
    
    tt.n.primary_i.name = "Primary";
    tt.n.primary_i.value = 0;
    tt.n.primary_i.min = 0;
    tt.n.primary_i.offset = 0;
    tt.n.primary_i.unit = TT_UNIT_A;
    tt.n.primary_i.divider = 0;
    tt.n.primary_i.high_res = pdFALSE;
    tt.n.primary_i.resend_time = TT_FAST;
    tt.n.primary_i.chart = 2;
    tt.n.primary_i.gauge = 4;
    
    tt.n.midi_voices.name = "Voices";
    tt.n.midi_voices.value = 0;
    tt.n.midi_voices.min = 0;
    tt.n.midi_voices.max = 4;
    tt.n.midi_voices.offset = 0;
    tt.n.midi_voices.unit = TT_UNIT_NONE;
    tt.n.midi_voices.divider = 1;
    tt.n.midi_voices.high_res = pdFALSE;
    tt.n.midi_voices.resend_time = TT_FAST;
    tt.n.midi_voices.chart = TT_NO_TELEMETRY;
    tt.n.midi_voices.gauge = 5;
    
    tt.n.bus_v.name = "Voltage";
    tt.n.bus_v.value = 0;
    tt.n.bus_v.min = 0;
    tt.n.bus_v.offset = 0;
    tt.n.bus_v.unit = TT_UNIT_V;
    tt.n.bus_v.divider = 1;
    tt.n.bus_v.high_res = pdFALSE;
    tt.n.bus_v.resend_time = TT_FAST;
    tt.n.bus_v.chart = 0;
    tt.n.bus_v.gauge = 0;
    
    tt.n.temp1.name = "Temp1";
    tt.n.temp1.value = 0;
    tt.n.temp1.min = 0;
    tt.n.temp1.offset = 0;
    tt.n.temp1.unit = TT_UNIT_C;
    tt.n.temp1.divider = 1;
    tt.n.temp1.high_res = pdFALSE;
    tt.n.temp1.resend_time = TT_SLOW;
    tt.n.temp1.chart = 1;
    tt.n.temp1.gauge = 1;
    
    tt.n.temp2.name = "Temp2";
    tt.n.temp2.value = 0;
    tt.n.temp2.min = 0;
    tt.n.temp2.offset = 0;
    tt.n.temp2.unit = TT_UNIT_C;
    tt.n.temp2.divider = 1;
    tt.n.temp2.high_res = pdFALSE;
    tt.n.temp2.resend_time = TT_SLOW;
    tt.n.temp2.chart = TT_NO_TELEMETRY;
    tt.n.temp2.gauge = TT_NO_TELEMETRY;
       
    tt.n.batt_i.name = "Current";
    tt.n.batt_i.value = 0;
    tt.n.batt_i.min = 0;
    tt.n.batt_i.offset = 0;
    tt.n.batt_i.unit = TT_UNIT_A;
    tt.n.batt_i.divider = 10;
    tt.n.batt_i.high_res = pdTRUE;
    tt.n.batt_i.resend_time = TT_FAST;
    tt.n.batt_i.chart = TT_NO_TELEMETRY;
    tt.n.batt_i.gauge = 3;
        
    tt.n.avg_power.name = "Power";
    tt.n.avg_power.value = 0;
    tt.n.avg_power.min = 0;
    tt.n.avg_power.offset = 0;
    tt.n.avg_power.unit = TT_UNIT_kW;
    tt.n.avg_power.divider = 1000;
    tt.n.avg_power.high_res = pdTRUE;
    tt.n.avg_power.resend_time = TT_FAST;
    tt.n.avg_power.chart = 3;
    tt.n.avg_power.gauge = 2;
    
    tt.n.tx_datarate.name = "TX_Datarate";
    tt.n.tx_datarate.value = 0;
    tt.n.tx_datarate.min = 0;
    tt.n.tx_datarate.max = 57600;
    tt.n.tx_datarate.offset = 0;
    tt.n.tx_datarate.unit = TT_UNIT_PERCENT;
    tt.n.tx_datarate.divider = 1;
    tt.n.tx_datarate.high_res = pdTRUE;
    tt.n.tx_datarate.resend_time = TT_FAST;
    tt.n.tx_datarate.chart = TT_NO_TELEMETRY;
    tt.n.tx_datarate.gauge = TT_NO_TELEMETRY;
    
    tt.n.rx_datarate.name = "RX_Datarate";
    tt.n.rx_datarate.value = 0;
    tt.n.rx_datarate.min = 0;
    tt.n.rx_datarate.max = 57600;
    tt.n.rx_datarate.offset = 0;
    tt.n.rx_datarate.unit = TT_UNIT_W;
    tt.n.rx_datarate.divider = 1;
    tt.n.rx_datarate.high_res = pdTRUE;
    tt.n.rx_datarate.resend_time = TT_FAST;
    tt.n.rx_datarate.chart = TT_NO_TELEMETRY;
    tt.n.rx_datarate.gauge = TT_NO_TELEMETRY;
    
    recalc_telemetry_limits();
    
}

void recalc_telemetry_limits(){
    tt.n.batt_v.max = configuration.r_top / 1000;
    tt.n.primary_i.max = ((5000ul * configuration.ct1_ratio) / configuration.ct1_burden)/100;
    tt.n.bus_v.max = configuration.r_top / 1000;
    tt.n.batt_i.max = ((configuration.ct2_ratio * 50) / configuration.ct2_burden);
    tt.n.temp1.max = configuration.temp1_max;
    tt.n.temp2.max = configuration.temp2_max;
    tt.n.avg_power.max = (configuration.r_top / 1000) * ((configuration.ct2_ratio * 50) / configuration.ct2_burden);
}


void tsk_overlay_chart_stop(){
    chart=0;
}
void tsk_overlay_chart_start(){
    chart=1;
}

void show_overlay_100ms(TERMINAL_HANDLE * handle){

    if(portM->term_mode == PORT_TERM_VT100){
        TERM_sendVT100Code(handle, _VT100_CURSOR_SAVE_POSITION,0);
    	TERM_sendVT100Code(handle, _VT100_CURSOR_DISABLE,0);

    	uint8_t row_pos = 1;
    	uint8_t col_pos = 90;
    	TERM_Box(handle, row_pos, col_pos, row_pos + 11, col_pos + 25);
    	TERM_setCursorPos(handle, row_pos + 1, col_pos + 1);
    	ttprintf("Bus Voltage:       %4iV", tt.n.bus_v.value);

    	TERM_setCursorPos(handle, row_pos + 2, col_pos + 1);
    	ttprintf("Battery Voltage:   %4iV", tt.n.batt_v.value);

    	TERM_setCursorPos(handle, row_pos + 3, col_pos + 1);
    	ttprintf("Temp 1:          %4i *C", tt.n.temp1.value);

    	TERM_setCursorPos(handle, row_pos + 4, col_pos + 1);
    	ttprintf("Temp 2:          %4i *C", tt.n.temp2.value);

    	TERM_setCursorPos(handle, row_pos + 5, col_pos + 1);
        ttprintf("Bus status: ");

    	switch (tt.n.bus_status.value) {
    	case BUS_READY:
            ttprintf("       Ready");
    		break;
    	case BUS_CHARGING:
            ttprintf("    Charging");
    		break;
    	case BUS_OFF:
            ttprintf("         OFF");
    		break;
        case BUS_BATT_UV_FLT:
            ttprintf("  Battery UV");
    		break;
        case BUS_TEMP1_FAULT:
            ttprintf("  Temp fault");
    		break;
    	}

    	TERM_setCursorPos(handle, row_pos + 6, col_pos + 1);
    	ttprintf("Average power:     %4iW", tt.n.avg_power.value);

    	TERM_setCursorPos(handle, row_pos + 7, col_pos + 1);
    	ttprintf("Average Current: %4i.%iA", tt.n.batt_i.value / 10, tt.n.batt_i.value % 10);

    	TERM_setCursorPos(handle, row_pos + 8, col_pos + 1);
    	ttprintf("Primary Current:   %4iA", tt.n.primary_i.value);
        
        TERM_setCursorPos(handle, row_pos + 9, col_pos + 1);
    	ttprintf("MIDI voices:         %1i/4", tt.n.midi_voices.value);
        
        TERM_setCursorPos(handle, row_pos + 10, col_pos + 1);
        if(tt.n.fres.value==-1){
            ttprintf("Fres:        no feedback");
        }else{
    	    ttprintf("Fres:          %4i.%ikHz", tt.n.fres.value / 10, tt.n.fres.value % 10);
        }

    	TERM_sendVT100Code(handle, _VT100_CURSOR_RESTORE_POSITION,0);
    	TERM_sendVT100Code(handle, _VT100_CURSOR_ENABLE,0);
        
    
    }else{
        for(uint32_t i = 0;i<N_TELE;i++){
            if(tt.a[i].resend_time == TT_FAST){
                if(tt.a[i].gauge!=TT_NO_TELEMETRY){
                    if(tt.a[i].high_res){
                        send_gauge32(tt.a[i].gauge, tt.a[i].value, handle);
                    }else{
                        send_gauge(tt.a[i].gauge, tt.a[i].value, handle);
                    }
                }
                if(tt.a[i].chart!=TT_NO_TELEMETRY && chart){
                    if(tt.a[i].divider>1){
                        send_chart(tt.a[i].chart, tt.a[i].value / tt.a[i].divider, handle);
                    }else{
                        send_chart(tt.a[i].chart, tt.a[i].value, handle);
                    }
                }
            }
        }
        

        if(portM->term_mode==PORT_TERM_MQTT){
             ALARMS temp;
            if(alarm_pop(&temp)==pdPASS){
                send_event(&temp,handle);
                alarm_free(&temp);
            }
        }else{
            if(chart){
                send_chart_draw(handle);
            }
        }
    }
	
}

void show_overlay_400ms(TERMINAL_HANDLE * handle) {
    if(portM->term_mode == PORT_TERM_TT){
        for(uint32_t i = 0;i<N_TELE;i++){
            if(tt.a[i].resend_time == TT_SLOW){
                if(tt.a[i].gauge!=TT_NO_TELEMETRY){
                    if(tt.a[i].high_res){
                        send_gauge32(tt.a[i].gauge, tt.a[i].value, handle);
                    }else{
                        send_gauge(tt.a[i].gauge, tt.a[i].value, handle);
                    }
                }
                if(tt.a[i].chart!=TT_NO_TELEMETRY && chart){
                    if(tt.a[i].divider>1){
                        send_chart(tt.a[i].chart, tt.a[i].value / tt.a[i].divider, handle);
                    } else {
                        send_chart(tt.a[i].chart, tt.a[i].value, handle);
                    }
                }
            }
        }

        if(portM->term_mode!=PORT_TERM_MQTT){
            send_status(tt.n.bus_status.value!=BUS_OFF,
                        interrupter.mode!=INTR_MODE_OFF,
                        configuration.ps_scheme!=AC_NO_RELAY_BUS_SCHEME,
                        sysfault.interlock,
                        handle);
        }
    }
}

void calculate_datarate(){
    static uint32 last_rx=0;
    static uint32 last_tx=0;
  
    tt.n.rx_datarate.value = ((uart_bytes_received-last_rx)*10)/4;
    tt.n.tx_datarate.value = ((uart_bytes_transmitted-last_tx)*10)/4;
    
    last_rx=uart_bytes_received;
    last_tx=uart_bytes_transmitted;
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
    
    TERMINAL_HANDLE * handle = pvParameters;

	/* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */

	/* `#END` */
    switch(portM->term_mode){
        case PORT_TERM_TT:
            alarm_push(ALM_PRIO_INFO,warn_task_TT_overlay, portM->num);
        break;
        case PORT_TERM_MQTT:
            alarm_push(ALM_PRIO_INFO,warn_task_MQTT_overlay, portM->num);
        break;
        case PORT_TERM_VT100:
            alarm_push(ALM_PRIO_INFO,warn_task_VT100_overlay, ALM_NO_VALUE);
        break; 
    }

	    
    for (;;) {
		/* `#START TASK_LOOP_CODE` */
        xSemaphoreTake(portM->term_block, portMAX_DELAY);
        show_overlay_100ms(handle);
        if(cnt<3){
            cnt++;
        }else{
            cnt=0;
            calculate_datarate();
            show_overlay_400ms(handle);
        }
        
        
        
        xSemaphoreGive(portM->term_block);
        
		/* `#END` */
        if(portM->term_mode==PORT_TERM_VT100){
		    vTaskDelay(500 / portTICK_PERIOD_MS);
        }else{
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
	}
}

/*****************************************************************************
* Helper function for spawning the overlay task
******************************************************************************/
void start_overlay_task(TERMINAL_HANDLE * handle){
    

    if (portM->telemetry_handle == NULL) {
        switch(portM->type){
        case PORT_TYPE_SERIAL:
        	xTaskCreate(tsk_overlay_TaskProc, "Overl_S", STACK_OVERLAY, handle, PRIO_OVERLAY, &portM->telemetry_handle);
        break;
        case PORT_TYPE_USB:
    		xTaskCreate(tsk_overlay_TaskProc, "Overl_U", STACK_OVERLAY, handle, PRIO_OVERLAY, &portM->telemetry_handle);
        break;
        case PORT_TYPE_MIN:
        	xTaskCreate(tsk_overlay_TaskProc, "Overl_M", STACK_OVERLAY, handle, PRIO_OVERLAY, &portM->telemetry_handle);
        break;
        }    
    }
}


/*****************************************************************************
* Helper function for killing the overlay task
******************************************************************************/
void stop_overlay_task(TERMINAL_HANDLE * handle){
    if (portM->telemetry_handle != NULL) {
        vTaskDelete(portM->telemetry_handle);
    	portM->telemetry_handle = NULL;
    }
}

/*****************************************************************************
* Initializes the teslaterm telemetry
* Spawns the overlay task for telemetry stream generation
******************************************************************************/
void init_tt(uint8_t with_chart, TERMINAL_HANDLE * handle){
    
    for(uint32_t i = 0;i<N_TELE;i++){
        if(tt.a[i].gauge!=TT_NO_TELEMETRY){
            if(tt.a[i].high_res){
                send_gauge_config32(tt.a[i].gauge, tt.a[i].min, tt.a[i].max, tt.a[i].divider, tt.a[i].name, handle);
            }else{
                send_gauge_config(tt.a[i].gauge, tt.a[i].min, tt.a[i].max, tt.a[i].name, handle);
            }
        }
        if(tt.a[i].chart!=TT_NO_TELEMETRY && with_chart){
            if(tt.a[i].high_res){
                send_chart_config(tt.a[i].chart, tt.a[i].min / tt.a[i].divider, tt.a[i].max / tt.a[i].divider, tt.a[i].offset / tt.a[i].divider, tt.a[i].unit, tt.a[i].name, handle);
            }else{
                send_chart_config(tt.a[i].chart, tt.a[i].min, tt.a[i].max, tt.a[i].offset, tt.a[i].unit, tt.a[i].name, handle);
            }
        }
    }

    start_overlay_task(handle);
}

/*****************************************************************************
* 
******************************************************************************/
uint8_t CMD_status(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf("Usage: status [start|stop]\r\n");
    }
	

	if(strcmp(args[0], "start") == 0){
		start_overlay_task(handle);
        return TERM_CMD_EXIT_SUCCESS;
	}
	if(strcmp(args[0], "stop") == 0){
		stop_overlay_task(handle);
        return TERM_CMD_EXIT_SUCCESS;
	}
    return TERM_CMD_EXIT_SUCCESS;
}

uint8_t CMD_tterm(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf("Usage: tterm [start|stop|mqtt|notelemetry]\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    }
    port_str * ptr = handle->port;
    
	if(strcmp(args[0], "start") == 0){
        ptr->term_mode = PORT_TERM_TT;
        init_tt(pdTRUE,handle);
        return TERM_CMD_EXIT_SUCCESS;
    }
    if(strcmp(args[0], "mqtt") == 0){
        ptr->term_mode = PORT_TERM_MQTT;
        init_tt(pdFALSE,handle);
        return TERM_CMD_EXIT_SUCCESS;
    }
    if(strcmp(args[0], "notelemetry") == 0){
        ptr->term_mode = PORT_TERM_TT;
        stop_overlay_task(handle);
        return TERM_CMD_EXIT_SUCCESS;
    }
	if(strcmp(args[0], "stop") == 0){
        ptr->term_mode = PORT_TERM_VT100;
        stop_overlay_task(handle);
        return TERM_CMD_EXIT_SUCCESS;
	} 
    return TERM_CMD_EXIT_SUCCESS;
}


uint8_t CMD_telemetry(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf(   "Usage: telemetry list\r\n"
                    "       telemetry ls\r\n"
                    "       telemetry gauge n [name]\r\n"
                    "       telemetry chart n [name]\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    } 
            
    if(strcmp(args[0], "list") == 0){
        for(int i=0;i<N_GAUGES;i++){
            int cnt=-1;
            for(uint8_t w=0;w<N_TELE;w++){
                if(i==tt.a[w].gauge){
                    cnt=w;
                    break;
                }
            }
            if(cnt==TT_NO_TELEMETRY){
                ttprintf("Gauge %i: none\r\n",i);
            }else{
                if(tt.a[cnt].high_res && tt.a[cnt].divider > 1){
                    ttprintf("Gauge %i: %s = %.2f %s\r\n",i,tt.a[cnt].name,(float)tt.a[cnt].value/(float)tt.a[cnt].divider,units[tt.a[cnt].unit]);
                }else{
                    ttprintf("Gauge %i: %s = %i %s\r\n",i,tt.a[cnt].name,tt.a[cnt].value,units[tt.a[cnt].unit]);
                }
            }
        }
        for(int i=0;i<N_CHARTS;i++){
            int cnt=-1;
            for(uint8_t w=0;w<N_TELE;w++){
                if(i==tt.a[w].chart){
                    cnt=w;
                    break;
                }
            }
            if(cnt==TT_NO_TELEMETRY){
                ttprintf("Chart %i: none\r\n",i);
            }else{
                if(tt.a[cnt].high_res && tt.a[cnt].divider > 1){
                    ttprintf("Chart %i: %s = %.2f %s\r\n",i,tt.a[cnt].name,(float)tt.a[cnt].value/(float)tt.a[cnt].divider,units[tt.a[cnt].unit]);
                }else{
                    ttprintf("Chart %i: %s = %i %s\r\n",i,tt.a[cnt].name,tt.a[cnt].value,units[tt.a[cnt].unit]);
                }
            }
        }
        return TERM_CMD_EXIT_SUCCESS;
        
    } else if(strcmp(args[0], "gauge") == 0 && argCount == 3){
        int n = atoi(args[1]);
        if(n>=N_GAUGES || n<0){
            ttprintf("Gauge number must be between 0 and 6\r\n");
            return 0;
        }
        if(strcmp(args[2],"none")==0){
            for(uint8_t w=0;w<N_TELE;w++){
                if(n==tt.a[w].gauge){
                    tt.a[w].gauge=TT_NO_TELEMETRY;
                }
            }
            send_gauge_config(n,0,0,"none",handle);
            ttprintf("OK\r\n");
            return TERM_CMD_EXIT_SUCCESS;
        }
        
        
        int cnt=-1;
        for(uint8_t w=0;w<N_TELE;w++){
            if(strcmp(args[2],tt.a[w].name)==0){
                for(uint8_t i=0;i<N_TELE;i++){
                    if(n==tt.a[i].gauge)tt.a[i].gauge=TT_NO_TELEMETRY;
                }
                cnt=w;
                break;
            }
        }
        if(cnt==TT_NO_TELEMETRY){
            ttprintf("Telemetry name not found\r\n"); 
        }else{
            tt.a[cnt].gauge = n;
            ttprintf("OK\r\n");
            if(tt.a[cnt].high_res){
                    send_gauge_config32(n,tt.a[cnt].min,tt.a[cnt].max,tt.a[cnt].divider,tt.a[cnt].name,handle);
            }else{
                    send_gauge_config(n,tt.a[cnt].min,tt.a[cnt].max,tt.a[cnt].name,handle);
            }
        }
        return TERM_CMD_EXIT_SUCCESS;

    } else if (strcmp(args[0], "chart") == 0 && argCount == 3) {
        int8_t n = atoi(args[1]);
        if(n>=N_CHARTS || n<0){
            ttprintf("Chart number must be between 0 and 3\r\n");
            return TERM_CMD_EXIT_SUCCESS;
        }
        if(strcmp(args[2],"none")==0){
            for(uint8_t w=0;w<N_TELE;w++){
                if(n==tt.a[w].chart){
                    tt.a[w].chart=TT_NO_TELEMETRY;
                }
            }
            send_chart_config(n,0,0,0,TT_UNIT_NONE,"none",handle);
            ttprintf("OK\r\n");
            return TERM_CMD_EXIT_SUCCESS;
        }
        int cnt=-1;
        for(uint8_t w=0;w<N_TELE;w++){
            if(strcmp(args[2],tt.a[w].name)==0){
                for(uint8_t i=0;i<N_TELE;i++){
                    if(n==tt.a[i].chart)tt.a[i].chart=TT_NO_TELEMETRY;
                }
                cnt=w;
                break;
            }
        }
        if(cnt==TT_NO_TELEMETRY){
            ttprintf("Telemetry name not found\r\n");
        }else{
            tt.a[cnt].chart = n;
            ttprintf("OK\r\n");
            send_chart_config(n,tt.a[cnt].min,tt.a[cnt].max,tt.a[cnt].offset,tt.a[cnt].unit,tt.a[cnt].name,handle);
        }
        return TERM_CMD_EXIT_SUCCESS;

    } else if (strcmp(args[0], "ls") == 0) {
        for(uint8_t w=0;w<N_TELE;w++){
            ttprintf("%i: %s\r\n", w+1, tt.a[w].name);
        }
        return TERM_CMD_EXIT_SUCCESS;
    }
    return TERM_CMD_EXIT_SUCCESS;
}
