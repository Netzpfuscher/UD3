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

#include <math.h>
#include <stdlib.h>
#include "qcw.h"
#include "interrupter.h"
#include "ntlibc.h"

#include "ZCDtoPWM.h"
#include "helper/teslaterm.h"
#include "tasks/tsk_overlay.h"

void qcw_handle(){
    if(SG_Timer_ReadCounter() < timer.time_stop){
        qcw_reg = 0;
        QCW_enable_Control = 0;
        return;
    }
    
    if (ramp.index < sizeof(ramp.data)) {
        qcw_modulate(ramp.data[ramp.index]);
        ramp.index++;
	}
}

void qcw_handle_synth(){
    if(SG_Timer_ReadCounter() < timer.time_stop){
        QCW_enable_Control = 0;
        return;
    }
}

void qcw_regenerate_ramp(){
    if(ramp.changed){
        float ramp_val=param.qcw_offset;
        
        uint32_t max_index = (configuration.max_qcw_pw*10)/125;   
        if (max_index > sizeof(ramp.data)) max_index = sizeof(ramp.data);
        
        float ramp_increment = param.qcw_ramp / 100.0;
      
        for(uint16_t i=0;i<max_index;i++){
            if(ramp_val>param.qcw_max)ramp_val=param.qcw_max;
            ramp.data[i]=floor(ramp_val);
            if(i>param.qcw_holdoff) ramp_val+=ramp_increment;
        }
        ramp.changed = pdFALSE;
    }
}

void qcw_ramp_point(uint16_t x,uint8_t y){
    if(x<sizeof(ramp.data)){
        ramp.data[x] = y;
    }
}

void qcw_ramp_line(uint16_t x0,uint8_t y0,uint16_t x1, uint8_t y1){
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2, e2;
    
	for (;;) {
		qcw_ramp_point(x0, y0);
		if (x0 == x1 && y0 == y1)
			break;
		e2 = err;
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		}
	}
}

#define TTERM_HEIGHT 255L
#define TTERM_WIDTH 400L

#define OFFSET_X 20
#define OFFSET_Y 20

void qcw_draw_d(port_str *ptr){
    uint16_t f;
    send_chart_line(OFFSET_X, TTERM_HEIGHT+OFFSET_Y, TTERM_WIDTH+OFFSET_X, TTERM_HEIGHT+OFFSET_Y, TT_COLOR_WHITE, ptr);
    send_chart_line(OFFSET_X, OFFSET_Y, OFFSET_X, TTERM_HEIGHT+OFFSET_Y, TT_COLOR_WHITE, ptr);

	for (f = 0; f <= TTERM_HEIGHT; f += 25) {
		send_chart_line(OFFSET_X, f+OFFSET_Y, OFFSET_X-10, f+OFFSET_Y, TT_COLOR_WHITE, ptr);
        send_chart_line(OFFSET_X, f+OFFSET_Y, TTERM_WIDTH+OFFSET_X, f+OFFSET_Y, TT_COLOR_GRAY, ptr);
	}

}

void qcw_ramp_visualize(port_str *ptr){
    for(uint16_t i = 0; i<sizeof(ramp.data)-1;i++){
        send_chart_line(OFFSET_X+i,TTERM_HEIGHT+OFFSET_Y-ramp.data[i],OFFSET_X+i+1,TTERM_HEIGHT+OFFSET_Y-ramp.data[i+1], TT_COLOR_GREEN ,ptr);
    }
}

void qcw_start(){
    timer.time_start = SG_Timer_ReadCounter();
    uint32_t sg_period = 3125; //ns
    uint32_t cycles_to_stop = (configuration.max_qcw_pw*10000)/sg_period;
    uint32_t cycles_since_last_pulse = timer.time_stop-timer.time_start;
    
    uint32_t cycles_to_stop_limited = (cycles_since_last_pulse * configuration.max_qcw_duty) / 500;
	if ((cycles_to_stop_limited > cycles_to_stop) || (cycles_to_stop_limited == 0)) {
		cycles_to_stop_limited = cycles_to_stop;
	}

    timer.time_stop = timer.time_start - cycles_to_stop_limited;
    ramp.index=0;
	//the next stuff is time sensitive, so disable interrupts to avoid glitches
	CyGlobalIntDisable;
	//now enable the QCW interrupter
	QCW_enable_Control = 1;
	params.pwmb_psb_val = params.pwm_top - params.pwmb_start_psb_val;
	CyGlobalIntEnable;
}

void qcw_modulate(uint16_t val){
#if USE_DEBUG_DAC  
    if(QCW_enable_Control) DEBUG_DAC_SetValue(val); 
#endif
    //linearize modulation value based on fb_filter_out period
	uint16_t shift_period = (val * (params.pwm_top - fb_filter_out)) >> 8;
	//assign new modulation value to the params.pwmb_psb_val ram
	if ((shift_period + params.pwmb_start_psb_val) > (params.pwmb_start_prd - 4)) {
		params.pwmb_psb_val = 4;
	} else {
		params.pwmb_psb_val = params.pwm_top - (shift_period + params.pwmb_start_psb_val);
	}  
}

void qcw_stop(){
    qcw_reg = 0;
#if USE_DEBUG_DAC 
    DEBUG_DAC_SetValue(0);
#endif
    QCW_enable_Control = 0;
}

uint8_t qcw_command_ramp(char *commandline, port_str *ptr){
    SKIP_SPACE(commandline);
    CHECK_NULL(commandline); 
    
    char *buffer[5];
    uint8_t items = split(commandline, buffer, sizeof(buffer)/sizeof(char*), ' ');
    
    
    if (ntlibc_stricmp(buffer[0], "point") == 0 && items == 3){
        int x = ntlibc_atoi(buffer[1]);
        int y = ntlibc_atoi(buffer[2]);
        qcw_ramp_point(x,y);
        return pdPASS;
        
    } else if (ntlibc_stricmp(buffer[0], "line") == 0 && items == 5) {
        int x0 = ntlibc_atoi(buffer[1]);
        int y0 = ntlibc_atoi(buffer[2]);
        int x1 = ntlibc_atoi(buffer[3]);
        int y1 = ntlibc_atoi(buffer[4]);
        qcw_ramp_line(x0,y0,x1,y1);
        return pdPASS;
        
    } else if (ntlibc_stricmp(buffer[0], "clear") == 0) {
        for(uint16_t i = 0; i<sizeof(ramp.data);i++){
            ramp.data[i] = 0;
        }
        return pdPASS;
    } else if (ntlibc_stricmp(buffer[0], "draw") == 0) {
        tsk_overlay_chart_stop();
        send_chart_clear(ptr);
        qcw_draw_d(ptr);
        qcw_ramp_visualize(ptr);
        return pdPASS;
    }
    
    
 	HELP_TEXT("Usage: ramp line x1 y1 x2 y2\r\n"
              "       ramp point x y\r\n"
              "       ramp clear\r\n"
              "       ramp draw\r\n");   
}