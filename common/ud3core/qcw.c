/**
 * @file qcw.c
 * @brief QCW (Quasi-Continuous Wave) mode implementation
 *
 * Implements long-pulse QCW operation with ramped current envelopes. Core features:
 * - Ramp generation with linear slope, holdoff, and frequency modulation overlay
 * - Real-time ramp playback at MIDI ISR rate (8kHz)
 * - Phase shift modulation linearized against feedback filter output
 * - Manual ramp editing (point/line drawing with Bresenham algorithm)
 * - Auto-repeat mode with FreeRTOS software timer
 * - Teslaterm visualization support
 *
 * Architecture:
 * - MIDI ISR calls qcw_handle() every 125µs to advance ramp playback
 * - Each sample modulates phase shift via qcw_modulate()
 * - Ramp generation uses floating-point math (safe, runs outside ISR)
 * - Safety limits enforced: max_qcw_pw, max_qcw_duty
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
#include "telemetry.h"

/** @brief FreeRTOS timer for QCW auto-repeat mode */
TimerHandle_t xQCW_Timer;

/**
 * @brief MIDI ISR callback - advance QCW ramp playback by one sample
 *
 * Called at MIDI ISR rate (8kHz). Reads current ramp sample, updates phase shift
 * modulation, and increments playback index. Stops pulse when reaching stop_index.
 *
 * @note Must execute quickly (within 125µs ISR budget)
 */
void qcw_handle(){
    if(ramp.index >= ramp.stop_index){
        qcw_modulate(0);
        QCW_enable_Control = 0;
        params.pwmb_psb_val = 0;
        ramp.index = 0;
    }else{
        qcw_modulate(ramp.data[ramp.index]);
        ramp.index++;
    }
}

/**
 * @brief Regenerate ramp envelope from current parameters
 *
 * Generates 400-sample current envelope with:
 * 1. Linear ramp from qcw_offset with slope qcw_ramp
 * 2. Holdoff period (qcw_holdoff samples at constant qcw_offset)
 * 3. Clamping to qcw_max limit (scaled down if qcw_vol would overflow)
 * 4. Optional frequency modulation overlay (qcw_vol amplitude at qcw_freq)
 *
 * Parameters used:
 * - qcw_offset: Starting current level (0-255)
 * - qcw_ramp: Ramp slope (units per 100 samples)
 * - qcw_max: Maximum current level (0-255)
 * - qcw_holdoff: Holdoff period in samples before ramping starts
 * - qcw_vol: Frequency modulation amplitude (0-255)
 * - qcw_freq: Modulation frequency in tenths of Hz
 * - qcw_pw: Pulse width in µs (determines stop_index)
 *
 * Only regenerates if ramp.changed flag is set. Clears flag when complete.
 *
 * @note Uses floating-point math - do not call from ISR
 */
void qcw_regenerate_ramp(){
    
    uint8_t toggle =0;
    float divider = (10.0f / (float)param.qcw_freq) / 0.00025f;  //Frequency in tenths
    uint32_t div = roundf(divider);
    
    uint32_t temp_max = param.qcw_max;
    
    if((temp_max + param.qcw_vol) > 255){
        temp_max = 255 - param.qcw_vol;  //Scale the max down to fit the volume
    }
    
    if(ramp.changed){
        float ramp_val = param.qcw_offset;

        uint16_t pw = param.qcw_pw;
        if (pw > configuration.max_qcw_pw) {
           pw = configuration.max_qcw_pw;
        }
        uint32_t max_active = (pw*10)/MIDI_ISR_US;
        if (max_active > sizeof(ramp.data)) max_active = sizeof(ramp.data);
        
        ramp.stop_index = max_active;
        
        float ramp_increment = param.qcw_ramp / 100.0;
      
        for(uint16_t i=0;i<max_active;i++){
            if(ramp_val > temp_max) ramp_val = temp_max;
            
            ramp.data[i]=floorf(ramp_val);
            if(i>param.qcw_holdoff){
                ramp_val += ramp_increment;
             
                if(param.qcw_vol > 0){
                    
			        if((i % div) == 0){
				        toggle = toggle == 0 ? 1 : 0;
			        }
			        if(toggle == 1){
                        uint32_t dat = ramp.data[i];
                        dat += param.qcw_vol;
                        if(dat > 255) dat = 255;
                        ramp.data[i] = dat;
			        }
                }   
            }
            
        }
        for (uint16_t i = max_active; i < QCW_RAMP_SAMPLES; ++i) {
           ramp.data[i] = 0;
        }
        ramp.changed = pdFALSE;
    }
}

