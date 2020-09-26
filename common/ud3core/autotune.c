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

#include <device.h>
#include <math.h>
#include "helper/printf.h"

#include "ZCDtoPWM.h"
#include "autotune.h"
#include "helper/brailledraw.h"
#include "helper/teslaterm.h"
#include "cli_common.h"
#include "interrupter.h"
#include "tasks/tsk_analog.h"
#include "tasks/tsk_overlay.h"
#include "ntlibc.h"

#define FREQ 0
#define CURR 1

#define TTERM_HEIGHT 300L
#define TTERM_WIDTH 400L

#define OFFSET_X 20
#define OFFSET_Y 20

uint16_t run_adc_sweep(uint16_t F_min, uint16_t F_max, uint16_t pulsewidth, uint8_t channel, uint8_t delay, port_str *ptr);


/*****************************************************************************
* commands a frequency sweep for the primary coil. It searches for a peak
* and makes a second run with +-6kHz around the peak
******************************************************************************/
uint8_t command_tune(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    CHECK_NULL(commandline);
    
    
    SEND_CONST_STRING("Warning: The bridge will switch hard, make sure that the bus voltage is appropriate\r\n",ptr);
    SEND_CONST_STRING("Press [y] to proceed\r\n",ptr);
    
    if(getch(ptr, portMAX_DELAY) != 'y'){
        SEND_CONST_STRING("Tune aborted\r\n",ptr);
        return 0;
    }
    if(ptr->term_mode == PORT_TERM_TT){
        tsk_overlay_chart_stop();
        send_chart_clear(ptr);
        
    }
    SEND_CONST_STRING("Start sweep:\r\n",ptr);
    if (ntlibc_stricmp(commandline, "prim") == 0) {
    	run_adc_sweep(param.tune_start, param.tune_end, param.tune_pw, CT_PRIMARY, param.tune_delay, ptr);
        SEND_CONST_STRING("Type cls to go back to normal telemtry chart\r\n",ptr);
        return 0;
    }else if (ntlibc_stricmp(commandline, "sec") == 0) {
        run_adc_sweep(param.tune_start, param.tune_end, param.tune_pw, CT_SECONDARY, param.tune_delay, ptr);
        SEND_CONST_STRING("Type cls to go back to normal telemetry chart\r\n",ptr);
        return 0;
    }

    HELP_TEXT("Usage: tune [prim|sec]\r\n");
}

void autotune_draw_d(port_str *ptr){
    uint16_t f;
    send_chart_line(OFFSET_X, TTERM_HEIGHT+OFFSET_Y, TTERM_WIDTH+OFFSET_X, TTERM_HEIGHT+OFFSET_Y, TT_COLOR_WHITE, ptr);
    send_chart_line(OFFSET_X, OFFSET_Y, OFFSET_X, TTERM_HEIGHT+OFFSET_Y, TT_COLOR_WHITE, ptr);

	for (f = 0; f <= TTERM_HEIGHT; f += 25) {
		send_chart_line(OFFSET_X, f+OFFSET_Y, OFFSET_X-10, f+OFFSET_Y, TT_COLOR_WHITE, ptr);
        send_chart_line(OFFSET_X, f+OFFSET_Y, TTERM_WIDTH+OFFSET_X, f+OFFSET_Y, TT_COLOR_GRAY, ptr);
	}

}
#define COLS 2
#define ROWS 128

typedef struct freq_struct freq_str;
struct freq_struct {
    uint16_t curr[ROWS];
    uint16_t freq[ROWS];
};

