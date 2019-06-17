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
#include "cli_commands.h"
#include "ZCDtoPWM.h"
#include "autotune.h"
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
#include "tasks/tsk_fault.h"
#include "min_id.h"
#include "helper/teslaterm.h"
#include "math.h"
#include "alarmevent.h"

#define UNUSED_VARIABLE(N) \
	do {                   \
		(void)(N);         \
	} while (0)
        


	
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
uint8_t callback_baudrateFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_VisibleFunction(parameter_entry * params, uint8_t index, port_str *ptr);


uint8_t burst_state = 0;

TimerHandle_t xQCW_Timer;
TimerHandle_t xBurst_Timer;

cli_config configuration;
cli_parameter param;

void eeprom_load(port_str *ptr){
    EEPROM_read_conf(confparam, PARAM_SIZE(confparam) ,0,ptr);
    i2t_set_limit(configuration.max_const_i,configuration.max_fault_i,10000);
}

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
    configuration.baudrate = 500000;
    configuration.r_top = 500000;
    ntlibc_strcpy(configuration.ud_name,"UD3-Tesla");
    ntlibc_strcpy(configuration.ip_addr,"192.168.50.250");
    ntlibc_strcpy(configuration.ip_subnet,"255.255.255.0");
    ntlibc_strcpy(configuration.ip_mac,"21:24:5D:AA:68:57");
    ntlibc_strcpy(configuration.ip_gw,"192.168.50.1");
    ntlibc_strcpy(configuration.ssid,"NULL");
    ntlibc_strcpy(configuration.passwd,"NULL");
    configuration.minprot = 1;
    configuration.max_const_i = 0;
    configuration.max_fault_i = 250;
    configuration.eth_hw = 0; //ESP32
    configuration.ct2_type = CT2_TYPE_CURRENT;
    configuration.ct2_voltage = 4000;
    configuration.ct2_offset = 0;
    configuration.ct2_current = 0;
    configuration.chargedelay = 1000;
    
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
    //       Parameter Type ,Visible      ,"Text   "         , Value ptr                     , Type          ,Min    ,Max    ,Div    ,Callback Function           ,Help text
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"pw"              , param.pw                      , TYPE_UNSIGNED ,0      ,800    ,0      ,callback_TRFunction         ,"Pulsewidth")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"pwd"             , param.pwd                     , TYPE_UNSIGNED ,0      ,60000  ,0      ,callback_TRFunction         ,"Pulsewidthdelay")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"bon"             , param.burst_on                , TYPE_UNSIGNED ,0      ,1000   ,0      ,callback_BurstFunction      ,"Burst mode ontime [ms] 0=off")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"boff"            , param.burst_off               , TYPE_UNSIGNED ,0      ,1000   ,0      ,callback_BurstFunction      ,"Burst mode offtime [ms]")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"tune_start"      , param.tune_start              , TYPE_UNSIGNED ,5      ,5000   ,10     ,callback_TuneFunction       ,"Start frequency [kHz]")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"tune_end"        , param.tune_end                , TYPE_UNSIGNED ,5      ,5000   ,10     ,callback_TuneFunction       ,"End frequency [kHz]")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"tune_pw"         , param.tune_pw                 , TYPE_UNSIGNED ,0      ,800    ,0      ,NULL                        ,"Tune pulsewidth")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"tune_delay"      , param.tune_delay              , TYPE_UNSIGNED ,1      ,200    ,0      ,NULL                        ,"Tune delay")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"offtime"         , param.offtime                 , TYPE_UNSIGNED ,2      ,250    ,0      ,callback_OfftimeFunction    ,"Offtime for MIDI")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"qcw_ramp"        , param.qcw_ramp                , TYPE_UNSIGNED ,1      ,10     ,0      ,NULL                        ,"QCW Ramp Inc/93us")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"qcw_repeat"      , param.qcw_repeat              , TYPE_UNSIGNED ,0      ,1000   ,0      ,NULL                        ,"QCW pulse repeat time [ms] <100=single shot")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"transpose"       , param.transpose               , TYPE_SIGNED   ,-48    ,48     ,0      ,NULL                        ,"Transpose MIDI")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"synth"           , param.synth                   , TYPE_UNSIGNED ,0      ,2      ,0      ,callback_SynthFunction      ,"0=off 1=MIDI 2=SID")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"watchdog"        , configuration.watchdog        , TYPE_UNSIGNED ,0      ,1      ,0      ,callback_ConfigFunction     ,"Watchdog Enable")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_tr_pw"       , configuration.max_tr_pw       , TYPE_UNSIGNED ,0      ,3000   ,0      ,callback_ConfigFunction     ,"Maximum TR PW [uSec]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_tr_prf"      , configuration.max_tr_prf      , TYPE_UNSIGNED ,0      ,3000   ,0      ,callback_ConfigFunction     ,"Maximum TR frequency [Hz]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_qcw_pw"      , configuration.max_qcw_pw      , TYPE_UNSIGNED ,0      ,30000  ,1000   ,callback_ConfigFunction     ,"Maximum QCW PW [ms]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_tr_current"  , configuration.max_tr_current  , TYPE_UNSIGNED ,0      ,2000   ,0      ,callback_ConfigFunction     ,"Maximum TR current [A]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"min_tr_current"  , configuration.min_tr_current  , TYPE_UNSIGNED ,0      ,2000   ,0      ,callback_ConfigFunction     ,"Minimum TR current [A]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_qcw_current" , configuration.max_qcw_current , TYPE_UNSIGNED ,0      ,2000   ,0      ,callback_ConfigFunction     ,"Maximum QCW current [A]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"temp1_max"       , configuration.temp1_max       , TYPE_UNSIGNED ,0      ,100    ,0      ,NULL                        ,"Max temperature 1 [*C]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"temp2_max"       , configuration.temp2_max       , TYPE_UNSIGNED ,0      ,100    ,0      ,NULL                        ,"Max temperature 2 [*C]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct1_ratio"       , configuration.ct1_ratio       , TYPE_UNSIGNED ,1      ,2000   ,0      ,callback_ConfigFunction     ,"CT1 [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct2_ratio"       , configuration.ct2_ratio       , TYPE_UNSIGNED ,1      ,5000   ,0      ,callback_ConfigFunction     ,"CT2 [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct3_ratio"       , configuration.ct3_ratio       , TYPE_UNSIGNED ,1      ,2000   ,0      ,callback_ConfigFunction     ,"CT3 [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct1_burden"      , configuration.ct1_burden      , TYPE_UNSIGNED ,1      ,1000   ,10     ,callback_ConfigFunction     ,"CT1 burden [Ohm]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct2_burden"      , configuration.ct2_burden      , TYPE_UNSIGNED ,1      ,1000   ,10     ,callback_ConfigFunction     ,"CT2 burden [Ohm]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct3_burden"      , configuration.ct3_burden      , TYPE_UNSIGNED ,1      ,1000   ,10     ,callback_ConfigFunction     ,"CT3 burden [Ohm]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct2_type"        , configuration.ct2_type        , TYPE_UNSIGNED ,0      ,1      ,0      ,callback_ConfigFunction     ,"CT2 type 0=current 1=voltage")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct2_current"     , configuration.ct2_current     , TYPE_UNSIGNED ,0      ,20000  ,10     ,callback_ConfigFunction     ,"CT2 current @ ct_voltage")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct2_voltage"     , configuration.ct2_voltage     , TYPE_UNSIGNED ,0      ,5000   ,1000   ,callback_ConfigFunction     ,"CT2 voltage @ ct_current")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct2_offset"      , configuration.ct2_offset      , TYPE_UNSIGNED ,0      ,5000   ,1000   ,callback_ConfigFunction     ,"CT2 offset voltage")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"lead_time"       , configuration.lead_time       , TYPE_UNSIGNED ,0      ,2000   ,0      ,callback_ConfigFunction     ,"Lead time [nSec]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"start_freq"      , configuration.start_freq      , TYPE_UNSIGNED ,0      ,5000   ,10     ,callback_ConfigFunction     ,"Resonant freq [kHz]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"start_cycles"    , configuration.start_cycles    , TYPE_UNSIGNED ,0      ,20     ,0      ,callback_ConfigFunction     ,"Start Cyles [N]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_tr_duty"     , configuration.max_tr_duty     , TYPE_UNSIGNED ,1      ,500    ,10     ,callback_ConfigFunction     ,"Max TR duty cycle [%]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_qcw_duty"    , configuration.max_qcw_duty    , TYPE_UNSIGNED ,1      ,500    ,10     ,callback_ConfigFunction     ,"Max QCW duty cycle [%]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"temp1_setpoint"  , configuration.temp1_setpoint  , TYPE_UNSIGNED ,0      ,100    ,0      ,NULL                        ,"Setpoint for fan [*C]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"batt_lockout_v"  , configuration.batt_lockout_v  , TYPE_UNSIGNED ,0      ,500    ,0      ,NULL                        ,"Battery lockout voltage [V]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"slr_fswitch"     , configuration.slr_fswitch     , TYPE_UNSIGNED ,0      ,1000   ,10     ,callback_ConfigFunction     ,"SLR switch frequency [kHz]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"slr_vbus"        , configuration.slr_vbus        , TYPE_UNSIGNED ,0      ,1000   ,0      ,NULL                        ,"SLR Vbus [V]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ps_scheme"       , configuration.ps_scheme       , TYPE_UNSIGNED ,0      ,5      ,0      ,callback_ConfigFunction     ,"Power supply sheme")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"autotune_s"      , configuration.autotune_s      , TYPE_UNSIGNED ,1      ,32     ,0      ,NULL                        ,"Number of samples for Autotune")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ud_name"         , configuration.ud_name         , TYPE_STRING   ,0      ,0      ,0      ,NULL                        ,"Name of the Coil [15 chars]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ip_addr"         , configuration.ip_addr         , TYPE_STRING   ,0      ,0      ,0      ,NULL                        ,"IP-Adress of the UD3")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ip_gateway"      , configuration.ip_gw           , TYPE_STRING   ,0      ,0      ,0      ,NULL                        ,"Gateway adress")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ip_subnet"       , configuration.ip_subnet       , TYPE_STRING   ,0      ,0      ,0      ,NULL                        ,"Subnet")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ip_mac"          , configuration.ip_mac          , TYPE_STRING   ,0      ,0      ,0      ,NULL                        ,"MAC adress")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"min_enable"      , configuration.minprot         , TYPE_UNSIGNED ,0      ,1      ,0      ,NULL                        ,"Use MIN-Protocol")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_const_i"     , configuration.max_const_i     , TYPE_UNSIGNED ,0      ,2000   ,10     ,callback_i2tFunction        ,"Maximum constant current [A] 0=off")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_fault_i"     , configuration.max_fault_i     , TYPE_UNSIGNED ,0      ,2000   ,10     ,callback_i2tFunction        ,"Maximum fault current for 10s [A]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"eth_hw"          , configuration.eth_hw          , TYPE_UNSIGNED ,0      ,2      ,0      ,callback_VisibleFunction    ,"0=Disabled 1=W5500 2=ESP32")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ssid"            , configuration.ssid            , TYPE_STRING   ,0      ,0      ,0      ,NULL                        ,"WLAN SSID")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"passwd"          , configuration.passwd          , TYPE_STRING   ,0      ,0      ,0      ,NULL                        ,"WLAN password")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"baudrate"        , configuration.baudrate        , TYPE_UNSIGNED ,1200   ,4000000,0      ,callback_baudrateFunction   ,"Serial baudrate")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"r_bus"           , configuration.r_top           , TYPE_UNSIGNED ,100    ,1000000,1000   ,NULL                        ,"Series resistor of voltage input [kOhm]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"charge_delay"    , configuration.chargedelay     , TYPE_UNSIGNED ,1      ,60000  ,0      ,callback_ConfigFunction     ,"Delay for the charge relay [ms]")
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
    ADD_COMMAND("signals"       ,command_signals        ,"For debugging")
    ADD_COMMAND("alarms"        ,command_alarms         ,"Alarms [show/reset]")
};

