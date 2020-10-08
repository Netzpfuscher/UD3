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
#include "tasks/tsk_priority.h"

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

#include "ntlibc.h"



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
    //tt.n.i2t_i.high_res = pdFALSE;
    tt.n.i2t_i.high_res = pdTRUE;
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
    tt.n.avg_power.unit = TT_UNIT_W;
    tt.n.avg_power.divider = 1;
    tt.n.avg_power.high_res = pdTRUE;
    tt.n.avg_power.resend_time = TT_FAST;
    tt.n.avg_power.chart = 3;
    tt.n.avg_power.gauge = 2;
    
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
    	ret = snprintf(buffer, sizeof(buffer), "Bus Voltage:       %4iV", tt.n.bus_v.value);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 2, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Battery Voltage:   %4iV", tt.n.batt_v.value);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 3, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Temp 1:          %4i *C", tt.n.temp1.value);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 4, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Temp 2:          %4i *C", tt.n.temp2.value);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 5, col_pos + 1, ptr);
        SEND_CONST_STRING("Bus status: ", ptr);

    	switch (tt.n.bus_status.value) {
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
    	ret = snprintf(buffer, sizeof(buffer), "Average power:     %4iW", tt.n.avg_power.value);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 7, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Average Current: %4i.%iA", tt.n.batt_i.value / 10, tt.n.batt_i.value % 10);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 8, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Primary Current:   %4iA", tt.n.primary_i.value);
        send_buffer((uint8_t*)buffer,ret,ptr);
        
        Term_Move_Cursor(row_pos + 9, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "MIDI voices:         %1i/4", tt.n.midi_voices.value);
        send_buffer((uint8_t*)buffer,ret,ptr);
        
        Term_Move_Cursor(row_pos + 10, col_pos + 1, ptr);
        if(tt.n.fres.value==-1){
            ret = snprintf(buffer, sizeof(buffer), "Fres:        no feedback");
        }else{
    	    ret = snprintf(buffer, sizeof(buffer), "Fres:          %4i.%ikHz", tt.n.fres.value / 10, tt.n.fres.value % 10);
        }
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Restore_Cursor(ptr);
    	Term_Enable_Cursor(ptr);
        
    
    }else{
        for(uint32_t i = 0;i<N_TELE;i++){
            if(tt.a[i].resend_time == TT_FAST){
                if(tt.a[i].gauge!=TT_NO_TELEMETRY){
                    if(tt.a[i].high_res){
                        send_gauge32(tt.a[i].gauge, tt.a[i].value, ptr);
                    }else{
                        send_gauge(tt.a[i].gauge, tt.a[i].value, ptr);
                    }
                }
                if(tt.a[i].chart!=TT_NO_TELEMETRY && chart){
                    send_chart(tt.a[i].chart, tt.a[i].value, ptr);
                }
            }
        }
        

        if(ptr->term_mode==PORT_TERM_MQTT){
             ALARMS temp;
            if(alarm_pop(&temp)==pdPASS){
                send_event(&temp,ptr);
            }
        }else{
            if(chart){
                send_chart_draw(ptr);
            }
        }
    }
	
}

