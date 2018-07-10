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

#include "cli_common.h"
#include "ZCDtoPWM.h"
#include "autotune.h"
#include "charging.h"
#include "interrupter.h"
#include "ntshell.h"
#include "telemetry.h"
#include <math.h>
#include <project.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cyapicallbacks.h"
#include <cytypes.h>



#include "tasks/tsk_analog.h"
#include "tasks/tsk_overlay.h"
#include "tasks/tsk_priority.h"
#include "tasks/tsk_uart.h"
#include "tasks/tsk_usb.h"
#include "helper/teslaterm.h"

#define UNUSED_VARIABLE(N) \
	do {                   \
		(void)(N);         \
	} while (0)
        
#define EEPROM_READ_BYTE(x) EEPROM_1_ReadByte(x)
#define EEPROM_WRITE_ROW(x,y) EEPROM_1_Write(y,x)

#define SKIP_SPACE(ptr) 	if (*ptr == 0x20 && ptr != 0) ptr++ //skip space
#define HELP_TEXT(text)        \
	if (*commandline == 0 || commandline == 0) {    \
		Term_Color_Red(port);       \
		send_string(text, port);  \
		Term_Color_White(port);     \
		return 1;}                  \
	



typedef struct {
	const char *text;
	uint8_t (*commandFunction)(char *commandline, uint8_t port);
	const char *help;
} command_entry;

uint8_t callback_ConfigFunction(parameter_entry * params, uint8_t index, uint8_t port);
uint8_t callback_DefaultFunction(parameter_entry * params, uint8_t index, uint8_t port);
uint8_t callback_TuneFunction(parameter_entry * params, uint8_t index, uint8_t port);
uint8_t callback_TRFunction(parameter_entry * params, uint8_t index, uint8_t port);
uint8_t callback_OfftimeFunction(parameter_entry * params, uint8_t index, uint8_t port);

uint8_t commandHelp(char *commandline, uint8_t port);
uint8_t commandGet(char *commandline, uint8_t port);
uint8_t commandSet(char *commandline, uint8_t port);
uint8_t command_tr(char *commandline, uint8_t port);
uint8_t command_udkill(char *commandline, uint8_t port);
uint8_t command_eprom(char *commandline, uint8_t port);
uint8_t command_status(char *commandline, uint8_t port);
uint8_t command_cls(char *commandline, uint8_t port);
uint8_t command_tune_p(char *commandline, uint8_t port);
uint8_t command_tune_s(char *commandline, uint8_t port);
uint8_t command_tasks(char *commandline, uint8_t port);
uint8_t command_bootloader(char *commandline, uint8_t port);
uint8_t command_qcw(char *commandline, uint8_t port);
uint8_t command_bus(char *commandline, uint8_t port);
uint8_t command_load_default(char *commandline, uint8_t port);
uint8_t command_tterm(char *commandline, uint8_t port);
uint8_t command_reset(char *commandline, uint8_t port);
uint8_t command_minstat(char *commandline, uint8_t port);

void nt_interpret(const char *text, uint8_t port);

const uint8_t kill_msg[3] = {0xb0, 0x77, 0x00};

uint8_t term_mode = TERM_MODE_VT100;

uint8_t tr_running = 0;
uint8_t burst_state = 0;
TimerHandle_t xQCW_Timer;
TimerHandle_t xBurst_Timer;

cli_config configuration;
cli_parameter param;

void init_config(){
    configuration.watchdog = 1;
    configuration.max_tr_pw = 1000;
    configuration.max_tr_prf = 800;
    configuration.max_qcw_pw = 10000;
    configuration.max_tr_current = 400;
    configuration.min_tr_current = 100;
    configuration.max_qcw_current = 300;
    configuration.temp1_max = 40;
    configuration.temp2_max = 40;
    configuration.ct1_ratio = 600;
    configuration.ct2_ratio = 1000;
    configuration.ct3_ratio = 30;
    configuration.ct1_burden = 33;
    configuration.ct2_burden = 500;
    configuration.ct3_burden = 33;
    configuration.lead_time = 200;
    configuration.start_freq = 630;
    configuration.start_cycles = 3;
    configuration.max_tr_duty = 100;
    configuration.max_qcw_duty = 350;
    configuration.temp1_setpoint = 30;
    configuration.ext_trig_enable = 1;
    configuration.batt_lockout_v = 360;
    configuration.slr_fswitch = 500;
    configuration.slr_vbus = 200;
    configuration.ps_scheme = 2;
    configuration.autotune_s = 1;
    strcpy(configuration.ud_name,"UD3-Tesla");
    strcpy(configuration.ip_addr,"0.0.0.0");
    
    param.pw = 0;
    param.pwd = 50000;
    param.burst_on = 0;
    param.burst_off = 500;
    param.tune_start = 400;
    param.tune_end = 1000;
    param.tune_pw = 50;
    param.tune_delay = 50;
    param.offtime = 2;
    param.qcw_ramp = 2;
    param.qcw_repeat = 500;
    param.transpose = 0;
}