void update_visibilty(void){

    switch(configuration.eth_hw){
        case ETH_HW_DISABLED:
            set_visibility(confparam,CONF_SIZE, "ip_addr",VISIBLE_FALSE);
            set_visibility(confparam,CONF_SIZE, "ip_gateway",VISIBLE_FALSE);
            set_visibility(confparam,CONF_SIZE, "ip_subnet",VISIBLE_FALSE);
            set_visibility(confparam,CONF_SIZE, "ip_mac",VISIBLE_FALSE);
            set_visibility(confparam,CONF_SIZE, "ssid",VISIBLE_FALSE);
            set_visibility(confparam,CONF_SIZE, "passwd",VISIBLE_FALSE);
        break;
        case ETH_HW_W5500:
            set_visibility(confparam,CONF_SIZE, "ip_addr",VISIBLE_TRUE);
            set_visibility(confparam,CONF_SIZE, "ip_gateway",VISIBLE_TRUE);
            set_visibility(confparam,CONF_SIZE, "ip_subnet",VISIBLE_TRUE);
            set_visibility(confparam,CONF_SIZE, "ip_mac",VISIBLE_TRUE);
            set_visibility(confparam,CONF_SIZE, "ssid",VISIBLE_FALSE);
            set_visibility(confparam,CONF_SIZE, "passwd",VISIBLE_FALSE);
        break;
        case ETH_HW_ESP32:
            set_visibility(confparam,CONF_SIZE, "ip_addr",VISIBLE_FALSE);
            set_visibility(confparam,CONF_SIZE, "ip_gateway",VISIBLE_FALSE);
            set_visibility(confparam,CONF_SIZE, "ip_subnet",VISIBLE_FALSE);
            set_visibility(confparam,CONF_SIZE, "ip_mac",VISIBLE_FALSE);
            set_visibility(confparam,CONF_SIZE, "ssid",VISIBLE_TRUE);
            set_visibility(confparam,CONF_SIZE, "passwd",VISIBLE_TRUE);
        break;
    }
    switch(configuration.ct2_type){
        case CT2_TYPE_CURRENT:
            set_visibility(confparam,CONF_SIZE, "ct2_current",VISIBLE_FALSE);
            set_visibility(confparam,CONF_SIZE, "ct2_voltage",VISIBLE_FALSE);
            set_visibility(confparam,CONF_SIZE, "ct2_offset",VISIBLE_FALSE);
            set_visibility(confparam,CONF_SIZE, "ct2_ratio",VISIBLE_TRUE);
            set_visibility(confparam,CONF_SIZE, "ct2_burden",VISIBLE_TRUE);
        break;
        case CT2_TYPE_VOLTAGE:
            set_visibility(confparam,CONF_SIZE, "ct2_ratio",VISIBLE_FALSE);
            set_visibility(confparam,CONF_SIZE, "ct2_burden",VISIBLE_FALSE);
            set_visibility(confparam,CONF_SIZE, "ct2_current",VISIBLE_TRUE);
            set_visibility(confparam,CONF_SIZE, "ct2_voltage",VISIBLE_TRUE);
            set_visibility(confparam,CONF_SIZE, "ct2_offset",VISIBLE_TRUE);
        break;
    }
    
    
}

