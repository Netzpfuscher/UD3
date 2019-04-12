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
#include "ntlibc.h"
#include "telemetry.h"
#include <project.h>
#include <stdint.h>
#include "helper/printf.h"

#include "cyapicallbacks.h"
#include <cytypes.h>

#include "tasks/tsk_analog.h"
#include "tasks/tsk_overlay.h"
#include "tasks/tsk_priority.h"
#include "tasks/tsk_uart.h"
#include "tasks/tsk_usb.h"
#include "tasks/tsk_midi.h"
#include "tasks/tsk_cli.h"
#include "tasks/tsk_min.h"
#include "min_id.h"
#include "helper/teslaterm.h"

#define UNUSED_VARIABLE(N) \
	do {                   \
		(void)(N);         \
	} while (0)
        

#define SKIP_SPACE(ptr) 	if (*ptr == 0x20 && ptr != 0) ptr++ //skip space
#define HELP_TEXT(text)        \
	if (*commandline == 0 || commandline == 0) {    \
		Term_Color_Red(ptr);       \
		send_string(text, ptr);  \
		Term_Color_White(ptr);     \
		return 1;}                  \
	
typedef struct {
	const char *text;
	uint8_t (*commandFunction)(char *commandline, port_str *ptr);
	const char *help;
} command_entry;

uint8_t callback_ConfigFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_DefaultFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_TuneFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_TRFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_OfftimeFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_BurstFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_SynthFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_i2tFunction(parameter_entry * params, uint8_t index, port_str *ptr);

uint8_t command_help(char *commandline, port_str *ptr);
uint8_t command_get(char *commandline, port_str *ptr);
uint8_t command_set(char *commandline, port_str *ptr);
uint8_t command_tr(char *commandline, port_str *ptr);
uint8_t command_udkill(char *commandline, port_str *ptr);
uint8_t command_eprom(char *commandline, port_str *ptr);
uint8_t command_status(char *commandline, port_str *ptr);
uint8_t command_tune_p(char *commandline, port_str *ptr);
uint8_t command_tune_s(char *commandline, port_str *ptr);
uint8_t command_tasks(char *commandline, port_str *ptr);
uint8_t command_bootloader(char *commandline, port_str *ptr);
uint8_t command_qcw(char *commandline, port_str *ptr);
uint8_t command_bus(char *commandline, port_str *ptr);
uint8_t command_load_default(char *commandline, port_str *ptr);
uint8_t command_tterm(char *commandline, port_str *ptr);
uint8_t command_reset(char *commandline, port_str *ptr);
uint8_t command_minstat(char *commandline, port_str *ptr);
uint8_t command_config_get(char *commandline, port_str *ptr);
uint8_t command_exit(char *commandline, port_str *ptr);
uint8_t command_ethcon(char *commandline, port_str *ptr);
uint8_t command_fuse(char *commandline, port_str *ptr);


uint8_t burst_state = 0;

TimerHandle_t xQCW_Timer;
TimerHandle_t xBurst_Timer;

cli_config configuration;
cli_parameter param;


/*****************************************************************************
* Initializes parameters with default values
******************************************************************************/
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
    ntlibc_strcpy(configuration.ud_name,"UD3-Tesla");
    ntlibc_strcpy(configuration.ip_addr,"192.168.50.250");
    ntlibc_strcpy(configuration.ip_subnet,"255.255.255.0");
    ntlibc_strcpy(configuration.ip_mac,"21:24:5D:AA:68:57");
    ntlibc_strcpy(configuration.ip_gw,"192.168.50.1");
    ntlibc_strcpy(configuration.ssid,"NULL");
    ntlibc_strcpy(configuration.passwd,"NULL");
    configuration.minprot = 0;
    configuration.max_const_i = 160;
    configuration.max_fault_i = 250;
    configuration.eth_hw = 0; //ESP32
    
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
    param.synth = SYNTH_MIDI;
    
    i2t_set_limit(configuration.max_const_i,configuration.max_fault_i,10000);
}

// clang-format off

/*****************************************************************************
* Parameter struct
******************************************************************************/

