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
#include "hardware.h"
#include "SignalGenerator.h"

#include "ZCDtoPWM.h"
#include "helper/teslaterm.h"
#include "tasks/tsk_overlay.h"
#include "tasks/tsk_midi.h"

TimerHandle_t xQCW_Timer;

void qcw_handle(){
    if(SG_Timer_ReadCounter() < timer.time_stop){
        qcw_modulate(0);
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
    static uint8_t toggle =0;
    
    float divider = (1.0f / (float)param.qcw_freq) / 0.00025f;
    uint32_t div = roundf(divider);
    
    if(ramp.changed){
        float ramp_val=param.qcw_offset;
        
        uint32_t max_index = (configuration.max_qcw_pw*10)/MIDI_ISR_US;   
        if (max_index > sizeof(ramp.data)) max_index = sizeof(ramp.data);
        
        float ramp_increment = param.qcw_ramp / 100.0;
      
        for(uint16_t i=0;i<max_index;i++){
            if(ramp_val>param.qcw_max)ramp_val=param.qcw_max;
            ramp.data[i]=floor(ramp_val);
            if(i>param.qcw_holdoff){
                ramp_val+=ramp_increment;
             
                if(param.qcw_vol > 0){
                    
			if((i % div) == 0){
				toggle = toggle == 0 ? 1 : 0;
			}
			if(toggle == 1){
                        toggle = 1;
                        uint32_t dat = ramp.data[i];
                        dat += param.qcw_vol;
                        if(dat > 255) dat = 255;
                        ramp.data[i] = dat;
			}
                }   
            }
            
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


void qcw_ramp_visualize(CHART *chart, TERMINAL_HANDLE * handle){
    for(uint16_t i = 0; i<sizeof(ramp.data)-1;i++){
        send_chart_line(chart->offset_x+i,chart->height+chart->offset_y-ramp.data[i],chart->offset_x+i+1,chart->height+chart->offset_y-ramp.data[i+1], TT_COLOR_GREEN ,handle);
    }
    uint16_t red_line;
    red_line = configuration.max_qcw_pw*10 / MIDI_ISR_US;
    
    send_chart_line(chart->offset_x+red_line,chart->offset_y,chart->offset_x+red_line,chart->offset_y+chart->height, TT_COLOR_RED, handle);
    
}

void qcw_start(){
    timer.time_start = SG_Timer_ReadCounter();
    uint32_t sg_period = SG_PERIOD_NS;
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
    QCW_enable_Control = 0;
}

uint8_t callback_rampFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    ramp.changed = pdTRUE;
    if(!QCW_enable_Control){
        qcw_regenerate_ramp();
    }
    
    return pdPASS;
}



uint8_t CMD_ramp(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf(   "Usage: ramp line x1 y1 x2 y2\r\n"
                    "       ramp point x y\r\n"
                    "       ramp clear\r\n"
                    "       ramp draw\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    } 
    
  
    if(strcmp(args[0], "point") == 0 && argCount == 3){
        int x = atoi(args[1]);
        int y = atoi(args[2]);
        qcw_ramp_point(x,y);
        return TERM_CMD_EXIT_SUCCESS;
        
    } else if(strcmp(args[0], "line") == 0 && argCount == 5){
        int x0 = atoi(args[1]);
        int y0 = atoi(args[2]);
        int x1 = atoi(args[3]);
        int y1 = atoi(args[4]);
        qcw_ramp_line(x0,y0,x1,y1);
        return TERM_CMD_EXIT_SUCCESS;
        
    } else if(strcmp(args[0], "clear") == 0){
        for(uint16_t i = 0; i<sizeof(ramp.data);i++){
            ramp.data[i] = 0;
        }
        return TERM_CMD_EXIT_SUCCESS;
    } else if(strcmp(args[0], "draw") == 0){
        port_str * ptr = handle->port;
        if(ptr->term_mode == PORT_TERM_VT100){
            ttprintf("Command only available with Teslaterm\r\n");
            return TERM_CMD_EXIT_SUCCESS;
        }
        tsk_overlay_chart_stop();
        send_chart_clear(handle, "QCW ramp");
        CHART temp;
        temp.height = RAMP_CHART_HEIGHT;
        temp.width = RAMP_CHART_WIDTH;
        temp.offset_x = RAMP_CHART_OFFSET_X;
        temp.offset_y = RAMP_CHART_OFFSET_Y;
        temp.div_x = RAMP_CHART_DIV_X;
        temp.div_y = RAMP_CHART_DIV_Y;
        
        tt_chart_init(&temp,handle);
        qcw_ramp_visualize(&temp,handle);
        return TERM_CMD_EXIT_SUCCESS;
    }
     return TERM_CMD_EXIT_SUCCESS;
}

/*****************************************************************************
* Timer callback for the QCW autofire
******************************************************************************/
void vQCW_Timer_Callback(TimerHandle_t xTimer){
    qcw_regenerate_ramp();
    qcw_start();
    qcw_reg = 1;
    if(param.qcw_repeat<100) param.qcw_repeat = 100;
    xTimerChangePeriod( xTimer, param.qcw_repeat / portTICK_PERIOD_MS, 0 );
}

BaseType_t QCW_delete_timer(void){
    if (xQCW_Timer != NULL) {
    	if(xTimerDelete(xQCW_Timer, 200 / portTICK_PERIOD_MS) != pdFALSE){
            xQCW_Timer = NULL;
            return pdPASS;
        }else{
            return pdFAIL;
        }
    }else{
        return pdFAIL;
    }
}

/*****************************************************************************
* starts the QCW mode. Spawns a timer for the automatic QCW pulses.
******************************************************************************/
uint8_t CMD_qcw(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf("Usage: qcw [start|stop]\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    }
    
    if(configuration.is_qcw == pdFALSE){
        ttprintf("This is not a QCW coil. Set [qcw_coil] to 1.\r\n");  
        return TERM_CMD_EXIT_SUCCESS;
    }
    
	if(strcmp(args[0], "start") == 0){
        if(param.qcw_repeat>99){
            if(xQCW_Timer==NULL){
                xQCW_Timer = xTimerCreate("QCW-Tmr", param.qcw_repeat / portTICK_PERIOD_MS, pdFALSE,(void * ) 0, vQCW_Timer_Callback);
                if(xQCW_Timer != NULL){
                    xTimerStart(xQCW_Timer, 0);
                    ttprintf("QCW Enabled\r\n");
                }else{
                    ttprintf("Cannot create QCW Timer\r\n");
                }
            }
        }else{
            qcw_regenerate_ramp();
		    qcw_start();
            qcw_reg = 1;
            ttprintf("QCW single shot\r\n");
        }
		
		return TERM_CMD_EXIT_SUCCESS;
	}
	if(strcmp(args[0], "stop") == 0){
        if (xQCW_Timer != NULL) {
				if(!QCW_delete_timer()){
                    ttprintf("Cannot delete QCW Timer\r\n");
                }
		}
        qcw_stop();
		ttprintf("QCW Disabled\r\n");
		return TERM_CMD_EXIT_SUCCESS;
	}
	return TERM_CMD_EXIT_SUCCESS;
}