void show_overlay_400ms(port_str *ptr) {
    if(ptr->term_mode == PORT_TERM_TT){
        for(uint32_t i = 0;i<N_TELE;i++){
            if(tt.a[i].resend_time == TT_FAST){
                if(tt.a[i].gauge!=TT_NO_TELEMETRY){
                    if(tt.a[i].high_res){
                        send_gauge32(tt.a[i].gauge, tt.a[i].value, ptr);
                    }else{
                        send_gauge(tt.a[i].gauge, tt.a[i].value, ptr);
                    }
                }
                if(tt.a[i].chart!=TT_NO_TELEMETRY && chart){
                    send_chart(tt.a[i].chart, tt.a[i].value, ptr);
                }
            }
        }

        if(ptr->term_mode!=PORT_TERM_MQTT){
            send_status(tt.n.bus_status.value!=BUS_OFF,
                        tr_running!=0,
                        configuration.ps_scheme!=AC_NO_RELAY_BUS_SCHEME,
                        sysfault.interlock,
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

/*****************************************************************************
* Helper function for spawning the overlay task
******************************************************************************/
void start_overlay_task(port_str *ptr){
    if (ptr->telemetry_handle == NULL) {
        switch(ptr->type){
        case PORT_TYPE_SERIAL:
        	xTaskCreate(tsk_overlay_TaskProc, "Overl_S", STACK_OVERLAY, ptr, PRIO_OVERLAY, &ptr->telemetry_handle);
        break;
        case PORT_TYPE_USB:
    		xTaskCreate(tsk_overlay_TaskProc, "Overl_U", STACK_OVERLAY, ptr, PRIO_OVERLAY, &ptr->telemetry_handle);
        break;
        case PORT_TYPE_MIN:
        	xTaskCreate(tsk_overlay_TaskProc, "Overl_M", STACK_OVERLAY, ptr, PRIO_OVERLAY, &ptr->telemetry_handle);
        break;
        }    
    }
}


/*****************************************************************************
* Helper function for killing the overlay task
******************************************************************************/
void stop_overlay_task(port_str *ptr){
    if (ptr->telemetry_handle != NULL) {
        vTaskDelete(ptr->telemetry_handle);
    	ptr->telemetry_handle = NULL;
    }
}

/*****************************************************************************
* Initializes the teslaterm telemetry
* Spawns the overlay task for telemetry stream generation
******************************************************************************/
void init_tt(uint8_t with_chart, port_str *ptr){
    
    for(uint32_t i = 0;i<N_TELE;i++){
        if(tt.a[i].gauge!=TT_NO_TELEMETRY){
            if(tt.a[i].high_res){
                send_gauge_config32(tt.a[i].gauge, tt.a[i].min, tt.a[i].max, tt.a[i].divider, tt.a[i].name, ptr);
            }else{
                send_gauge_config(tt.a[i].gauge, tt.a[i].min, tt.a[i].max, tt.a[i].name, ptr);
            }
        }
        if(tt.a[i].chart!=TT_NO_TELEMETRY && with_chart){
            send_chart_config(tt.a[i].chart, tt.a[i].min, tt.a[i].max, tt.a[i].offset, tt.a[i].unit, tt.a[i].name, ptr);
        }
    }

    start_overlay_task(ptr);
}

/*****************************************************************************
* 
******************************************************************************/
uint8_t command_status(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    CHECK_NULL(commandline);
	

	if (ntlibc_stricmp(commandline, "start") == 0) {
		start_overlay_task(ptr);
        return 1;
	}
	if (ntlibc_stricmp(commandline, "stop") == 0) {
		stop_overlay_task(ptr);
        return 1;
	}

	HELP_TEXT("Usage: status [start|stop]\r\n");
}

uint8_t command_tterm(char *commandline, port_str *ptr){
    SKIP_SPACE(commandline);
    CHECK_NULL(commandline);
    
	if (ntlibc_stricmp(commandline, "start") == 0) {
        ptr->term_mode = PORT_TERM_TT;
        init_tt(pdTRUE,ptr);
        return 1;
    }
    if (ntlibc_stricmp(commandline, "mqtt") == 0) {
        ptr->term_mode = PORT_TERM_MQTT;
        init_tt(pdFALSE,ptr);
        return 1;
    }
	if (ntlibc_stricmp(commandline, "notelemetry") == 0) {
        ptr->term_mode = PORT_TERM_TT;
        stop_overlay_task(ptr);
        return 1;
    }
	if (ntlibc_stricmp(commandline, "stop") == 0) {
        ptr->term_mode = PORT_TERM_VT100;
        stop_overlay_task(ptr);
        return 1;
	} 
    
    HELP_TEXT("Usage: tterm [start|stop|mqtt|notelemetry]\r\n");
}


uint8_t telemetry_command_setup(char *commandline, port_str *ptr){
    SKIP_SPACE(commandline);
    CHECK_NULL(commandline); 
    
    char *buffer[5];
    char temp[40];
    uint8_t ret;
    
    uint8_t items = split(commandline, buffer, sizeof(buffer)/sizeof(char*), ' ');
    
    
    if (ntlibc_stricmp(buffer[0], "list") == 0){
        for(int i=0;i<N_GAUGES;i++){
            int cnt=-1;
            for(uint8_t w=0;w<N_TELE;w++){
                if(i==tt.a[w].gauge){
                    cnt=w;
                    break;
                }
            }
            if(cnt==TT_NO_TELEMETRY){
                ret = snprintf(temp, sizeof(temp),"Gauge %i: none\r\n",i);
            }else{
                if(tt.a[cnt].high_res && tt.a[cnt].divider > 1){
                    ret = snprintf(temp, sizeof(temp),"Gauge %i: %s = %.2f %s\r\n",i,tt.a[cnt].name,(float)tt.a[cnt].value/(float)tt.a[cnt].divider,units[tt.a[cnt].unit]);
                }else{
                    ret = snprintf(temp, sizeof(temp),"Gauge %i: %s = %i %s\r\n",i,tt.a[cnt].name,tt.a[cnt].value,units[tt.a[cnt].unit]);
                }
            }
            send_buffer((uint8_t*)temp,ret,ptr);
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
                ret = snprintf(temp, sizeof(temp),"Chart %i: none\r\n",i);
            }else{
                if(tt.a[cnt].high_res && tt.a[cnt].divider > 1){
                    ret = snprintf(temp, sizeof(temp),"Chart %i: %s = %.2f %s\r\n",i,tt.a[cnt].name,(float)tt.a[cnt].value/(float)tt.a[cnt].divider,units[tt.a[cnt].unit]);
                }else{
                    ret = snprintf(temp, sizeof(temp),"Chart %i: %s = %i %s\r\n",i,tt.a[cnt].name,tt.a[cnt].value,units[tt.a[cnt].unit]);
                }
            }
            send_buffer((uint8_t*)temp,ret,ptr);
        }
        return pdPASS;
        
    } else if (ntlibc_stricmp(buffer[0], "gauge") == 0 && items == 3) {
        int n = ntlibc_atoi(buffer[1]);
        if(n>=N_GAUGES || n<0){
            SEND_CONST_STRING("Gauge number must be between 0 and 6\r\n",ptr);
            return 0;
        }
        if(ntlibc_stricmp(buffer[2],"none")==0){
            for(uint8_t w=0;w<N_TELE;w++){
                if(n==tt.a[w].gauge){
                    tt.a[w].gauge=TT_NO_TELEMETRY;
                }
            }
            send_gauge_config(n,0,0,"none",ptr);
            SEND_CONST_STRING("OK\r\n",ptr);
            return pdPASS;
        }
        
        
        int cnt=-1;
        for(uint8_t w=0;w<N_TELE;w++){
            if(ntlibc_stricmp(buffer[2],tt.a[w].name)==0){
                for(uint8_t i=0;i<N_TELE;i++){
                    if(n==tt.a[i].gauge)tt.a[i].gauge=TT_NO_TELEMETRY;
                }
                cnt=w;
                break;
            }
        }
        if(cnt==TT_NO_TELEMETRY){
            SEND_CONST_STRING("Telemetry name not found\r\n",ptr); 
        }else{
            tt.a[cnt].gauge = n;
            SEND_CONST_STRING("OK\r\n",ptr);
            if(tt.a[cnt].high_res){
                    send_gauge_config32(n,tt.a[cnt].min,tt.a[cnt].max,tt.a[cnt].divider,tt.a[cnt].name,ptr);
            }else{
                    send_gauge_config(n,tt.a[cnt].min,tt.a[cnt].max,tt.a[cnt].name,ptr);
            }
        }
        return pdPASS;

    } else if (ntlibc_stricmp(buffer[0], "chart") == 0 && items == 3) {
        int8_t n = ntlibc_atoi(buffer[1]);
        if(n>=N_CHARTS || n<0){
            SEND_CONST_STRING("Chart number must be between 0 and 3\r\n",ptr);
            return 0;
        }
        if(ntlibc_stricmp(buffer[2],"none")==0){
            for(uint8_t w=0;w<N_TELE;w++){
                if(n==tt.a[w].chart){
                    tt.a[w].chart=TT_NO_TELEMETRY;
                }
            }
            send_chart_config(n,0,0,0,TT_UNIT_NONE,"none",ptr);
            SEND_CONST_STRING("OK\r\n",ptr);
            return pdPASS;
        }
        int cnt=-1;
        for(uint8_t w=0;w<N_TELE;w++){
            if(ntlibc_stricmp(buffer[2],tt.a[w].name)==0){
                for(uint8_t i=0;i<N_TELE;i++){
                    if(n==tt.a[i].chart)tt.a[i].chart=TT_NO_TELEMETRY;
                }
                cnt=w;
                break;
            }
        }
        if(cnt==TT_NO_TELEMETRY){
            SEND_CONST_STRING("Telemetry name not found\r\n",ptr);
        }else{
            tt.a[cnt].chart = n;
            SEND_CONST_STRING("OK\r\n",ptr);
            send_chart_config(n,tt.a[cnt].min,tt.a[cnt].max,tt.a[cnt].offset,tt.a[cnt].unit,tt.a[cnt].name,ptr);
        }
        return pdPASS;

    } else if (ntlibc_stricmp(buffer[0], "ls") == 0) {
        for(uint8_t w=0;w<N_TELE;w++){
            ret = snprintf(temp, sizeof(temp),"%i: %s\r\n", w+1, tt.a[w].name);
            send_buffer((uint8_t*)temp,ret,ptr);
        }
        return pdPASS;
    }
    
    
 	HELP_TEXT("Usage: telemetry list\r\n"
              "       telemetry ls\r\n"
              "       telemetry gauge n [name]\r\n"
              "       telemetry chart n [name]\r\n");
}