/**
 * @brief Trigger QCW pulse from MIDI command with frequency modulation
 * @param volume Volume (unused in current implementation)
 * @param frequencyTenths Modulation frequency in tenths of Hz (e.g., 4400 = 440.0 Hz)
 *
 * Updates qcw_freq parameter and starts new pulse if no pulse is currently active.
 * Used for MIDI-controlled QCW mode (e.g., SYNTH_MIDI_QCW, SYNTH_SID_QCW).
 */
void qcw_cmd_midi_pulse(int32_t volume, int32_t frequencyTenths){
    param.qcw_freq = frequencyTenths;
    if(!QCW_enable_Control){
        ramp.changed = pdTRUE;  
        qcw_regenerate_ramp();
        qcw_start();
    }
}

/**
 * @brief Set single point in ramp array
 * @param x Sample index (0-399)
 * @param y Current value (0-255)
 *
 * Bounds-checked write to ramp data array. Used by manual ramp editing commands.
 */
void qcw_ramp_point(uint16_t x,uint8_t y){
    if(x<sizeof(ramp.data)){
        ramp.data[x] = y;
    }
}

/**
 * @brief Draw line in ramp array using Bresenham's line algorithm
 * @param x0 Start X coordinate (sample index, 0-399)
 * @param y0 Start Y value (current, 0-255)
 * @param x1 End X coordinate (sample index, 0-399)
 * @param y1 End Y value (current, 0-255)
 *
 * Draws anti-aliased line between two points in ramp array. Uses integer-only
 * Bresenham algorithm for efficiency. Calls qcw_ramp_point() for bounds checking.
 */
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


/**
 * @brief Visualize ramp waveform in Teslaterm graphical chart
 * @param chart Chart configuration (dimensions, offsets, divisions)
 * @param handle Terminal handle for output
 *
 * Draws three elements:
 * 1. Ramp envelope (green) - connects all 400 samples with lines
 * 2. Safety limit (red vertical line) - max_qcw_pw converted to sample index
 * 3. Current setting (blue vertical line) - qcw_pw converted to sample index
 *
 * Only works with Teslaterm protocol (not VT100 terminal).
 */
void qcw_ramp_visualize(CHART *chart, TERMINAL_HANDLE * handle){
    for(uint16_t i = 0; i<sizeof(ramp.data)-1;i++){
        send_chart_line(chart->offset_x+i,chart->height+chart->offset_y-ramp.data[i],chart->offset_x+i+1,chart->height+chart->offset_y-ramp.data[i+1], TT_COLOR_GREEN ,handle);
    }

    uint16_t red_line = configuration.max_qcw_pw*10 / MIDI_ISR_US;
    send_chart_line(chart->offset_x+red_line,chart->offset_y,chart->offset_x+red_line,chart->offset_y+chart->height, TT_COLOR_RED, handle);

    uint16_t blue_line = param.qcw_pw*10 / MIDI_ISR_US;
    send_chart_line(chart->offset_x+blue_line,chart->offset_y,chart->offset_x+blue_line,chart->offset_y+chart->height, TT_COLOR_BLUE, handle);
    
}

/**
 * @brief Start QCW pulse (enables hardware and initializes playback)
 *
 * Safety checks:
 * - Returns early if current duty cycle exceeds max_qcw_duty
 *
 * Atomic sequence (interrupts disabled):
 * 1. Reset ramp playback index to 0
 * 2. Enable QCW hardware control flag
 * 3. Set initial phase shift (pwmb_psb_val = pwm_top - pwmb_start_psb_val)
 *
 * @note Called by qcw command, MIDI handler, or auto-repeat timer
 */
void qcw_start(){
    if(tt.n.dutycycle.value > configuration.max_qcw_duty) return;  //Don't command a pulse if duty is too high
       
    ramp.index=0;
	//the next stuff is time sensitive, so disable interrupts to avoid glitches
	CyGlobalIntDisable;
	//now enable the QCW interrupter
	QCW_enable_Control = 1;
	params.pwmb_psb_val = params.pwm_top - params.pwmb_start_psb_val;
	CyGlobalIntEnable;
}