// clang-format off
parameter_entry confparam[] = {
    //        Parameter Type,"Text   "         , Value ptr                     , Type          ,Min    ,Max    ,Callback Function           ,Help text
    ADD_PARAM(PARAM_DEFAULT ,"pw"              , param.pw                      , TYPE_UNSIGNED ,0      ,800    ,callback_TRFunction         ,"Pulsewidth")
    ADD_PARAM(PARAM_DEFAULT ,"pwd"             , param.pwd                     , TYPE_UNSIGNED ,0      ,60000  ,callback_TRFunction         ,"Pulsewidthdelay")
    ADD_PARAM(PARAM_DEFAULT ,"bon"             , param.burst_on                , TYPE_UNSIGNED ,0      ,1000   ,callback_TRFunction         ,"Burst mode ontime [ms] 0=off")
    ADD_PARAM(PARAM_DEFAULT ,"boff"            , param.burst_off               , TYPE_UNSIGNED ,0      ,1000   ,callback_TRFunction         ,"Burst mode offtime [ms]")
    ADD_PARAM(PARAM_DEFAULT ,"tune_start"      , param.tune_start              , TYPE_UNSIGNED ,5      ,1000   ,callback_TuneFunction       ,"Start frequency")
    ADD_PARAM(PARAM_DEFAULT ,"tune_end"        , param.tune_end                , TYPE_UNSIGNED ,5      ,1000   ,callback_TuneFunction       ,"End frequency")
    ADD_PARAM(PARAM_DEFAULT ,"tune_pw"         , param.tune_pw                 , TYPE_UNSIGNED ,0      ,800    ,NULL                        ,"Tune pulsewidth")
    ADD_PARAM(PARAM_DEFAULT ,"tune_delay"      , param.tune_delay              , TYPE_UNSIGNED ,1      ,200    ,NULL                        ,"Tune delay")
    ADD_PARAM(PARAM_CONFIG  ,"offtime"         , param.offtime                 , TYPE_UNSIGNED ,2      ,250    ,callback_OfftimeFunction    ,"Offtime for MIDI")
    ADD_PARAM(PARAM_DEFAULT ,"qcw_ramp"        , param.qcw_ramp                , TYPE_UNSIGNED ,1      ,10     ,NULL                        ,"QCW Ramp Inc/93us")
    ADD_PARAM(PARAM_DEFAULT ,"qcw_repeat"      , param.qcw_repeat              , TYPE_UNSIGNED ,0      ,1000   ,NULL                        ,"QCW pulse repeat time [ms] <100=single shot")
    ADD_PARAM(PARAM_DEFAULT ,"transpose"       , param.transpose               , TYPE_SIGNED   ,-48    ,48     ,NULL                        ,"Transpose MIDI")
    ADD_PARAM(PARAM_CONFIG  ,"watchdog"        , configuration.watchdog        , TYPE_UNSIGNED ,0      ,1      ,NULL                        ,"Watchdog Enable")
    ADD_PARAM(PARAM_CONFIG  ,"max_tr_pw"       , configuration.max_tr_pw       , TYPE_UNSIGNED ,0      ,3000   ,callback_ConfigFunction     ,"Maximum TR PW [uSec]")
    ADD_PARAM(PARAM_CONFIG  ,"max_tr_prf"      , configuration.max_tr_prf      , TYPE_UNSIGNED ,0      ,3000   ,callback_ConfigFunction     ,"Maximum TR frequency [Hz]")
    ADD_PARAM(PARAM_CONFIG  ,"max_qcw_pw"      , configuration.max_qcw_pw      , TYPE_UNSIGNED ,0      ,30000  ,callback_ConfigFunction     ,"Maximum QCW PW [uSec*10]")
    ADD_PARAM(PARAM_CONFIG  ,"max_tr_current"  , configuration.max_tr_current  , TYPE_UNSIGNED ,0      ,1000   ,callback_ConfigFunction     ,"Maximum TR current [A]")
    ADD_PARAM(PARAM_CONFIG  ,"min_tr_current"  , configuration.min_tr_current  , TYPE_UNSIGNED ,0      ,1000   ,callback_ConfigFunction     ,"Minimum TR current [A]")
    ADD_PARAM(PARAM_CONFIG  ,"max_qcw_current" , configuration.max_qcw_current , TYPE_UNSIGNED ,0      ,1000   ,callback_ConfigFunction     ,"Maximum QCW current [A]")
    ADD_PARAM(PARAM_CONFIG  ,"temp1_max"       , configuration.temp1_max       , TYPE_UNSIGNED ,0      ,100    ,NULL                        ,"Max temperature 1 [*C]")
    ADD_PARAM(PARAM_CONFIG  ,"temp2_max"       , configuration.temp2_max       , TYPE_UNSIGNED ,0      ,100    ,NULL                        ,"Max temperature 2 [*C]")
    ADD_PARAM(PARAM_CONFIG  ,"ct1_ratio"       , configuration.ct1_ratio       , TYPE_UNSIGNED ,1      ,2000   ,callback_ConfigFunction     ,"CT1 [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,"ct2_ratio"       , configuration.ct2_ratio       , TYPE_UNSIGNED ,1      ,2000   ,callback_ConfigFunction     ,"CT2 [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,"ct3_ratio"       , configuration.ct3_ratio       , TYPE_UNSIGNED ,1      ,2000   ,callback_ConfigFunction     ,"CT3 [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,"ct1_burden"      , configuration.ct1_burden      , TYPE_UNSIGNED ,1      ,1000   ,callback_ConfigFunction     ,"CT1 burden [deciOhm]")
    ADD_PARAM(PARAM_CONFIG  ,"ct2_burden"      , configuration.ct2_burden      , TYPE_UNSIGNED ,1      ,1000   ,callback_ConfigFunction     ,"CT2 burden [deciOhm]")
    ADD_PARAM(PARAM_CONFIG  ,"ct3_burden"      , configuration.ct3_burden      , TYPE_UNSIGNED ,1      ,1000   ,callback_ConfigFunction     ,"CT3 burden [deciOhm]")
    ADD_PARAM(PARAM_CONFIG  ,"lead_time"       , configuration.lead_time       , TYPE_UNSIGNED ,0      ,2000   ,callback_ConfigFunction     ,"Lead time [nSec]")
    ADD_PARAM(PARAM_CONFIG  ,"start_freq"      , configuration.start_freq      , TYPE_UNSIGNED ,0      ,5000   ,callback_ConfigFunction     ,"Resonant freq [Hz*100]")
    ADD_PARAM(PARAM_CONFIG  ,"start_cycles"    , configuration.start_cycles    , TYPE_UNSIGNED ,0      ,20     ,callback_ConfigFunction     ,"Start Cyles [N]")
    ADD_PARAM(PARAM_CONFIG  ,"max_tr_duty"     , configuration.max_tr_duty     , TYPE_UNSIGNED ,1      ,500    ,callback_ConfigFunction     ,"Max TR duty cycle [0.1% incr]")
    ADD_PARAM(PARAM_CONFIG  ,"max_qcw_duty"    , configuration.max_qcw_duty    , TYPE_UNSIGNED ,1      ,500    ,callback_ConfigFunction     ,"Max QCW duty cycle [0.1% incr]")
    ADD_PARAM(PARAM_CONFIG  ,"temp1_setpoint"  , configuration.temp1_setpoint  , TYPE_UNSIGNED ,0      ,100    ,NULL                        ,"Setpoint for fan [*C]")
    ADD_PARAM(PARAM_CONFIG  ,"batt_lockout_v"  , configuration.batt_lockout_v  , TYPE_UNSIGNED ,0      ,500    ,NULL                        ,"Battery lockout voltage [V]")
    ADD_PARAM(PARAM_CONFIG  ,"slr_fswitch"     , configuration.slr_fswitch     , TYPE_UNSIGNED ,0      ,1000   ,callback_ConfigFunction     ,"SLR switch frequency [Hz*100]")
    ADD_PARAM(PARAM_CONFIG  ,"slr_vbus"        , configuration.slr_vbus        , TYPE_UNSIGNED ,0      ,1000   ,NULL                        ,"SLR Vbus [V]")
    ADD_PARAM(PARAM_CONFIG  ,"ps_scheme"       , configuration.ps_scheme       , TYPE_UNSIGNED ,0      ,4      ,callback_ConfigFunction     ,"Power supply sheme")
    ADD_PARAM(PARAM_CONFIG  ,"autotune_s"      , configuration.autotune_s      , TYPE_UNSIGNED ,1      ,32     ,NULL                        ,"Number of samples for Autotune")
    ADD_PARAM(PARAM_CONFIG  ,"ud_name"         , configuration.ud_name         , TYPE_STRING   ,0      ,0      ,NULL                        ,"Name of the Coil [15 chars]")
    ADD_PARAM(PARAM_DEFAULT ,"ip_addr"         , configuration.ip_addr         , TYPE_STRING   ,0      ,0      ,NULL                        ,"IP-Adress of the ESP8266")
};

command_entry commands[] = {
    ADD_COMMAND("bootloader"    ,command_bootloader     ,"Start bootloader")
    ADD_COMMAND("bus"           ,command_bus            ,"Bus [on/off]")
    ADD_COMMAND("cls"		    ,command_cls            ,"Clear screen")
    ADD_COMMAND("eeprom"	    ,command_eprom          ,"Save/Load config [load/save]")
	ADD_COMMAND("get"		    ,commandGet             ,"Usage get [param]")
    ADD_COMMAND("help"          ,commandHelp            ,"This text")
    ADD_COMMAND("kill"		    ,command_udkill         ,"Kills all UD Coils") 
    ADD_COMMAND("load_default"  ,command_load_default   ,"Loads the default parameters")
    ADD_COMMAND("qcw"           ,command_qcw            ,"QCW [start/stop]")
    ADD_COMMAND("reset"         ,command_reset          ,"Resets UD3")
	ADD_COMMAND("set"		    ,commandSet             ,"Usage set [param] [value]")
    ADD_COMMAND("status"	    ,command_status         ,"Displays coil status")
    ADD_COMMAND("tasks"	        ,command_tasks          ,"Show running Tasks")
    ADD_COMMAND("tr"		    ,command_tr             ,"Transient [start/stop]")
    ADD_COMMAND("tune_p"	    ,command_tune_p         ,"Autotune Primary")
    ADD_COMMAND("tune_s"	    ,command_tune_s         ,"Autotune Secondary")        
    ADD_COMMAND("tterm"	        ,command_tterm          ,"Changes terminal mode")
    ADD_COMMAND("minstat"	    ,command_minstat        ,"Prints the min statistics")
};
// clang-format on
    
uint8_t callback_OfftimeFunction(parameter_entry * params, uint8_t index, uint8_t port){
    Offtime_WritePeriod(param.offtime);
    return 1;
}

uint8_t callback_TuneFunction(parameter_entry * params, uint8_t index, uint8_t port) {
    if(param.tune_start >= param.tune_end){
        send_string("ERROR: tune_start > tune_end", port);
		return 0;
	} else
		return 1;
}



uint8_t callback_TRFunction(parameter_entry * params, uint8_t index, uint8_t port) {
	interrupter.pw = param.pw;
	interrupter.prd = param.pwd;
	if (tr_running) {
		update_interrupter();
	}
	return 1;
}


uint8_t callback_ConfigFunction(parameter_entry * params, uint8_t index, uint8_t port){
    uint8 sfflag = system_fault_Read();
    system_fault_Control = 0; //halt tesla coil operation during updates!
    initialize_interrupter();
	initialize_charging();
	configure_ZCD_to_PWM();
	system_fault_Control = sfflag;
    return 1;
}

uint8_t callback_DefaultFunction(parameter_entry * params, uint8_t index, uint8_t port){
    
    return 1;
}

uint8_t command_bus(char *commandline, uint8_t port) {
    SKIP_SPACE(commandline);
    HELP_TEXT("Usage: bus [on|off]\r\n");

	if (strcmp(commandline, "on") == 0) {
		bus_command = BUS_COMMAND_ON;
		send_string("BUS ON\r\n", port);
	}
	if (strcmp(commandline, "off") == 0) {
		bus_command = BUS_COMMAND_OFF;
		send_string("BUS OFF\r\n", port);
	}

	return 1;
}

uint8_t command_load_default(char *commandline, uint8_t port) {
    send_string("Default parameters loaded\r\n", port);
    init_config();
    return 1;
}

uint8_t command_reset(char *commandline, uint8_t port){
    CySoftwareReset();
    return 1;
}

xTaskHandle overlay_Serial_TaskHandle;
xTaskHandle overlay_USB_TaskHandle;

uint8_t command_status(char *commandline, uint8_t port) {
	SKIP_SPACE(commandline);
	HELP_TEXT("Usage: status [start|stop]\r\n");

    
	if (strcmp(commandline, "start") == 0) {
		switch (port) {
		case SERIAL:
			if (overlay_Serial_TaskHandle == NULL) {
				xTaskCreate(tsk_overlay_TaskProc, "Overl_S", 256, (void *)SERIAL, PRIO_OVERLAY, &overlay_Serial_TaskHandle);
			}
			break;
		case USB:
			if (overlay_USB_TaskHandle == NULL) {
				xTaskCreate(tsk_overlay_TaskProc, "Overl_U", 256, (void *)USB, PRIO_OVERLAY, &overlay_USB_TaskHandle);
			}
			break;
		}
	}
	if (strcmp(commandline, "stop") == 0) {
		switch (port) {
		case SERIAL:
			if (overlay_Serial_TaskHandle != NULL) {
				vTaskDelete(overlay_Serial_TaskHandle);
				overlay_Serial_TaskHandle = NULL;
			}
			break;
		case USB:
			if (overlay_USB_TaskHandle != NULL) {
				vTaskDelete(overlay_USB_TaskHandle);
				overlay_Serial_TaskHandle = NULL;
			}
			break;
		}
	}

	return 1;
}

uint8_t command_tterm(char *commandline, uint8_t port){
    SKIP_SPACE(commandline);
    HELP_TEXT("Usage: status [start|stop]\r\n");


	if (strcmp(commandline, "start") == 0) {
        send_gauge_config(0, GAUGE0_MIN, GAUGE0_MAX, GAUGE0_NAME, port);
        send_gauge_config(1, GAUGE1_MIN, GAUGE1_MAX, GAUGE1_NAME, port);
        send_gauge_config(2, GAUGE2_MIN, GAUGE2_MAX, GAUGE2_NAME, port);
        send_gauge_config(3, GAUGE3_MIN, GAUGE3_MAX, GAUGE3_NAME, port);
        send_gauge_config(4, GAUGE4_MIN, GAUGE4_MAX, GAUGE4_NAME, port);
        send_gauge_config(5, GAUGE5_MIN, GAUGE5_MAX, GAUGE5_NAME, port);
        send_gauge_config(6, GAUGE6_MIN, GAUGE6_MAX, GAUGE6_NAME, port);
        
        send_chart_config(0, CHART0_MIN, CHART0_MAX, CHART0_OFFSET, CHART0_UNIT, CHART0_NAME, port);
        send_chart_config(1, CHART1_MIN, CHART1_MAX, CHART1_OFFSET, CHART1_UNIT, CHART1_NAME, port);
        send_chart_config(2, CHART2_MIN, CHART2_MAX, CHART2_OFFSET, CHART2_UNIT, CHART2_NAME, port);
        send_chart_config(3, CHART3_MIN, CHART3_MAX, CHART3_OFFSET, CHART3_UNIT, CHART3_NAME, port);
        
        switch (port) {
		case SERIAL:
			if (overlay_Serial_TaskHandle == NULL) {
				xTaskCreate(tsk_overlay_TaskProc, "Overl_S", 256, (void *)SERIAL, PRIO_OVERLAY, &overlay_Serial_TaskHandle);
			}
			break;
		case USB:
			if (overlay_USB_TaskHandle == NULL) {
				xTaskCreate(tsk_overlay_TaskProc, "Overl_U", 256, (void *)USB, PRIO_OVERLAY, &overlay_USB_TaskHandle);
			}
			break;
		}

        term_mode = port;
	}
	if (strcmp(commandline, "stop") == 0) {
        term_mode = TERM_MODE_VT100;
        switch (port) {
		case SERIAL:
			if (overlay_Serial_TaskHandle != NULL) {
				vTaskDelete(overlay_Serial_TaskHandle);
				overlay_Serial_TaskHandle = NULL;
			}
			break;
		case USB:
			if (overlay_USB_TaskHandle != NULL) {
				vTaskDelete(overlay_USB_TaskHandle);
				overlay_Serial_TaskHandle = NULL;
			}
			break;
		}
	} 
    
    return 1;
}


uint8_t command_cls(char *commandline, uint8_t port) {
	Term_Erase_Screen(port);
    send_string("  _   _   ___    ____    _____              _\r\n",port);
    send_string(" | | | | |   \\  |__ /   |_   _|  ___   ___ | |  __ _\r\n",port);
    send_string(" | |_| | | |) |  |_ \\     | |   / -_) (_-< | | / _` |\r\n",port);
    send_string("  \\___/  |___/  |___/     |_|   \\___| /__/ |_| \\__,_|\r\n\r\n",port);
    send_string("\tCoil: ",port);
    send_string(configuration.ud_name,port);
    send_string("\r\n\r\n",port);
	return 1;
}

uint8_t command_minstat(char *commandline, uint8_t port){
    char buffer[60];
    sprintf(buffer,"Dropped frames        : %lu\r\n",telemetry.dropped_frames);
    send_string(buffer,port);
    sprintf(buffer,"Spurious acks         : %lu\r\n",telemetry.spurious_acks);
    send_string(buffer,port);
    sprintf(buffer,"Resets received       : %lu\r\n",telemetry.resets_received);
    send_string(buffer,port);
    sprintf(buffer,"Sequence mismatch drop: %lu\r\n",telemetry.sequence_mismatch_drop);
    send_string(buffer,port);
    sprintf(buffer,"Max frames in buffer  : %u\r\n",telemetry.min_frames_max);
    send_string(buffer,port); 
    return 1; 
}

uint8_t command_bootloader(char *commandline, uint8_t port) {
	Bootloadable_Load();
	return 1;
}

#define BURST_ON 0
#define BURST_OFF 1

void vBurst_Timer_Callback(TimerHandle_t xTimer){
    if(burst_state == BURST_ON){
        interrupter.pw = 0;
        burst_state = BURST_OFF;
        xTimerChangePeriod( xTimer, param.burst_off / portTICK_PERIOD_MS, 0 );                                                   
    }else{
        interrupter.pw = param.pw;
        burst_state = BURST_ON;
        xTimerChangePeriod( xTimer, param.burst_on / portTICK_PERIOD_MS, 0 );
    }          
}

uint8_t command_tr(char *commandline, uint8_t port) {
    SKIP_SPACE(commandline);
    HELP_TEXT("Usage: tr [start|stop]\r\n");

	if (strcmp(commandline, "start") == 0) {
        interrupter.pw = param.pw;
		interrupter.prd = param.pwd;
        update_interrupter();
        burst_state = BURST_ON;
        if(xBurst_Timer==NULL && param.burst_on > 0){
                xBurst_Timer = xTimerCreate("Bust-Tmr", param.burst_on / portTICK_PERIOD_MS, pdFALSE,(void * ) 0, vBurst_Timer_Callback);
                if(xBurst_Timer != NULL){
                    xTimerStart(xBurst_Timer, 0);
                    send_string("Burst Enabled\r\n", port);
                    tr_running=1;
                }else{
                    interrupter.pw = 0;
                    send_string("Cannot create burst Timer\r\n", port);
                    tr_running=0;
                }
        }else{
		    tr_running = 1;
		    send_string("Transient Enabled\r\n", port);
        }
        return 0;
	}
	if (strcmp(commandline, "stop") == 0) {
        interrupter.pw = 0;
		update_interrupter();
        if (xBurst_Timer != NULL) {
				if(xTimerDelete(xBurst_Timer, 200 / portTICK_PERIOD_MS) != pdFALSE){
				    xBurst_Timer = NULL;
                    burst_state = BURST_ON;
                    send_string("\r\nBurst Disabled\r\n", port);    
                } else {
                    send_string("Cannot delete burst Timer\r\n", port);
                    burst_state = BURST_ON;
                }
		}else{
            
        send_string("\r\nTransient Disabled\r\n", port);    
        }
        
		interrupter.pw = 0;
		update_interrupter();
		tr_running = 0;
		
		return 0;
	}
	return 1;
}
void vQCW_Timer_Callback(TimerHandle_t xTimer){
    if(!qcw_reg){
         	ramp.modulation_value = 20;
            qcw_reg = 1;
		    ramp_control(); 
    }
    if(param.qcw_repeat<100) param.qcw_repeat = 100;
    xTimerChangePeriod( xTimer, param.qcw_repeat / portTICK_PERIOD_MS, 0 );                                                   
}

uint8_t command_qcw(char *commandline, uint8_t port) {
    SKIP_SPACE(commandline);
    HELP_TEXT("Usage: qcw [start|stop]\r\n");
    
	if (strcmp(commandline, "start") == 0) {
        if(param.qcw_repeat>99){
            if(xQCW_Timer==NULL){
                xQCW_Timer = xTimerCreate("QCW-Tmr", param.qcw_repeat / portTICK_PERIOD_MS, pdFALSE,(void * ) 0, vQCW_Timer_Callback);
                if(xQCW_Timer != NULL){
                    xTimerStart(xQCW_Timer, 0);
                    send_string("QCW Enabled\r\n", port);
                }else{
                    send_string("Cannot create QCW Timer\r\n", port);
                }
            }
        }else{
		    ramp.modulation_value = 20;
            qcw_reg = 1;
		    ramp_control();
            send_string("QCW single shot\r\n", port);
        }
		
		return 0;
	}
	if (strcmp(commandline, "stop") == 0) {
        if (xQCW_Timer != NULL) {
				if(xTimerDelete(xQCW_Timer, 200 / portTICK_PERIOD_MS) != pdFALSE){
				    xQCW_Timer = NULL;
                } else {
                    send_string("Cannot delete QCW Timer\r\n", port);
                }
		}
        QCW_enable_Control = 0;
        qcw_reg = 0;
		send_string("QCW Disabled\r\n", port);
		return 0;
	}
	return 1;
}

uint8_t command_udkill(char *commandline, uint8_t port) {
    SKIP_SPACE(commandline);
    HELP_TEXT("Usage: kill [set|reset|get]\r\n");
    
    if (strcmp(commandline, "set") == 0) {
        interrupter_kill();
    	USBMIDI_1_callbackLocalMidiEvent(0, (uint8_t*)kill_msg);
    	bus_command = BUS_COMMAND_OFF;
        
        if (xQCW_Timer != NULL) {
    		if(xTimerDelete(xQCW_Timer, 200 / portTICK_PERIOD_MS) != pdFALSE){
    	        xQCW_Timer = NULL;
            }
    	}
        
    	interrupter1_control_Control = 0;
    	QCW_enable_Control = 0;
    	Term_Color_Green(port);
    	send_string("Killbit set\r\n", port);
    	Term_Color_White(port);
    }else if (strcmp(commandline, "reset") == 0) {
        interrupter_unkill();
        Term_Color_Green(port);
    	send_string("Killbit reset\r\n", port);
    	Term_Color_White(port);
        return 1;
    }else if (strcmp(commandline, "get") == 0) {
        char buf[30];
        Term_Color_Green(port);
        sprintf(buf, "Killbit: %u",interrupter_get_kill());
        send_string(buf,port);
        Term_Color_White(port);
    }
    
        
	return 0;
}


uint8_t command_tune_p(char *commandline, uint8_t port) {
    uint16_t f_res;
    int16_t f_fine_start;
    int16_t f_fine_end;
    
	f_res = run_adc_sweep(param.tune_start, param.tune_end, param.tune_pw, CT_PRIMARY, param.tune_delay, port);
    f_fine_start = f_res -60;
    f_fine_end = f_res +60;
    if(f_fine_start < param.tune_start) f_fine_start = param.tune_start;
    if(f_fine_end > param.tune_end) f_fine_end = param.tune_end;
    
    f_res = run_adc_sweep(f_fine_start, f_fine_end, param.tune_pw, CT_PRIMARY, param.tune_delay, port);
	return 0;
}

uint8_t command_tune_s(char *commandline, uint8_t port) {
	run_adc_sweep(param.tune_start, param.tune_end, param.tune_pw, CT_SECONDARY, param.tune_delay, port);
	return 0;
}

uint8_t command_tasks(char *commandline, uint8_t port) {
    #if configUSE_STATS_FORMATTING_FUNCTIONS && configUSE_TRACE_FACILITY && configGENERATE_RUN_TIME_STATS
        
	    static char buff[600];
	    send_string("**********************************************\n\r", port);
	    send_string("Task            State   Prio    Stack    Num\n\r", port);
	    send_string("**********************************************\n\r", port);
	    vTaskList(buff);
	    send_string(buff, port);
	    send_string("**********************************************\n\r\n\r", port);
        send_string("**********************************************\n\r", port);
	    send_string("Task            Abs time        % time\n\r", port);
	    send_string("**********************************************\n\r", port);
        vTaskGetRunTimeStats( buff );
        send_string(buff, port);
        send_string("**********************************************\n\r\r\n", port);
        sprintf(buff, "Free heap: %d\r\n",xPortGetFreeHeapSize());
        send_string(buff,port);
        return 0;
    #endif
    
    #if !configUSE_STATS_FORMATTING_FUNCTIONS && !configUSE_TRACE_FACILITY
	    send_string("Taskinfo not active, activate it in FreeRTOSConfig.h\n\r", port);
        return 0;
	#endif
    
	
}

uint8_t commandGet(char *commandline, uint8_t port) {
	SKIP_SPACE(commandline);
    
	uint8_t current_parameter;


	if (*commandline == 0 || commandline == 0) //no param --> show help text
	{
		print_param_help(confparam, PARAM_SIZE(confparam), port);
		return 1;
	}

	for (current_parameter = 0; current_parameter < sizeof(confparam) / sizeof(parameter_entry); current_parameter++) {
		if (strcmp(commandline, confparam[current_parameter].name) == 0) {
			//Parameter not found:
			print_param(confparam,current_parameter,port);
			return 1;
		}
	}
	Term_Color_Red(port);
	send_string("E: unknown param\r\n", port);
	Term_Color_White(port);
	return 0;
}

uint8_t commandSet(char *commandline, uint8_t port) {
	
	char *param_value;

	if (*commandline == ' ')
		commandline++;

	if (commandline == NULL) {
		if (!port)
			Term_Color_Red(port);
		send_string("E: no name\r\n", port);
		Term_Color_White(port);
		return 0;
	}

	param_value = strchr(commandline, ' ');
	if (param_value == NULL) {
		Term_Color_Red(port);
		send_string("E: no value\r\n", port);
		Term_Color_White(port);
		return 0;
	}

	*param_value = 0;
	param_value++;

	if (*param_value == '\0') {
		Term_Color_Red(port);
		send_string("E: no val\r\n", port);
		Term_Color_White(port);
		return 0;
	}

	uint8_t current_parameter;
	for (current_parameter = 0; current_parameter < sizeof(confparam) / sizeof(parameter_entry); current_parameter++) {
		if (strcmp(commandline, confparam[current_parameter].name) == 0) {
			//parameter name found:

			if (updateDefaultFunction(confparam, param_value,current_parameter, port)){
                if(confparam[current_parameter].callback_function){
                    if (confparam[current_parameter].callback_function(confparam, current_parameter, port)){
                        Term_Color_Green(port);
                        send_string("OK\r\n", port);
                        Term_Color_White(port);
                        return 1;
                    }else{
                        Term_Color_Red(port);
                        send_string("ERROR: Callback\r\n", port);
                        Term_Color_White(port);
                        return 1;
                    }
                }else{
                    Term_Color_Green(port);
                    send_string("OK\r\n", port);
                    Term_Color_White(port);
                    return 1;
                }
			} else {
				Term_Color_Red(port);
				send_string("NOK\r\n", port);
				Term_Color_White(port);
				return 1;
			}
		}
	}
	Term_Color_Red(port);
	send_string("E: unknown param\r\n", port);
	Term_Color_White(port);
	return 0;
}

void eeprom_load(){
   EEPROM_read_conf(confparam, PARAM_SIZE(confparam) ,0,SERIAL); 
}

/********************************************
* Saves confparams to eeprom
*********************************************/
uint8_t command_eprom(char *commandline, uint8_t port) {
	SKIP_SPACE(commandline);
    HELP_TEXT("Usage: eprom [load|save]\r\n");
    
	uint8 sfflag = system_fault_Read();
	system_fault_Control = 0; //halt tesla coil operation during updates!
	if (strcmp(commandline, "save") == 0) {
	    EEPROM_write_conf(confparam, PARAM_SIZE(confparam),0, port);
		system_fault_Control = sfflag;
		return 0;
	}
	if (strcmp(commandline, "load") == 0) {
        uint8 sfflag = system_fault_Read();
        system_fault_Control = 0; //halt tesla coil operation during updates!
		EEPROM_read_conf(confparam, PARAM_SIZE(confparam) ,0,port);
        
        initialize_interrupter();
	    initialize_charging();
	    configure_ZCD_to_PWM();
	    system_fault_Control = sfflag;
		return 0;
	}
	return 1;
}

/********************************************
* Prints the help text
*********************************************/
uint8_t commandHelp(char *commandline, uint8_t port) {
	UNUSED_VARIABLE(commandline);
	uint8_t current_command;
	send_string("\r\nCommands:\r\n", port);
	for (current_command = 0; current_command < (sizeof(commands) / sizeof(command_entry)); current_command++) {
		send_string("\t", port);
		Term_Color_Cyan(port);
		send_string((char *)commands[current_command].text, port);
		Term_Color_White(port);
		if (strlen(commands[current_command].text) > 7) {
			send_string("\t-> ", port);
		} else {
			send_string("\t\t-> ", port);
		}
		send_string((char *)commands[current_command].help, port);
		send_string("\r\n", port);
	}

	send_string("\r\nParameters:\r\n", port);
	for (current_command = 0; current_command < sizeof(confparam) / sizeof(parameter_entry); current_command++) {
        if(confparam[current_command].parameter_type == PARAM_DEFAULT){
            send_string("\t", port);
            Term_Color_Cyan(port);
            send_string((char *)confparam[current_command].name, port);
            Term_Color_White(port);
            if (strlen(confparam[current_command].name) > 7) {
                send_string("\t-> ", port);
            } else {
                send_string("\t\t-> ", port);
            }

            send_string((char *)confparam[current_command].help, port);
            send_string("\r\n", port);
        }
	}

	send_string("\r\nConfiguration:\r\n", port);
	for (current_command = 0; current_command < sizeof(confparam) / sizeof(parameter_entry); current_command++) {
        if(confparam[current_command].parameter_type == PARAM_CONFIG){
            send_string("\t", port);
            Term_Color_Cyan(port);
            send_string((char *)confparam[current_command].name, port);
            Term_Color_White(port);
            if (strlen(confparam[current_command].name) > 7) {
                send_string("\t-> ", port);
            } else {
                send_string("\t\t-> ", port);
            }

            send_string((char *)confparam[current_command].help, port);
            send_string("\r\n", port);
        }
	}

	return 0;
}


/********************************************
* Interprets the Input String
*********************************************/
void nt_interpret(const char *text, uint8_t port) {
	uint8_t current_command;
	for (current_command = 0; current_command < (sizeof(commands) / sizeof(command_entry)); current_command++) {
		if (memcmp(text, commands[current_command].text, strlen(commands[current_command].text)) == 0) {
			commands[current_command].commandFunction((char *)strchr(text, ' '), port);

			return;
		}
	}
	if (text[0] != 0x00) {
		Term_Color_Red(port);
		send_string("Unknown Command: ", port);
		send_string((char *)text, port);
		send_string("\r\n", port);
		Term_Color_White(port);
	}
}