uint16_t run_adc_sweep(uint16_t F_min, uint16_t F_max, uint16_t pulsewidth, uint8_t channel, uint8_t delay, port_str *ptr) {

    freq_str *freq_resp;
    freq_resp = pvPortMalloc(sizeof(freq_str));
    if(freq_resp==NULL){
        SEND_CONST_STRING("Malloc failed\r\n", ptr);
        return 0;
    }

	CT_MUX_Select(channel);
	//units for frequency are 0.1kHz (so 1000 = 100khz).  Pulsewidth in uS
	uint16 f;
	uint8 n;
	uint16 original_freq;
	uint8 original_lock_cycles;
	char buffer[100];
    int ret=0;
	float current_buffer = 0;

	

	if (F_min > F_max) {
		return 0;
	}
	//store the original setting, restore it after the sweep
	original_freq = configuration.start_freq;
	original_lock_cycles = configuration.start_cycles;

	//max_response = 0;
	for (f = 0; f < ROWS; f++) {
		freq_resp->curr[f] = 0;
	}

	//create frequency table based on F_max, F_min, 128 values, [f],[0] holds frequency, [f],[1] holds amplitude response

	for (f = 0; f < ROWS; f++) {
		current_buffer = 0;
		//this generates the requested trial frequency for the table entry, starting at Fmin and working our way up to Fmax
		freq_resp->freq[f] = (((F_max - F_min) * (f + 1)) >> 7) + F_min;

		configuration.start_freq = freq_resp->freq[f];
		configuration.start_cycles = 120;
		configure_ZCD_to_PWM();

		for (n = 0; n < configuration.autotune_s; n++) {

			//command a one shot

			interrupter_oneshot(pulsewidth, 127);

			vTaskDelay(delay / portTICK_PERIOD_MS);

			current_buffer += CT1_Get_Current_f(channel);
		}
		freq_resp->curr[f] = round(current_buffer / (float)configuration.autotune_s * 10);
		ret = snprintf(buffer, sizeof(buffer), "Frequency: %i00Hz Current: %4i,%iA     \r", freq_resp->freq[f], freq_resp->curr[f] / 10, freq_resp->curr[f] % 10);
		send_buffer((uint8_t*)buffer, ret, ptr);
	}
	SEND_CONST_STRING("\r\n", ptr);

	//restore original frequency
	configuration.start_freq = original_freq;
	configuration.start_cycles = original_lock_cycles;
	configure_ZCD_to_PWM();

	CT_MUX_Select(CT_PRIMARY);

	//search max current
	uint16_t max_curr = 0;
	uint8_t max_curr_num = 0;
	for (f = 0; f < ROWS; f++) {
		if (freq_resp->curr[f] > max_curr) {
			max_curr = freq_resp->curr[f];
			max_curr_num = f;
		}
	}

	//Draw Diagram

    if(ptr->term_mode == PORT_TERM_VT100){
        braille_malloc(ptr);
        braille_clear();
	    braille_line(0, 63, 127, 63);
	    braille_line(0, 0, 0, 63);

    	for (f = 0; f < PIX_HEIGHT; f += 4) {
    		braille_line(0, f, 2, f);
    	}
    	for (f = 0; f < PIX_WIDTH; f += 4) {
    		braille_line(f, PIX_HEIGHT - 1, f, PIX_HEIGHT - 3);
    	}

    	for (f = 1; f < ROWS; f++) {
    		braille_line(f - 1, ((PIX_HEIGHT - 1) - ((PIX_HEIGHT - 1) * freq_resp->curr[f-1]) / max_curr), f, ((PIX_HEIGHT - 1) - ((PIX_HEIGHT - 1) * freq_resp->curr[f]) / max_curr));
    	}
    	braille_draw(ptr);
    	for (f = 0; f < PIX_WIDTH / 2; f += 4) {
    		if (!f) {
    			ret = snprintf(buffer, sizeof(buffer), "%i", freq_resp->freq[f*2]);
    			send_buffer((uint8_t*)buffer, ret, ptr);
    		} else {
    			ret = snprintf(buffer, sizeof(buffer), "%4i", freq_resp->freq[f*2]);
    			send_buffer((uint8_t*)buffer, ret, ptr);
    		}
    	}
        braille_free(ptr);
    }else{

        float step_w = (float)TTERM_WIDTH/128;
        
        float x_val_1;
        float x_val_2;
        autotune_draw_d(ptr);
        
        //Draw x grid
        for (f = 0; f < ROWS; f+=8) {
            x_val_1 = f*step_w;
            send_chart_line(x_val_1+OFFSET_X, OFFSET_Y, x_val_1+OFFSET_X, TTERM_HEIGHT+OFFSET_Y, TT_COLOR_GRAY, ptr);
    	}
        f=ROWS-1;
        x_val_1 = f*step_w;
        send_chart_line(x_val_1+OFFSET_X, OFFSET_Y, x_val_1+OFFSET_X, TTERM_HEIGHT+OFFSET_Y, TT_COLOR_GRAY, ptr);
        
        //Draw line
    	for (f = 0; f < ROWS-1; f++) {
            x_val_1 = f*step_w;
            x_val_2 = (f+1)*step_w;
    		send_chart_line(x_val_1+OFFSET_X, ((TTERM_HEIGHT - 1) - ((TTERM_HEIGHT - 1) * freq_resp->curr[f]) / max_curr)+OFFSET_Y, x_val_2+OFFSET_X, ((TTERM_HEIGHT - 1) - ((TTERM_HEIGHT - 1) * freq_resp->curr[f+1]) / max_curr)+OFFSET_Y, TT_COLOR_GREEN, ptr);
    	}
        
        //Draw text
    	for (f = 0; f < ROWS; f+=8) {
            x_val_1 = f*step_w;
            snprintf(buffer, sizeof(buffer), "%i,%i", freq_resp->freq[f]/10,freq_resp->freq[f]%10);
    		send_chart_text_center(x_val_1+OFFSET_X,OFFSET_Y+TTERM_HEIGHT+20,TT_COLOR_WHITE,8,buffer,ptr);
            send_chart_line(x_val_1+OFFSET_X, TTERM_HEIGHT+OFFSET_Y, x_val_1+OFFSET_X, TTERM_HEIGHT +10+OFFSET_Y, TT_COLOR_WHITE, ptr);
    	}
        f=127;
        x_val_1 = f*step_w;
        snprintf(buffer, sizeof(buffer), "%i,%i", freq_resp->freq[f]/10,freq_resp->freq[f]%10);
    	send_chart_text_center(x_val_1+OFFSET_X,OFFSET_Y+TTERM_HEIGHT+20,TT_COLOR_WHITE,8,buffer,ptr);
        send_chart_line(x_val_1+OFFSET_X, TTERM_HEIGHT+OFFSET_Y, x_val_1+OFFSET_X, TTERM_HEIGHT +10+OFFSET_Y, TT_COLOR_WHITE, ptr);
        x_val_1+=15;
        send_chart_text(x_val_1+OFFSET_X,OFFSET_Y+TTERM_HEIGHT+20,TT_COLOR_WHITE,8,"khz",ptr);

    }
    
	ret = snprintf(buffer, sizeof(buffer), "\r\nFound Peak at: %i00Hz\r\n", freq_resp->freq[max_curr_num]);
	send_buffer((uint8_t*)buffer, ret, ptr);
    uint16_t temp = freq_resp->freq[max_curr_num];
    vPortFree(freq_resp);
    return temp;
}

/* [] END OF FILE */