/**
 * @brief Modulate phase shift based on ramp sample value
 * @param val Modulation value (0-255, 0=minimum current, 255=maximum current)
 *
 * Linearizes modulation value against feedback filter output (fb_filter_out) to
 * maintain safe operating range. Phase shift modulation formula:
 * - shift_period = val * (pwm_top - fb_filter_out) / 256
 * - pwmb_psb_val = pwm_top - (shift_period + pwmb_start_psb_val)
 *
 * Safety clamp: If calculated value would cause period < 4, clamps to minimum (4).
 *
 * @note Called from MIDI ISR at 8kHz rate - must execute quickly
 */
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

/**
 * @brief Stop QCW pulse immediately
 *
 * Disables QCW hardware control flag and resets phase shift to zero.
 * Safe to call at any time (even if no pulse is active).
 */
void qcw_stop(){
    QCW_enable_Control = 0;
    params.pwmb_psb_val = 0;
}

/**
 * @brief Callback when QCW ramp parameters change
 * @param params Parameter array (unused)
 * @param index Parameter index (unused)
 * @param handle Terminal handle (unused)
 * @return pdPASS
 *
 * Sets ramp.changed flag to trigger regeneration before next pulse. If QCW is
 * not currently active, immediately regenerates ramp. If QCW is active, ramp
 * will regenerate before next pulse (prevents glitches during playback).
 *
 * Triggered by changes to: qcw_offset, qcw_ramp, qcw_max, qcw_holdoff, qcw_vol,
 * qcw_freq, qcw_pw.
 */
uint8_t callback_rampFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    ramp.changed = pdTRUE;
    if(!QCW_enable_Control){
        qcw_regenerate_ramp();
    }
    
    return pdPASS;
}



/**
 * @brief CLI command for manual ramp editing and visualization
 * @param handle Terminal handle for output
 * @param argCount Number of arguments
 * @param args Argument array
 * @return TERM_CMD_EXIT_SUCCESS
 *
 * Usage:
 * - `ramp point <x> <y>` - Set single point (x=0-399, y=0-255)
 * - `ramp line <x0> <y0> <x1> <y1>` - Draw line between two points
 * - `ramp clear` - Zero entire ramp array
 * - `ramp draw` - Visualize ramp in Teslaterm chart (Teslaterm only)
 *
 * Only available when is_qcw configuration flag is set. Point/line commands
 * allow manual waveform shaping beyond automatic ramp generation.
 */
uint8_t CMD_ramp(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf(   "Usage: ramp line x1 y1 x2 y2\r\n"
                    "       ramp point x y\r\n"
                    "       ramp clear\r\n"
                    "       ramp draw\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    } 
    if (!configuration.is_qcw) {
       ttprintf("Ramp control is only available for QCW coils\r\n");
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

/**
 * @brief FreeRTOS timer callback for QCW auto-repeat mode
 * @param xTimer Timer handle (unused)
 *
 * Called periodically by FreeRTOS timer at interval set by qcw_repeat parameter.
 * Regenerates ramp from current parameters and starts new pulse. Enforces minimum
 * repeat period of 100ms for safety.
 *
 * @note Runs in timer daemon task context - safe to call FreeRTOS APIs
 */
void vQCW_Timer_Callback(TimerHandle_t xTimer){
    qcw_regenerate_ramp();
    qcw_start();
    if(param.qcw_repeat<100) param.qcw_repeat = 100;
    xTimerChangePeriod( xTimer, param.qcw_repeat / portTICK_PERIOD_MS, 0 );
}

/**
 * @brief Delete QCW auto-repeat timer
 * @return pdPASS if timer deleted successfully, pdFAIL if timer NULL or delete failed
 *
 * Stops automatic QCW pulse repetition by deleting FreeRTOS timer. Waits up to
 * 200ms for timer deletion to complete. Sets xQCW_Timer to NULL on success.
 *
 * Safe to call even if timer doesn't exist (returns pdFAIL).
 */
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

/**
 * @brief CLI command to start/stop QCW mode
 * @param handle Terminal handle for output
 * @param argCount Number of arguments (expects 1)
 * @param args Argument array: args[0] = "start" or "stop"
 * @return TERM_CMD_EXIT_SUCCESS
 *
 * Usage:
 * - `qcw start` - Start QCW mode
 *   - If qcw_repeat >= 100ms: Creates auto-repeat timer with period = qcw_repeat
 *   - If qcw_repeat < 100ms: Single-shot pulse
 * - `qcw stop` - Stop QCW mode and delete auto-repeat timer
 *
 * Only available when is_qcw configuration flag is set. Auto-repeat mode uses
 * FreeRTOS software timer (vQCW_Timer_Callback) for periodic pulse generation.
 *
 * @note Timer is one-shot (pdFALSE) but period updated in callback for flexibility
 */
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
