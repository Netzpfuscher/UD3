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

TELE meter[13];

const TELE_HUMAN metering = {
    &meter[0],
    &meter[1],
    &meter[2],
    &meter[3],
    &meter[4],
    &meter[5],
    &meter[6],
    &meter[7],
    &meter[8],
    &meter[9],
    &meter[10],
    &meter[11],
    &meter[12]
};

void init_telemetry(){
    
    metering.i2t_i->name = "Fuse";
    metering.i2t_i->value = 0;
    metering.i2t_i->min = 0;
    metering.i2t_i->max = 100;
    metering.i2t_i->offset = 0;
    metering.i2t_i->unit = TT_UNIT_PERCENT;
    metering.i2t_i->divider = 1;
    metering.i2t_i->high_res = pdFALSE;
    metering.i2t_i->resend_time = TT_FAST;
    metering.i2t_i->chart = TT_NO_TELEMETRY;
    metering.i2t_i->gauge = 6;
    
    metering.fres->name = "Fres";
    metering.fres->value = 0;
    metering.fres->min = 0;
    metering.fres->max = 500;
    metering.fres->offset = 0;
    metering.fres->unit = TT_UNIT_Hz;
    metering.fres->divider = 1000;
    metering.fres->high_res = pdTRUE;
    metering.fres->resend_time = TT_SLOW;
    metering.fres->chart = TT_NO_TELEMETRY;
    metering.fres->gauge = TT_NO_TELEMETRY;
    
    metering.duty->name = "Dutycycle";
    metering.duty->value = 0;
    metering.duty->min = 0;
    metering.duty->max = 100;
    metering.duty->offset = 0;
    metering.duty->unit = TT_UNIT_PERCENT;
    metering.duty->divider = 10;
    metering.duty->high_res = pdTRUE;
    metering.duty->resend_time = TT_FAST;
    metering.duty->chart = TT_NO_TELEMETRY;
    metering.duty->gauge = TT_NO_TELEMETRY;
    
    metering.driver_v->name = "Driver_Voltage";
    metering.driver_v->value = 0;
    metering.driver_v->min = 0;
    metering.driver_v->max = 30;
    metering.driver_v->offset = 0;
    metering.driver_v->unit = TT_UNIT_V;
    metering.driver_v->divider = 1000;
    metering.driver_v->high_res = pdTRUE;
    metering.driver_v->resend_time = TT_SLOW;
    metering.driver_v->chart = TT_NO_TELEMETRY;
    metering.driver_v->gauge = TT_NO_TELEMETRY;
    
    metering.bus_status->name = "Bus_State";
    metering.bus_status->value = 0;
    metering.bus_status->min = 0;
    metering.bus_status->max = 7;
    metering.bus_status->offset = 0;
    metering.bus_status->unit = TT_UNIT_NONE;
    metering.bus_status->divider = 1;
    metering.bus_status->high_res = pdFALSE;
    metering.bus_status->resend_time =TT_SLOW;
    metering.bus_status->chart = TT_NO_TELEMETRY;
    metering.bus_status->gauge = TT_NO_TELEMETRY;
    
    metering.batt_v->name = "Voltage2";
    metering.batt_v->value = 0;
    metering.batt_v->min = 0;
    metering.batt_v->max = configuration.r_top / 1000;;
    metering.batt_v->offset = 0;
    metering.batt_v->unit = TT_UNIT_V;
    metering.batt_v->divider = 1;
    metering.batt_v->high_res = pdFALSE;
    metering.batt_v->resend_time = TT_FAST;
    metering.batt_v->chart = TT_NO_TELEMETRY;
    metering.batt_v->gauge = TT_NO_TELEMETRY;
    
    metering.primary_i->name = "Primary";
    metering.primary_i->value = 0;
    metering.primary_i->min = 0;
    metering.primary_i->max = ((5000ul * configuration.ct1_ratio) / configuration.ct1_burden)/100;
    metering.primary_i->offset = 0;
    metering.primary_i->unit = TT_UNIT_A;
    metering.primary_i->divider = 0;
    metering.primary_i->high_res = pdFALSE;
    metering.primary_i->resend_time = TT_FAST;
    metering.primary_i->chart = 2;
    metering.primary_i->gauge = 4;
    
    metering.midi_voices->name = "Voices";
    metering.midi_voices->value = 0;
    metering.midi_voices->min = 0;
    metering.midi_voices->max = 4;
    metering.midi_voices->offset = 0;
    metering.midi_voices->unit = TT_UNIT_NONE;
    metering.midi_voices->divider = 1;
    metering.midi_voices->high_res = pdFALSE;
    metering.midi_voices->resend_time = TT_FAST;
    metering.midi_voices->chart = TT_NO_TELEMETRY;
    metering.midi_voices->gauge = 5;
    
    metering.bus_v->name = "Voltage";
    metering.bus_v->value = 0;
    metering.bus_v->min = 0;
    metering.bus_v->max = configuration.r_top / 1000;
    metering.bus_v->offset = 0;
    metering.bus_v->unit = TT_UNIT_V;
    metering.bus_v->divider = 1;
    metering.bus_v->high_res = pdFALSE;
    metering.bus_v->resend_time = TT_FAST;
    metering.bus_v->chart = 0;
    metering.bus_v->gauge = 0;
    
    metering.temp1->name = "Temp1";
    metering.temp1->value = 0;
    metering.temp1->min = 0;
    metering.temp1->max = configuration.temp1_max;
    metering.temp1->offset = 0;
    metering.temp1->unit = TT_UNIT_C;
    metering.temp1->divider = 1;
    metering.temp1->high_res = pdFALSE;
    metering.temp1->resend_time = TT_SLOW;
    metering.temp1->chart = 1;
    metering.temp1->gauge = 1;
    
    metering.temp2->name = "Temp2";
    metering.temp2->value = 0;
    metering.temp2->min = 0;
    metering.temp2->max = configuration.temp2_max;
    metering.temp2->offset = 0;
    metering.temp2->unit = TT_UNIT_C;
    metering.temp2->divider = 1;
    metering.temp2->high_res = pdFALSE;
    metering.temp2->resend_time = TT_SLOW;
    metering.temp2->chart = TT_NO_TELEMETRY;
    metering.temp2->gauge = TT_NO_TELEMETRY;
       
    metering.batt_i->name = "Current";
    metering.batt_i->value = 0;
    metering.batt_i->min = 0;
    metering.batt_i->max = ((configuration.ct2_ratio * 50) / configuration.ct2_burden);
    metering.batt_i->offset = 0;
    metering.batt_i->unit = TT_UNIT_A;
    metering.batt_i->divider = 10;
    metering.batt_i->high_res = pdTRUE;
    metering.batt_i->resend_time = TT_FAST;
    metering.batt_i->chart = TT_NO_TELEMETRY;
    metering.batt_i->gauge = 3;
        
    metering.avg_power->name = "Power";
    metering.avg_power->value = 0;
    metering.avg_power->min = 0;
    metering.avg_power->max = (configuration.r_top / 1000) * ((configuration.ct2_ratio * 50) / configuration.ct2_burden);
    metering.avg_power->offset = 0;
    metering.avg_power->unit = TT_UNIT_W;
    metering.avg_power->divider = 1;
    metering.avg_power->high_res = pdTRUE;
    metering.avg_power->resend_time = TT_FAST;
    metering.avg_power->chart = 3;
    metering.avg_power->gauge = 2;
    
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
    	ret = snprintf(buffer, sizeof(buffer), "Bus Voltage:       %4iV", metering.bus_v->value);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 2, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Battery Voltage:   %4iV", metering.batt_v->value);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 3, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Temp 1:          %4i *C", metering.temp1->value);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 4, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Temp 2:          %4i *C", metering.temp2->value);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 5, col_pos + 1, ptr);
        SEND_CONST_STRING("Bus status: ", ptr);

    	switch (metering.bus_status->value) {
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
    	ret = snprintf(buffer, sizeof(buffer), "Average power:     %4iW", metering.avg_power->value);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 7, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Average Current: %4i.%iA", metering.batt_i->value / 10, metering.batt_i->value % 10);
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Move_Cursor(row_pos + 8, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "Primary Current:   %4iA", metering.primary_i->value);
        send_buffer((uint8_t*)buffer,ret,ptr);
        
        Term_Move_Cursor(row_pos + 9, col_pos + 1, ptr);
    	ret = snprintf(buffer, sizeof(buffer), "MIDI voices:         %1i/4", metering.midi_voices->value);
        send_buffer((uint8_t*)buffer,ret,ptr);
        
        Term_Move_Cursor(row_pos + 10, col_pos + 1, ptr);
        if(metering.fres->value==-1){
            ret = snprintf(buffer, sizeof(buffer), "Fres:        no feedback");
        }else{
    	    ret = snprintf(buffer, sizeof(buffer), "Fres:          %4i.%ikHz", metering.fres->value / 10, metering.fres->value % 10);
        }
        send_buffer((uint8_t*)buffer,ret,ptr);

    	Term_Restore_Cursor(ptr);
    	Term_Enable_Cursor(ptr);
        
    
    }else{
        for(uint32_t i = 0;i<sizeof(meter)/sizeof(TELE);i++){
            if(meter[i].resend_time == TT_FAST){
                if(meter[i].gauge!=TT_NO_TELEMETRY){
                    if(meter[i].high_res){
                        send_gauge32(meter[i].gauge, meter[i].value, ptr);
                    }else{
                        send_gauge(meter[i].gauge, meter[i].value, ptr);
                    }
                }
                if(meter[i].chart!=TT_NO_TELEMETRY && chart){
                    send_chart(meter[i].chart, meter[i].value, ptr);
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
        for(uint32_t i = 0;i<sizeof(meter)/sizeof(TELE);i++){
            if(meter[i].resend_time == TT_FAST){
                if(meter[i].gauge!=TT_NO_TELEMETRY){
                    if(meter[i].high_res){
                        send_gauge32(meter[i].gauge, meter[i].value, ptr);
                    }else{
                        send_gauge(meter[i].gauge, meter[i].value, ptr);
                    }
                }
                if(meter[i].chart!=TT_NO_TELEMETRY && chart){
                    send_chart(meter[i].chart, meter[i].value, ptr);
                }
            }
        }
        
        if(ptr->term_mode!=PORT_TERM_MQTT){
            send_status(metering.bus_status->value!=BUS_OFF,
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
            for(uint8_t w=0;w<sizeof(meter)/sizeof(TELE);w++){
                if(i==meter[w].gauge){
                    cnt=w;
                    break;
                }
            }
            if(cnt==TT_NO_TELEMETRY){
                ret = snprintf(temp, sizeof(temp),"Gauge %i: none\r\n",i);
            }else{
                if(meter[cnt].high_res && meter[cnt].divider > 1){
                    ret = snprintf(temp, sizeof(temp),"Gauge %i: %s = %.2f %s\r\n",i,meter[cnt].name,(float)meter[cnt].value/(float)meter[cnt].divider,units[meter[cnt].unit]);
                }else{
                    ret = snprintf(temp, sizeof(temp),"Gauge %i: %s = %i %s\r\n",i,meter[cnt].name,meter[cnt].value,units[meter[cnt].unit]);
                }
            }
            send_buffer((uint8_t*)temp,ret,ptr);
        }
        for(int i=0;i<N_CHARTS;i++){
            int cnt=-1;
            for(uint8_t w=0;w<sizeof(meter)/sizeof(TELE);w++){
                if(i==meter[w].chart){
                    cnt=w;
                    break;
                }
            }
            if(cnt==TT_NO_TELEMETRY){
                ret = snprintf(temp, sizeof(temp),"Chart %i: none\r\n",i);
            }else{
                if(meter[cnt].high_res && meter[cnt].divider > 1){
                    ret = snprintf(temp, sizeof(temp),"Chart %i: %s = %.2f %s\r\n",i,meter[cnt].name,(float)meter[cnt].value/(float)meter[cnt].divider,units[meter[cnt].unit]);
                }else{
                    ret = snprintf(temp, sizeof(temp),"Chart %i: %s = %i %s\r\n",i,meter[cnt].name,meter[cnt].value,units[meter[cnt].unit]);
                }
            }
            send_buffer((uint8_t*)temp,ret,ptr);
        }
        return pdPASS;
        
    } else if (ntlibc_stricmp(buffer[0], "gauge") == 0 && items == 3) {
        int n = ntlibc_atoi(buffer[1]);
        if(n>=N_GAUGES || n<0){
            ret = snprintf(temp, sizeof(temp),"Gauge number must be between 0 and 6\r\n");
            send_buffer((uint8_t*)temp,ret,ptr);
            return 0;
        }
        if(ntlibc_stricmp(buffer[2],"none")==0){
            for(uint8_t w=0;w<sizeof(meter)/sizeof(TELE);w++){
                if(n==meter[w].gauge){
                    meter[w].gauge=TT_NO_TELEMETRY;
                }
            }
            send_gauge_config(n,0,0,"none",ptr);
            ret = snprintf(temp, sizeof(temp),"OK\r\n");
            send_buffer((uint8_t*)temp,ret,ptr);
            return pdPASS;
        }
        
        
        int cnt=-1;
        for(uint8_t w=0;w<sizeof(meter)/sizeof(TELE);w++){
            if(ntlibc_stricmp(buffer[2],meter[w].name)==0){
                for(uint8_t i=0;i<sizeof(meter)/sizeof(TELE);i++){
                    if(n==meter[i].gauge)meter[i].gauge=TT_NO_TELEMETRY;
                }
                cnt=w;
                break;
            }
        }
        if(cnt==TT_NO_TELEMETRY){
            ret = snprintf(temp, sizeof(temp),"Telemetry name not found\r\n");  
        }else{
            meter[cnt].gauge = n;
            ret = snprintf(temp, sizeof(temp),"OK\r\n");
            if(meter[cnt].high_res){
                    send_gauge_config32(n,meter[cnt].min,meter[cnt].max,meter[cnt].divider,meter[cnt].name,ptr);
            }else{
                    send_gauge_config(n,meter[cnt].min,meter[cnt].max,meter[cnt].name,ptr);
            }
        }
        send_buffer((uint8_t*)temp,ret,ptr);
        return pdPASS;

    } else if (ntlibc_stricmp(buffer[0], "chart") == 0 && items == 3) {
        int8_t n = ntlibc_atoi(buffer[1]);
        if(n>=N_CHARTS || n<0){
            ret = snprintf(temp, sizeof(temp),"Chart number must be between 0 and 3\r\n");
            send_buffer((uint8_t*)temp,ret,ptr);
            return 0;
        }
        if(ntlibc_stricmp(buffer[2],"none")==0){
            for(uint8_t w=0;w<sizeof(meter)/sizeof(TELE);w++){
                if(n==meter[w].chart){
                    meter[w].chart=TT_NO_TELEMETRY;
                }
            }
            send_chart_config(n,0,0,0,TT_UNIT_NONE,"none",ptr);
            ret = snprintf(temp, sizeof(temp),"OK\r\n");
            send_buffer((uint8_t*)temp,ret,ptr);
            return pdPASS;
        }
        int cnt=-1;
        for(uint8_t w=0;w<sizeof(meter)/sizeof(TELE);w++){
            if(ntlibc_stricmp(buffer[2],meter[w].name)==0){
                for(uint8_t i=0;i<sizeof(meter)/sizeof(TELE);i++){
                    if(n==meter[i].chart)meter[i].chart=TT_NO_TELEMETRY;
                }
                cnt=w;
                break;
            }
        }
        if(cnt==TT_NO_TELEMETRY){
            ret = snprintf(temp, sizeof(temp),"Telemetry name not found\r\n");  
        }else{
            meter[cnt].chart = n;
            ret = snprintf(temp, sizeof(temp),"OK\r\n");
            send_chart_config(n,meter[cnt].min,meter[cnt].max,meter[cnt].offset,meter[cnt].unit,meter[cnt].name,ptr);
        }
        send_buffer((uint8_t*)temp,ret,ptr);
        return pdPASS;

    } else if (ntlibc_stricmp(buffer[0], "ls") == 0) {
        for(uint8_t w=0;w<sizeof(meter)/sizeof(TELE);w++){
            ret = snprintf(temp, sizeof(temp),"%i: %s\r\n", w+1, meter[w].name);
            send_buffer((uint8_t*)temp,ret,ptr);
        }
        return pdPASS;
    }
    
    
 	HELP_TEXT("Usage: telemetry list\r\n"
              "       telemetry ls\r\n"
              "       telemetry gauge n [name]\r\n"
              "       telemetry chart n [name]\r\n");
}