parameter_entry confparam[] = {
    //        Parameter Type,"Text   "         , Value ptr                     , Type          ,Min    ,Max    ,Div    ,Callback Function           ,Help text
    ADD_PARAM(PARAM_DEFAULT ,"pw"              , param.pw                      , TYPE_UNSIGNED ,0      ,800    ,0      ,callback_TRFunction         ,"Pulsewidth")
    ADD_PARAM(PARAM_DEFAULT ,"pwd"             , param.pwd                     , TYPE_UNSIGNED ,0      ,60000  ,0      ,callback_TRFunction         ,"Pulsewidthdelay")
    ADD_PARAM(PARAM_DEFAULT ,"bon"             , param.burst_on                , TYPE_UNSIGNED ,0      ,1000   ,0      ,callback_BurstFunction      ,"Burst mode ontime [ms] 0=off")
    ADD_PARAM(PARAM_DEFAULT ,"boff"            , param.burst_off               , TYPE_UNSIGNED ,0      ,1000   ,0      ,callback_BurstFunction      ,"Burst mode offtime [ms]")
    ADD_PARAM(PARAM_DEFAULT ,"tune_start"      , param.tune_start              , TYPE_UNSIGNED ,5      ,5000   ,10     ,callback_TuneFunction       ,"Start frequency [kHz]")
    ADD_PARAM(PARAM_DEFAULT ,"tune_end"        , param.tune_end                , TYPE_UNSIGNED ,5      ,5000   ,10     ,callback_TuneFunction       ,"End frequency [kHz]")
    ADD_PARAM(PARAM_DEFAULT ,"tune_pw"         , param.tune_pw                 , TYPE_UNSIGNED ,0      ,800    ,0      ,NULL                        ,"Tune pulsewidth")
    ADD_PARAM(PARAM_DEFAULT ,"tune_delay"      , param.tune_delay              , TYPE_UNSIGNED ,1      ,200    ,0      ,NULL                        ,"Tune delay")
    ADD_PARAM(PARAM_CONFIG  ,"offtime"         , param.offtime                 , TYPE_UNSIGNED ,2      ,250    ,0      ,callback_OfftimeFunction    ,"Offtime for MIDI")
    ADD_PARAM(PARAM_DEFAULT ,"qcw_ramp"        , param.qcw_ramp                , TYPE_UNSIGNED ,1      ,10     ,0      ,NULL                        ,"QCW Ramp Inc/93us")
    ADD_PARAM(PARAM_DEFAULT ,"qcw_repeat"      , param.qcw_repeat              , TYPE_UNSIGNED ,0      ,1000   ,0      ,NULL                        ,"QCW pulse repeat time [ms] <100=single shot")
    ADD_PARAM(PARAM_DEFAULT ,"transpose"       , param.transpose               , TYPE_SIGNED   ,-48    ,48     ,0      ,NULL                        ,"Transpose MIDI")
    ADD_PARAM(PARAM_DEFAULT ,"synth"           , param.synth                   , TYPE_UNSIGNED ,0      ,2      ,0      ,callback_SynthFunction      ,"0=off 1=MIDI 2=SID")
    ADD_PARAM(PARAM_CONFIG  ,"watchdog"        , configuration.watchdog        , TYPE_UNSIGNED ,0      ,1      ,0      ,NULL                        ,"Watchdog Enable")
    ADD_PARAM(PARAM_CONFIG  ,"max_tr_pw"       , configuration.max_tr_pw       , TYPE_UNSIGNED ,0      ,3000   ,0      ,callback_ConfigFunction     ,"Maximum TR PW [uSec]")
    ADD_PARAM(PARAM_CONFIG  ,"max_tr_prf"      , configuration.max_tr_prf      , TYPE_UNSIGNED ,0      ,3000   ,0      ,callback_ConfigFunction     ,"Maximum TR frequency [Hz]")
    ADD_PARAM(PARAM_CONFIG  ,"max_qcw_pw"      , configuration.max_qcw_pw      , TYPE_UNSIGNED ,0      ,30000  ,1000   ,callback_ConfigFunction     ,"Maximum QCW PW [ms]")
    ADD_PARAM(PARAM_CONFIG  ,"max_tr_current"  , configuration.max_tr_current  , TYPE_UNSIGNED ,0      ,2000   ,0      ,callback_ConfigFunction     ,"Maximum TR current [A]")
    ADD_PARAM(PARAM_CONFIG  ,"min_tr_current"  , configuration.min_tr_current  , TYPE_UNSIGNED ,0      ,2000   ,0      ,callback_ConfigFunction     ,"Minimum TR current [A]")
    ADD_PARAM(PARAM_CONFIG  ,"max_qcw_current" , configuration.max_qcw_current , TYPE_UNSIGNED ,0      ,2000   ,0      ,callback_ConfigFunction     ,"Maximum QCW current [A]")
    ADD_PARAM(PARAM_CONFIG  ,"temp1_max"       , configuration.temp1_max       , TYPE_UNSIGNED ,0      ,100    ,0      ,NULL                        ,"Max temperature 1 [*C]")
    ADD_PARAM(PARAM_CONFIG  ,"temp2_max"       , configuration.temp2_max       , TYPE_UNSIGNED ,0      ,100    ,0      ,NULL                        ,"Max temperature 2 [*C]")
    ADD_PARAM(PARAM_CONFIG  ,"ct1_ratio"       , configuration.ct1_ratio       , TYPE_UNSIGNED ,1      ,2000   ,0      ,callback_ConfigFunction     ,"CT1 [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,"ct2_ratio"       , configuration.ct2_ratio       , TYPE_UNSIGNED ,1      ,5000   ,0      ,callback_ConfigFunction     ,"CT2 [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,"ct3_ratio"       , configuration.ct3_ratio       , TYPE_UNSIGNED ,1      ,2000   ,0      ,callback_ConfigFunction     ,"CT3 [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,"ct1_burden"      , configuration.ct1_burden      , TYPE_UNSIGNED ,1      ,1000   ,10     ,callback_ConfigFunction     ,"CT1 burden [Ohm]")
    ADD_PARAM(PARAM_CONFIG  ,"ct2_burden"      , configuration.ct2_burden      , TYPE_UNSIGNED ,1      ,1000   ,10     ,callback_ConfigFunction     ,"CT2 burden [Ohm]")
    ADD_PARAM(PARAM_CONFIG  ,"ct3_burden"      , configuration.ct3_burden      , TYPE_UNSIGNED ,1      ,1000   ,10     ,callback_ConfigFunction     ,"CT3 burden [Ohm]")
    ADD_PARAM(PARAM_CONFIG  ,"lead_time"       , configuration.lead_time       , TYPE_UNSIGNED ,0      ,2000   ,0      ,callback_ConfigFunction     ,"Lead time [nSec]")
    ADD_PARAM(PARAM_CONFIG  ,"start_freq"      , configuration.start_freq      , TYPE_UNSIGNED ,0      ,5000   ,10     ,callback_ConfigFunction     ,"Resonant freq [kHz]")
    ADD_PARAM(PARAM_CONFIG  ,"start_cycles"    , configuration.start_cycles    , TYPE_UNSIGNED ,0      ,20     ,0      ,callback_ConfigFunction     ,"Start Cyles [N]")
    ADD_PARAM(PARAM_CONFIG  ,"max_tr_duty"     , configuration.max_tr_duty     , TYPE_UNSIGNED ,1      ,500    ,10     ,callback_ConfigFunction     ,"Max TR duty cycle [%]")
    ADD_PARAM(PARAM_CONFIG  ,"max_qcw_duty"    , configuration.max_qcw_duty    , TYPE_UNSIGNED ,1      ,500    ,10     ,callback_ConfigFunction     ,"Max QCW duty cycle [%]")
    ADD_PARAM(PARAM_CONFIG  ,"temp1_setpoint"  , configuration.temp1_setpoint  , TYPE_UNSIGNED ,0      ,100    ,0      ,NULL                        ,"Setpoint for fan [*C]")
    ADD_PARAM(PARAM_CONFIG  ,"batt_lockout_v"  , configuration.batt_lockout_v  , TYPE_UNSIGNED ,0      ,500    ,0      ,NULL                        ,"Battery lockout voltage [V]")
    ADD_PARAM(PARAM_CONFIG  ,"slr_fswitch"     , configuration.slr_fswitch     , TYPE_UNSIGNED ,0      ,1000   ,10     ,callback_ConfigFunction     ,"SLR switch frequency [kHz]")
    ADD_PARAM(PARAM_CONFIG  ,"slr_vbus"        , configuration.slr_vbus        , TYPE_UNSIGNED ,0      ,1000   ,0      ,NULL                        ,"SLR Vbus [V]")
    ADD_PARAM(PARAM_CONFIG  ,"ps_scheme"       , configuration.ps_scheme       , TYPE_UNSIGNED ,0      ,4      ,0      ,callback_ConfigFunction     ,"Power supply sheme")
    ADD_PARAM(PARAM_CONFIG  ,"autotune_s"      , configuration.autotune_s      , TYPE_UNSIGNED ,1      ,32     ,0      ,NULL                        ,"Number of samples for Autotune")
    ADD_PARAM(PARAM_CONFIG  ,"ud_name"         , configuration.ud_name         , TYPE_STRING   ,0      ,0      ,0      ,NULL                        ,"Name of the Coil [15 chars]")
    ADD_PARAM(PARAM_CONFIG  ,"ip_addr"         , configuration.ip_addr         , TYPE_STRING   ,0      ,0      ,0      ,NULL                        ,"IP-Adress of the UD3")
    ADD_PARAM(PARAM_CONFIG  ,"ip_gateway"      , configuration.ip_gw           , TYPE_STRING   ,0      ,0      ,0      ,NULL                        ,"Gateway adress")
    ADD_PARAM(PARAM_CONFIG  ,"ip_subnet"       , configuration.ip_subnet       , TYPE_STRING   ,0      ,0      ,0      ,NULL                        ,"Subnet")
    ADD_PARAM(PARAM_CONFIG  ,"ip_mac"          , configuration.ip_mac          , TYPE_STRING   ,0      ,0      ,0      ,NULL                        ,"MAC adress")
    ADD_PARAM(PARAM_CONFIG  ,"min_enable"      , configuration.minprot         , TYPE_UNSIGNED ,0      ,1      ,0      ,NULL                        ,"Use MIN-Protocol")
    ADD_PARAM(PARAM_CONFIG  ,"max_const_i"     , configuration.max_const_i     , TYPE_UNSIGNED ,0      ,2000   ,10     ,callback_i2tFunction        ,"Maximum constant current [A]")
    ADD_PARAM(PARAM_CONFIG  ,"max_fault_i"     , configuration.max_fault_i     , TYPE_UNSIGNED ,0      ,2000   ,10     ,callback_i2tFunction        ,"Maximum fault current for 10s [A]")
    ADD_PARAM(PARAM_CONFIG  ,"eth_hw"          , configuration.eth_hw          , TYPE_UNSIGNED ,0      ,2      ,0      ,NULL                        ,"0=Disabled 1=W5500 2=ESP32")
    ADD_PARAM(PARAM_CONFIG  ,"ssid"            , configuration.ssid            , TYPE_STRING   ,0      ,0      ,0      ,NULL                        ,"WLAN SSID")
    ADD_PARAM(PARAM_CONFIG  ,"passwd"          , configuration.passwd          , TYPE_STRING   ,0      ,0      ,0      ,NULL                        ,"WLAN password")
};

/*****************************************************************************
* Command struct
******************************************************************************/
command_entry commands[] = {
    ADD_COMMAND("bootloader"    ,command_bootloader     ,"Start bootloader")
    ADD_COMMAND("bus"           ,command_bus            ,"Bus [on/off]")
    ADD_COMMAND("cls"		    ,command_cls            ,"Clear screen")
    ADD_COMMAND("eeprom"	    ,command_eprom          ,"Save/Load config [load/save]")
	ADD_COMMAND("get"		    ,command_get            ,"Usage get [param]")
    ADD_COMMAND("help"          ,command_help           ,"This text")
    ADD_COMMAND("kill"		    ,command_udkill         ,"Kills all UD Coils")
    ADD_COMMAND("load_default"  ,command_load_default   ,"Loads the default parameters")
    ADD_COMMAND("qcw"           ,command_qcw            ,"QCW [start/stop]")
    ADD_COMMAND("reset"         ,command_reset          ,"Resets UD3")
	ADD_COMMAND("set"		    ,command_set            ,"Usage set [param] [value]")
    ADD_COMMAND("status"	    ,command_status         ,"Displays coil status")
    ADD_COMMAND("tasks"	        ,command_tasks          ,"Show running Tasks")
    ADD_COMMAND("tr"		    ,command_tr             ,"Transient [start/stop]")
    ADD_COMMAND("tune_p"	    ,command_tune_p         ,"Autotune Primary")
    ADD_COMMAND("tune_s"	    ,command_tune_s         ,"Autotune Secondary")
    ADD_COMMAND("tterm"	        ,command_tterm          ,"Changes terminal mode")
    ADD_COMMAND("minstat"	    ,command_minstat        ,"Prints the min statistics")
    ADD_COMMAND("ethcon"	    ,command_ethcon         ,"Prints the eth connections")
    ADD_COMMAND("config_get"    ,command_config_get     ,"Internal use")
    ADD_COMMAND("fuse_reset"    ,command_fuse           ,"Reset the internal fuse")
};
// clang-format on

/*****************************************************************************
* Callback if the offtime parameter is changed
******************************************************************************/
uint8_t callback_OfftimeFunction(parameter_entry * params, uint8_t index, port_str *ptr){
    Offtime_WritePeriod(param.offtime);
    return 1;
}

/*****************************************************************************
* Callback if a autotune parameter is changed
******************************************************************************/
uint8_t callback_TuneFunction(parameter_entry * params, uint8_t index, port_str *ptr) {
    if(param.tune_start >= param.tune_end){
        SEND_CONST_STRING("ERROR: tune_start > tune_end", ptr);
		return 0;
	} else
		return 1;
}

/*****************************************************************************
* Switches the synthesizer
******************************************************************************/
uint8_t callback_SynthFunction(parameter_entry * params, uint8_t index, port_str *ptr){
    switch_synth(param.synth);
    return 1;
}

/*****************************************************************************
* Callback if a transient mode parameter is changed
* Updates the interrupter hardware
******************************************************************************/
uint8_t callback_TRFunction(parameter_entry * params, uint8_t index, port_str *ptr) {
    
	interrupter.pw = param.pw;
	interrupter.prd = param.pwd;
    
    update_midi_duty();
    
	if (tr_running==1) {
		update_interrupter();
	}
	return 1;
}

#define BURST_ON 0
#define BURST_OFF 1

/*****************************************************************************
* Timer callback for burst mode
******************************************************************************/
void vBurst_Timer_Callback(TimerHandle_t xTimer){
    uint16_t bon_lim;
    uint16_t boff_lim;
    if(burst_state == BURST_ON){
        interrupter.pw = 0;
        update_interrupter();
        burst_state = BURST_OFF;
        if(!param.burst_off){
            boff_lim=2;
        }else{
            boff_lim=param.burst_off;
        }
        xTimerChangePeriod( xTimer, boff_lim / portTICK_PERIOD_MS, 0 );
    }else{
        interrupter.pw = param.pw;
        update_interrupter();
        burst_state = BURST_ON;
        if(!param.burst_on){
            bon_lim=2;
        }else{
            bon_lim=param.burst_on;
        }
        xTimerChangePeriod( xTimer, bon_lim / portTICK_PERIOD_MS, 0 );
    }
}


/*****************************************************************************
* Callback if a burst mode parameter is changed
******************************************************************************/
uint8_t callback_BurstFunction(parameter_entry * params, uint8_t index, port_str *ptr) {
    if(tr_running){
        if(xBurst_Timer==NULL && param.burst_on > 0){
            burst_state = BURST_ON;
            xBurst_Timer = xTimerCreate("Bust-Tmr", param.burst_on / portTICK_PERIOD_MS, pdFALSE,(void * ) 0, vBurst_Timer_Callback);
            if(xBurst_Timer != NULL){
                xTimerStart(xBurst_Timer, 0);
                SEND_CONST_STRING("Burst Enabled\r\n", ptr);
                tr_running=2;
            }else{
                interrupter.pw = 0;
                SEND_CONST_STRING("Cannot create burst Timer\r\n", ptr);
                tr_running=0;
            }
        }else if(xBurst_Timer!=NULL && !param.burst_on){
            if (xBurst_Timer != NULL) {
    			if(xTimerDelete(xBurst_Timer, 200 / portTICK_PERIOD_MS) != pdFALSE){
    			    xBurst_Timer = NULL;
                    burst_state = BURST_ON;
                    interrupter.pw =0;
                    update_interrupter();
                    tr_running=1;
                    SEND_CONST_STRING("\r\nBurst Disabled\r\n", ptr);
                }else{
                    SEND_CONST_STRING("Cannot delete burst Timer\r\n", ptr);
                    burst_state = BURST_ON;
                }
            }
        }else if(xBurst_Timer!=NULL && !param.burst_off){
            if (xBurst_Timer != NULL) {
    			if(xTimerDelete(xBurst_Timer, 200 / portTICK_PERIOD_MS) != pdFALSE){
    			    xBurst_Timer = NULL;
                    burst_state = BURST_ON;
                    interrupter.pw =param.pw;
                    update_interrupter();
                    tr_running=1;
                    SEND_CONST_STRING("\r\nBurst Disabled\r\n", ptr);
                }else{
                    SEND_CONST_STRING("Cannot delete burst Timer\r\n", ptr);
                    burst_state = BURST_ON;
                }
            }

        }
    }
	return 1;
}

/*****************************************************************************
* Callback if a configuration relevant parameter is changed
******************************************************************************/
uint8_t callback_ConfigFunction(parameter_entry * params, uint8_t index, port_str *ptr){
    uint8 sfflag = system_fault_Read();
    system_fault_Control = 0; //halt tesla coil operation during updates!
    initialize_interrupter();
	initialize_charging();
	configure_ZCD_to_PWM();
	system_fault_Control = sfflag;
    return 1;
}

/*****************************************************************************
* Default function if a parameter is changes (not used)
******************************************************************************/
uint8_t callback_DefaultFunction(parameter_entry * params, uint8_t index, port_str *ptr){
    
    return 1;
}

/*****************************************************************************
* Callback for overcurrent module
******************************************************************************/
uint8_t callback_i2tFunction(parameter_entry * params, uint8_t index, port_str *ptr){
    i2t_set_limit(configuration.max_const_i,configuration.max_fault_i,10000);
    return 1;
}
/*****************************************************************************
* Prints the ethernet connections
******************************************************************************/
uint8_t command_ethcon(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    #define COL_A 9
    #define COL_B 15
    char buffer[40];
    uint8_t ret;
    SEND_CONST_STRING("Ethernet\r\nIP: ",ptr);
    send_string(pEth_info[ETH_INFO_ETH]->ip,ptr);
    SEND_CONST_STRING("\r\nGateway: ",ptr);
    send_string(pEth_info[ETH_INFO_ETH]->gw,ptr);
    SEND_CONST_STRING("\r\nDevice info: ",ptr);
    send_string(pEth_info[ETH_INFO_ETH]->info,ptr);
    SEND_CONST_STRING("\r\n\nWIFI\r\nIP: ",ptr);
    send_string(pEth_info[ETH_INFO_WIFI]->ip,ptr);
    SEND_CONST_STRING("\r\nGateway: ",ptr);
    send_string(pEth_info[ETH_INFO_WIFI]->gw,ptr);
    SEND_CONST_STRING("\r\nDevice info: ",ptr);
    send_string(pEth_info[ETH_INFO_WIFI]->info,ptr);
    
    
    SEND_CONST_STRING("\r\n\nConnected clients:\r\n",ptr);
    Term_Move_cursor_right(COL_A,ptr);
    SEND_CONST_STRING("Num", ptr);
    Term_Move_cursor_right(COL_B,ptr);
    SEND_CONST_STRING("| Remote IP\r\n", ptr);
    
    for(uint8_t i=0;i<NUM_ETH_CON;i++){
        if(socket_info[i].socket==SOCKET_CONNECTED){
            Term_Move_cursor_right(COL_A,ptr);
            ret = snprintf(buffer,sizeof(buffer), "\033[36m%d", i);
            send_buffer((uint8_t*)buffer,ret,ptr);
            Term_Move_cursor_right(COL_B,ptr);
            ret = snprintf(buffer,sizeof(buffer), "\033[32m%s\r\n", socket_info[i].info);
            send_buffer((uint8_t*)buffer,ret,ptr);
        }
    }
    Term_Color_White(ptr);
	return 1;
}

/*****************************************************************************
* Switches the bus on/off
******************************************************************************/
uint8_t command_bus(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    HELP_TEXT("Usage: bus [on|off]\r\n");

	if (ntlibc_stricmp(commandline, "on") == 0) {
		bus_command = BUS_COMMAND_ON;
		SEND_CONST_STRING("BUS ON\r\n", ptr);
	}
	if (ntlibc_stricmp(commandline, "off") == 0) {
		bus_command = BUS_COMMAND_OFF;
		SEND_CONST_STRING("BUS OFF\r\n", ptr);
	}

	return 1;
}
/*****************************************************************************
* Resets the software fuse
******************************************************************************/
uint8_t command_fuse(char *commandline, port_str *ptr){
    i2t_reset();
    return 1;
}

/*****************************************************************************
* Loads the default parametes out of flash
******************************************************************************/
uint8_t command_load_default(char *commandline, port_str *ptr) {
    SEND_CONST_STRING("Default parameters loaded\r\n", ptr);
    init_config();
    return 1;
}

/*****************************************************************************
* Reset of the controller
******************************************************************************/
uint8_t command_reset(char *commandline, port_str *ptr){
    CySoftwareReset();
    return 1;
}

/*****************************************************************************
* Task handles for the overlay tasks
******************************************************************************/
xTaskHandle overlay_Serial_TaskHandle;
xTaskHandle overlay_USB_TaskHandle;
xTaskHandle overlay_ETH_TaskHandle[NUM_ETH_CON];

/*****************************************************************************
* Helper function for spawning the overlay task
******************************************************************************/
void start_overlay_task(port_str *ptr){
    switch(ptr->type){
        case PORT_TYPE_SERIAL:
            if (overlay_Serial_TaskHandle == NULL) {
        		xTaskCreate(tsk_overlay_TaskProc, "Overl_S", STACK_OVERLAY, ptr, PRIO_OVERLAY, &overlay_Serial_TaskHandle);
            }
        break;
        case PORT_TYPE_USB:
            if (overlay_USB_TaskHandle == NULL) {
    			xTaskCreate(tsk_overlay_TaskProc, "Overl_U", STACK_OVERLAY, ptr, PRIO_OVERLAY, &overlay_USB_TaskHandle);
    		}
        break;
        case PORT_TYPE_ETH:
            if (overlay_ETH_TaskHandle[ptr->num] == NULL) {
        		xTaskCreate(tsk_overlay_TaskProc, "Overl_E", STACK_OVERLAY, ptr, PRIO_OVERLAY, &overlay_ETH_TaskHandle[ptr->num]);
        	}
        break;
    }
}


/*****************************************************************************
* Helper function for killing the overlay task
******************************************************************************/
void stop_overlay_task(port_str *ptr){
    
    switch(ptr->type){
        case PORT_TYPE_SERIAL:
            if (overlay_Serial_TaskHandle != NULL) {
    				vTaskDelete(overlay_Serial_TaskHandle);
    				overlay_Serial_TaskHandle = NULL;
    		}
        break;
        case PORT_TYPE_USB:
            if (overlay_USB_TaskHandle != NULL) {
    				vTaskDelete(overlay_USB_TaskHandle);
    				overlay_USB_TaskHandle = NULL;
    		}
        break;
        case PORT_TYPE_ETH:
            if (overlay_ETH_TaskHandle[ptr->num] != NULL) {
			    vTaskDelete(overlay_ETH_TaskHandle[ptr->num]);
			    overlay_ETH_TaskHandle[ptr->num] = NULL;
		    }
        break;
    }
}

/*****************************************************************************
* 
******************************************************************************/
uint8_t command_status(char *commandline, port_str *ptr) {
	SKIP_SPACE(commandline);
	HELP_TEXT("Usage: status [start|stop]\r\n");

    
	if (ntlibc_stricmp(commandline, "start") == 0) {
		start_overlay_task(ptr);
	}
	if (ntlibc_stricmp(commandline, "stop") == 0) {
		stop_overlay_task(ptr);
	}

	return 1;
}

/*****************************************************************************
* Initializes the teslaterm telemetry
* Spawns the overlay task for telemetry stream generation
******************************************************************************/
uint8_t command_tterm(char *commandline, port_str *ptr){
    SKIP_SPACE(commandline);
    HELP_TEXT("Usage: tterm [start|stop]\r\n");


	if (ntlibc_stricmp(commandline, "start") == 0) {
        ptr->term_mode = PORT_TERM_TT;
        send_gauge_config(0, GAUGE0_MIN, GAUGE0_MAX, GAUGE0_NAME, ptr);
        send_gauge_config(1, GAUGE1_MIN, GAUGE1_MAX, GAUGE1_NAME, ptr);
        send_gauge_config(2, GAUGE2_MIN, GAUGE2_MAX, GAUGE2_NAME, ptr);
        send_gauge_config(3, GAUGE3_MIN, GAUGE3_MAX, GAUGE3_NAME, ptr);
        send_gauge_config(4, GAUGE4_MIN, GAUGE4_MAX, GAUGE4_NAME, ptr);
        send_gauge_config(5, GAUGE5_MIN, GAUGE5_MAX, GAUGE5_NAME, ptr);
        send_gauge_config(6, GAUGE6_MIN, GAUGE6_MAX, GAUGE6_NAME, ptr);
        
        send_chart_config(0, CHART0_MIN, CHART0_MAX, CHART0_OFFSET, CHART0_UNIT, CHART0_NAME, ptr);
        send_chart_config(1, CHART1_MIN, CHART1_MAX, CHART1_OFFSET, CHART1_UNIT, CHART1_NAME, ptr);
        send_chart_config(2, CHART2_MIN, CHART2_MAX, CHART2_OFFSET, CHART2_UNIT, CHART2_NAME, ptr);
        send_chart_config(3, CHART3_MIN, CHART3_MAX, CHART3_OFFSET, CHART3_UNIT, CHART3_NAME, ptr);

        start_overlay_task(ptr);

    }
	if (ntlibc_stricmp(commandline, "stop") == 0) {
        ptr->term_mode = PORT_TERM_VT100;
        stop_overlay_task(ptr);

	} 
    
    return 1;
}

/*****************************************************************************
* Clears the terminal screen and displays the logo
******************************************************************************/
uint8_t command_cls(char *commandline, port_str *ptr) {
    tsk_overlay_chart_start();
	Term_Erase_Screen(ptr);
    SEND_CONST_STRING("  _   _   ___    ____    _____              _\r\n",ptr);
    SEND_CONST_STRING(" | | | | |   \\  |__ /   |_   _|  ___   ___ | |  __ _\r\n",ptr);
    SEND_CONST_STRING(" | |_| | | |) |  |_ \\     | |   / -_) (_-< | | / _` |\r\n",ptr);
    SEND_CONST_STRING("  \\___/  |___/  |___/     |_|   \\___| /__/ |_| \\__,_|\r\n\r\n",ptr);
    SEND_CONST_STRING("\tCoil: ",ptr);
    send_string(configuration.ud_name,ptr);
    SEND_CONST_STRING("\r\n\r\n",ptr);
	return 1;
}

/*****************************************************************************
* Displays the statistics of the min protocol
******************************************************************************/
uint8_t command_minstat(char *commandline, port_str *ptr){
    char buffer[60];
    int ret=0;
    ret = snprintf(buffer, sizeof(buffer),"Dropped frames        : %lu\r\n",telemetry.dropped_frames);
    send_buffer((uint8_t*)buffer,ret,ptr);
    ret = snprintf(buffer, sizeof(buffer),"Spurious acks         : %lu\r\n",telemetry.spurious_acks);
    send_buffer((uint8_t*)buffer,ret,ptr);
    ret = snprintf(buffer, sizeof(buffer),"Resets received       : %lu\r\n",telemetry.resets_received);
    send_buffer((uint8_t*)buffer,ret,ptr);
    ret = snprintf(buffer, sizeof(buffer),"Sequence mismatch drop: %lu\r\n",telemetry.sequence_mismatch_drop);
    send_buffer((uint8_t*)buffer,ret,ptr);
    ret = snprintf(buffer, sizeof(buffer),"Max frames in buffer  : %u\r\n",telemetry.min_frames_max);
    send_buffer((uint8_t*)buffer,ret,ptr);
    return 1; 
}

/*****************************************************************************
* Sends the configuration to teslaterm
******************************************************************************/
uint8_t command_config_get(char *commandline, port_str *ptr){
    char buffer[80];
	for (uint8_t current_parameter = 0; current_parameter < sizeof(confparam) / sizeof(parameter_entry); current_parameter++) {
        if(confparam[current_parameter].parameter_type == PARAM_CONFIG){
            print_param_buffer(buffer, confparam, current_parameter);
            send_config(buffer,confparam[current_parameter].help, ptr);
        }
    }
    send_config("NULL","NULL", ptr);
    return 1; 
}

/*****************************************************************************
* Kicks the controller into the bootloader
******************************************************************************/
uint8_t command_bootloader(char *commandline, port_str *ptr) {
	Bootloadable_Load();
	return 1;
}

/*****************************************************************************
* starts or stops the transient mode (classic mode)
* also spawns a timer for the burst mode.
******************************************************************************/
uint8_t command_tr(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    HELP_TEXT("Usage: tr [start|stop]\r\n");

	if (ntlibc_stricmp(commandline, "start") == 0) {
        interrupter.pw = param.pw;
		interrupter.prd = param.pwd;
        update_interrupter();

		tr_running = 1;
        
        callback_BurstFunction(NULL, 0, ptr);
        
		SEND_CONST_STRING("Transient Enabled\r\n", ptr);
       
        return 0;
	}
	if (ntlibc_stricmp(commandline, "stop") == 0) {
        interrupter.pw = 0;
		update_interrupter();
     
        SEND_CONST_STRING("\r\nTransient Disabled\r\n", ptr);    
 
		interrupter.pw = 0;
		update_interrupter();
		tr_running = 0;
		
		return 0;
	}
	return 1;
}

/*****************************************************************************
* Timer callback for the QCW autofire
******************************************************************************/
void vQCW_Timer_Callback(TimerHandle_t xTimer){
    if(!qcw_reg){
         	ramp.modulation_value = 20;
            qcw_reg = 1;
		    ramp_control(); 
    }
    if(param.qcw_repeat<100) param.qcw_repeat = 100;
    xTimerChangePeriod( xTimer, param.qcw_repeat / portTICK_PERIOD_MS, 0 );
}

/*****************************************************************************
* starts the QCW mode. Spawns a timer for the automatic QCW pulses.
******************************************************************************/
uint8_t command_qcw(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    HELP_TEXT("Usage: qcw [start|stop]\r\n");
    
	if (ntlibc_stricmp(commandline, "start") == 0) {
        if(param.qcw_repeat>99){
            if(xQCW_Timer==NULL){
                xQCW_Timer = xTimerCreate("QCW-Tmr", param.qcw_repeat / portTICK_PERIOD_MS, pdFALSE,(void * ) 0, vQCW_Timer_Callback);
                if(xQCW_Timer != NULL){
                    xTimerStart(xQCW_Timer, 0);
                    SEND_CONST_STRING("QCW Enabled\r\n", ptr);
                }else{
                    SEND_CONST_STRING("Cannot create QCW Timer\r\n", ptr);
                }
            }
        }else{
		    ramp.modulation_value = 20;
            qcw_reg = 1;
		    ramp_control();
            SEND_CONST_STRING("QCW single shot\r\n", ptr);
        }
		
		return 0;
	}
	if (ntlibc_stricmp(commandline, "stop") == 0) {
        if (xQCW_Timer != NULL) {
				if(xTimerDelete(xQCW_Timer, 200 / portTICK_PERIOD_MS) != pdFALSE){
				    xQCW_Timer = NULL;
                } else {
                    SEND_CONST_STRING("Cannot delete QCW Timer\r\n", ptr);
                }
		}
        QCW_enable_Control = 0;
        qcw_reg = 0;
		SEND_CONST_STRING("QCW Disabled\r\n", ptr);
		return 0;
	}
	return 1;
}

/*****************************************************************************
* sets the kill bit, stops the interrupter and switches the bus off
******************************************************************************/
uint8_t command_udkill(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    HELP_TEXT("Usage: kill [set|reset|get]\r\n");
    
    if (ntlibc_stricmp(commandline, "set") == 0) {
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
    	Term_Color_Green(ptr);
    	SEND_CONST_STRING("Killbit set\r\n", ptr);
    	Term_Color_White(ptr);
    }else if (ntlibc_stricmp(commandline, "reset") == 0) {
        interrupter_unkill();
        Term_Color_Green(ptr);
    	SEND_CONST_STRING("Killbit reset\r\n", ptr);
    	Term_Color_White(ptr);
        return 1;
    }else if (ntlibc_stricmp(commandline, "get") == 0) {
        char buf[30];
        Term_Color_Red(ptr);
        sprintf(buf, "Killbit: %u\r\n",interrupter_get_kill());
        send_string(buf,ptr);
        Term_Color_White(ptr);
    }
    
        
	return 0;
}

/*****************************************************************************
* commands a frequency sweep for the primary coil. It searches for a peak
* and makes a second run with +-6kHz around the peak
******************************************************************************/
uint8_t command_tune_p(char *commandline, port_str *ptr) {
    
    if(ptr->term_mode == PORT_TERM_TT){
        tsk_overlay_chart_stop();
        send_chart_clear(ptr);
        
    }
    
	run_adc_sweep(param.tune_start, param.tune_end, param.tune_pw, CT_PRIMARY, param.tune_delay, ptr);

	return 0;
}

/*****************************************************************************
* commands a frequency sweep for the secondary coil
******************************************************************************/
uint8_t command_tune_s(char *commandline, port_str *ptr) {
	run_adc_sweep(param.tune_start, param.tune_end, param.tune_pw, CT_SECONDARY, param.tune_delay, ptr);
	return 0;
}

/*****************************************************************************
* Prints the task list needs to be enabled in FreeRTOSConfig.h
* only use it for debugging reasons
******************************************************************************/
uint8_t command_tasks(char *commandline, port_str *ptr) {
    #if configUSE_STATS_FORMATTING_FUNCTIONS && configUSE_TRACE_FACILITY && configGENERATE_RUN_TIME_STATS
        char * buff = pvPortMalloc(sizeof(char)* 40 * uxTaskGetNumberOfTasks());
        if(buff == NULL){
            SEND_CONST_STRING("Malloc failed\r\n", ptr);
            return 0;
        }
        
	    SEND_CONST_STRING("**********************************************\n\r", ptr);
	    SEND_CONST_STRING("Task            State   Prio    Stack    Num\n\r", ptr);
	    SEND_CONST_STRING("**********************************************\n\r", ptr);
	    vTaskList(buff);
	    send_string(buff, ptr);
	    SEND_CONST_STRING("**********************************************\n\r\n\r", ptr);
        SEND_CONST_STRING("**********************************************\n\r", ptr);
	    SEND_CONST_STRING("Task            Abs time        % time\n\r", ptr);
	    SEND_CONST_STRING("**********************************************\n\r", ptr);
        vTaskGetRunTimeStats( buff );
        send_string(buff, ptr);
        SEND_CONST_STRING("**********************************************\n\r\r\n", ptr);
        sprintf(buff, "Free heap: %d\r\n",xPortGetFreeHeapSize());
        send_string(buff,ptr);
        vPortFree(buff);
        return 0;
    #endif
    
    #if !configUSE_STATS_FORMATTING_FUNCTIONS && !configUSE_TRACE_FACILITY
	    SEND_CONST_STRING("Taskinfo not active, activate it in FreeRTOSConfig.h\n\r", ptr);
        return 0;
	#endif
    
	
}

/*****************************************************************************
* Get a value from a parameter or print all parameters
******************************************************************************/
uint8_t command_get(char *commandline, port_str *ptr) {
	SKIP_SPACE(commandline);
    
	if (*commandline == 0 || commandline == 0) //no param --> show help text
	{
		print_param_help(confparam, PARAM_SIZE(confparam), ptr);
		return 1;
	}

	for (uint8_t current_parameter = 0; current_parameter < sizeof(confparam) / sizeof(parameter_entry); current_parameter++) {
		if (ntlibc_stricmp(commandline, confparam[current_parameter].name) == 0) {
			//Parameter found:
			print_param(confparam,current_parameter,ptr);
			return 1;
		}
	}
	Term_Color_Red(ptr);
	SEND_CONST_STRING("E: unknown param\r\n", ptr);
	Term_Color_White(ptr);
	return 0;
}

/*****************************************************************************
* Set a new value to a parameter
******************************************************************************/
uint8_t command_set(char *commandline, port_str *ptr) {
	SKIP_SPACE(commandline);
	char *param_value;

	if (commandline == NULL) {
		//if (!port)
		Term_Color_Red(ptr);
		SEND_CONST_STRING("E: no name\r\n", ptr);
		Term_Color_White(ptr);
		return 0;
	}

	param_value = strchr(commandline, ' ');
	if (param_value == NULL) {
		Term_Color_Red(ptr);
		SEND_CONST_STRING("E: no value\r\n", ptr);
		Term_Color_White(ptr);
		return 0;
	}

	*param_value = 0;
	param_value++;

	if (*param_value == '\0') {
		Term_Color_Red(ptr);
		SEND_CONST_STRING("E: no val\r\n", ptr);
		Term_Color_White(ptr);
		return 0;
	}

	for (uint8_t current_parameter = 0; current_parameter < sizeof(confparam) / sizeof(parameter_entry); current_parameter++) {
		if (ntlibc_stricmp(commandline, confparam[current_parameter].name) == 0) {
			//parameter name found:

			if (updateDefaultFunction(confparam, param_value,current_parameter, ptr)){
                if(confparam[current_parameter].callback_function){
                    if (confparam[current_parameter].callback_function(confparam, current_parameter, ptr)){
                        Term_Color_Green(ptr);
                        SEND_CONST_STRING("OK\r\n", ptr);
                        Term_Color_White(ptr);
                        return 1;
                    }else{
                        Term_Color_Red(ptr);
                        SEND_CONST_STRING("ERROR: Callback\r\n", ptr);
                        Term_Color_White(ptr);
                        return 1;
                    }
                }else{
                    Term_Color_Green(ptr);
                    SEND_CONST_STRING("OK\r\n", ptr);
                    Term_Color_White(ptr);
                    return 1;
                }
			} else {
				Term_Color_Red(ptr);
				SEND_CONST_STRING("NOK\r\n", ptr);
				Term_Color_White(ptr);
				return 1;
			}
		}
	}
	Term_Color_Red(ptr);
	SEND_CONST_STRING("E: unknown param\r\n", ptr);
	Term_Color_White(ptr);
	return 0;
}


void eeprom_load(port_str *ptr){
    EEPROM_read_conf(confparam, PARAM_SIZE(confparam) ,0,ptr);
    i2t_set_limit(configuration.max_const_i,configuration.max_fault_i,10000);
}

/*****************************************************************************
* Saves confparams to eeprom
******************************************************************************/
uint8_t command_eprom(char *commandline, port_str *ptr) {
	SKIP_SPACE(commandline);
    HELP_TEXT("Usage: eprom [load|save]\r\n");
    EEPROM_1_UpdateTemperature();
	uint8 sfflag = system_fault_Read();
	system_fault_Control = 0; //halt tesla coil operation during updates!
	if (ntlibc_stricmp(commandline, "save") == 0) {

	    EEPROM_write_conf(confparam, PARAM_SIZE(confparam),0, ptr);

		system_fault_Control = sfflag;
		return 0;
	}
	if (ntlibc_stricmp(commandline, "load") == 0) {
        uint8 sfflag = system_fault_Read();
        system_fault_Control = 0; //halt tesla coil operation during updates!
		EEPROM_read_conf(confparam, PARAM_SIZE(confparam) ,0,ptr);
        
        initialize_interrupter();
	    initialize_charging();
	    configure_ZCD_to_PWM();
	    system_fault_Control = sfflag;
		return 0;
	}
	return 1;
}

/*****************************************************************************
* Prints the help text
******************************************************************************/
uint8_t command_help(char *commandline, port_str *ptr) {
	UNUSED_VARIABLE(commandline);
	SEND_CONST_STRING("\r\nCommands:\r\n", ptr);
	for (uint8_t current_command = 0; current_command < (sizeof(commands) / sizeof(command_entry)); current_command++) {
		SEND_CONST_STRING("\t", ptr);
		Term_Color_Cyan(ptr);
		send_string((char *)commands[current_command].text, ptr);
		Term_Color_White(ptr);
		if (strlen(commands[current_command].text) > 7) {
			SEND_CONST_STRING("\t-> ", ptr);
		} else {
			SEND_CONST_STRING("\t\t-> ", ptr);
		}
		send_string((char *)commands[current_command].help, ptr);
		SEND_CONST_STRING("\r\n", ptr);
	}

	SEND_CONST_STRING("\r\nParameters:\r\n", ptr);
	for (uint8_t current_command = 0; current_command < sizeof(confparam) / sizeof(parameter_entry); current_command++) {
        if(confparam[current_command].parameter_type == PARAM_DEFAULT){
            SEND_CONST_STRING("\t", ptr);
            Term_Color_Cyan(ptr);
            send_string((char *)confparam[current_command].name, ptr);
            Term_Color_White(ptr);
            if (strlen(confparam[current_command].name) > 7) {
                SEND_CONST_STRING("\t-> ", ptr);
            } else {
                SEND_CONST_STRING("\t\t-> ", ptr);
            }

            send_string((char *)confparam[current_command].help, ptr);
            SEND_CONST_STRING("\r\n", ptr);
        }
	}

	SEND_CONST_STRING("\r\nConfiguration:\r\n", ptr);
	for (uint8_t current_command = 0; current_command < sizeof(confparam) / sizeof(parameter_entry); current_command++) {
        if(confparam[current_command].parameter_type == PARAM_CONFIG){
            SEND_CONST_STRING("\t", ptr);
            Term_Color_Cyan(ptr);
            send_string((char *)confparam[current_command].name, ptr);
            Term_Color_White(ptr);
            if (strlen(confparam[current_command].name) > 7) {
                SEND_CONST_STRING("\t-> ", ptr);
            } else {
                SEND_CONST_STRING("\t\t-> ", ptr);
            }

            send_string((char *)confparam[current_command].help, ptr);
            SEND_CONST_STRING("\r\n", ptr);
        }
	}

	return 0;
}


/*****************************************************************************
* Interprets the Input String
******************************************************************************/
void nt_interpret(char *text, port_str *ptr) {
    int8_t max_len=-1;
    int8_t max_index=-1;
    
    char* p_text=text;
    while(*p_text){
        if(*p_text==' ') break;
        *p_text=ntlibc_tolower(*p_text);
        p_text++;
    }
    
	for (uint8_t current_command = 0; current_command < (sizeof(commands) / sizeof(command_entry)); current_command++) {
        uint8_t text_len = strlen(commands[current_command].text);
		if (memcmp(text, commands[current_command].text, text_len) == 0) {
            if(text_len > max_len){
			    max_len = text_len;
                max_index = current_command;
            }
		}
	}
    
    if(max_index != -1){
       commands[max_index].commandFunction((char *)strchr(text, ' '), ptr);
       return;
        
    }
 
	if (*text) {
		Term_Color_Red(ptr);
		SEND_CONST_STRING("Unknown Command: ", ptr);
		send_string(text, ptr);
		SEND_CONST_STRING("\r\n", ptr);
		Term_Color_White(ptr);
	}
}