// clang-format on

/*****************************************************************************
* Callback if the offtime parameter is changed
******************************************************************************/
uint8_t callback_VisibleFunction(parameter_entry * params, uint8_t index, port_str *ptr){
    update_visibilty();
    return 1;
}

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
* Callback if the baudrate is changed
******************************************************************************/
void uart_baudrate(uint32_t baudrate){
    float divider = (float)(BCLK__BUS_CLK__HZ/8)/(float)baudrate;
   
    uint32_t down_rate = (BCLK__BUS_CLK__HZ/8)/floor(divider);
    uint32_t up_rate = (BCLK__BUS_CLK__HZ/8)/ceil(divider);
   
    float down_rate_error = (down_rate/(float)baudrate)-1;
    float up_rate_error = (up_rate/(float)baudrate)-1;
    
    UART_2_Stop();
    if(fabs(down_rate_error) < fabs(up_rate_error)){
        //selected round down divider
        UART_CLK_SetDividerValue(floor(divider));
    }else{
        //selected round up divider
        UART_CLK_SetDividerValue(ceil(divider));
    }
    UART_2_Start();    
    
}

uint8_t callback_baudrateFunction(parameter_entry * params, uint8_t index, port_str *ptr){
    uart_baudrate(configuration.baudrate);
 
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
    WD_enable(configuration.watchdog);
    initialize_interrupter();
	initialize_charging();
	configure_ZCD_to_PWM();
    update_visibilty();
    reconfig_charge_timer();
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


