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

uint8_t command_help(char *commandline, port_str *ptr);
uint8_t command_get(char *commandline, port_str *ptr);
uint8_t command_set(char *commandline, port_str *ptr);
uint8_t command_tr(char *commandline, port_str *ptr);
uint8_t command_udkill(char *commandline, port_str *ptr);
uint8_t command_eprom(char *commandline, port_str *ptr);
uint8_t command_status(char *commandline, port_str *ptr);
uint8_t command_tune(char *commandline, port_str *ptr);
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
uint8_t command_signals(char *commandline, port_str *ptr);
uint8_t command_alarms(char *commandline, port_str *ptr);

uint8_t callback_ConfigFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_DefaultFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_TuneFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_TTupdateFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_TRFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_OfftimeFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_BurstFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_SynthFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_i2tFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_baudrateFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_SPIspeedFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_VisibleFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_MchFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_MchCopyFunction(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_ivoUART(parameter_entry * params, uint8_t index, port_str *ptr);
uint8_t callback_SIDfreq(parameter_entry * params, uint8_t index, port_str *ptr);

void update_ivo_uart();
void init_tt(uint8_t with_chart, port_str *ptr);


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
    configuration.ps_scheme = 2;
    configuration.autotune_s = 1;
    configuration.baudrate = 500000;
    configuration.spi_speed = 16;
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
    configuration.ivo_uart = 0;
    
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
    param.mch = 0;
    param.synth = SYNTH_MIDI;
    param.sid_freq = 50;
    param.sid_divider = 212; //divider valid for 50Hz
    
    i2t_set_limit(configuration.max_const_i,configuration.max_fault_i,10000);
    update_ivo_uart();
}

// clang-format off

/*****************************************************************************
* Parameter struct
******************************************************************************/

parameter_entry confparam[] = {
    //       Parameter Type ,Visible      ,"Text   "         , Value ptr                     ,Min     ,Max    ,Div    ,Callback Function           ,Help text
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"pw"              , param.pw                      , 0      ,800    ,0      ,callback_TRFunction         ,"Pulsewidth")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"pwd"             , param.pwd                     , 0      ,60000  ,0      ,callback_TRFunction         ,"Pulsewidthdelay")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"bon"             , param.burst_on                , 0      ,1000   ,0      ,callback_BurstFunction      ,"Burst mode ontime [ms] 0=off")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"boff"            , param.burst_off               , 0      ,1000   ,0      ,callback_BurstFunction      ,"Burst mode offtime [ms]")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"tune_start"      , param.tune_start              , 5      ,5000   ,10     ,callback_TuneFunction       ,"Start frequency [kHz]")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"tune_end"        , param.tune_end                , 5      ,5000   ,10     ,callback_TuneFunction       ,"End frequency [kHz]")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"tune_pw"         , param.tune_pw                 , 0      ,800    ,0      ,NULL                        ,"Tune pulsewidth")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"tune_delay"      , param.tune_delay              , 1      ,200    ,0      ,NULL                        ,"Tune delay")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"offtime"         , param.offtime                 , 2      ,250    ,0      ,callback_OfftimeFunction    ,"Offtime for MIDI")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"qcw_ramp"        , param.qcw_ramp                , 1      ,10     ,0      ,NULL                        ,"QCW Ramp Inc/93us")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"qcw_repeat"      , param.qcw_repeat              , 0      ,1000   ,0      ,NULL                        ,"QCW pulse repeat time [ms] <100=single shot")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"transpose"       , param.transpose               , -48    ,48     ,0      ,NULL                        ,"Transpose MIDI")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"synth"           , param.synth                   , 0      ,2      ,0      ,callback_SynthFunction      ,"0=off 1=MIDI 2=SID")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"mch"             , param.mch                     , 0      ,16     ,0      ,callback_MchFunction        ,"MIDI channel 16=write to all channels")
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"attack"          , midich[0].attack              , 0      ,127    ,0      ,callback_MchCopyFunction    ,"MIDI attack time of mch") //WARNING: Must be mch index +1
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"decay"           , midich[0].decay               , 0      ,127    ,0      ,callback_MchCopyFunction    ,"MIDI decay time of mch")  //WARNING: Must be mch index +2
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"release"         , midich[0].release             , 0      ,127    ,0      ,callback_MchCopyFunction    ,"MIDI release time of mch")//WARNING: Must be mch index +3
    ADD_PARAM(PARAM_DEFAULT ,VISIBLE_TRUE ,"sid_freq"        , param.sid_freq                , 45     ,100    ,0      ,callback_SIDfreq           ,"SID frame interrupt frequency")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"watchdog"        , configuration.watchdog        , 0      ,1      ,0      ,callback_ConfigFunction     ,"Watchdog Enable")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_tr_pw"       , configuration.max_tr_pw       , 0      ,3000   ,0      ,callback_ConfigFunction     ,"Maximum TR PW [uSec]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_tr_prf"      , configuration.max_tr_prf      , 0      ,3000   ,0      ,callback_ConfigFunction     ,"Maximum TR frequency [Hz]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_qcw_pw"      , configuration.max_qcw_pw      , 0      ,30000  ,1000   ,callback_ConfigFunction     ,"Maximum QCW PW [ms]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_tr_current"  , configuration.max_tr_current  , 0      ,8000   ,0      ,callback_TTupdateFunction   ,"Maximum TR current [A]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"min_tr_current"  , configuration.min_tr_current  , 0      ,8000   ,0      ,callback_ConfigFunction     ,"Minimum TR current [A]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_qcw_current" , configuration.max_qcw_current , 0      ,8000   ,0      ,callback_TTupdateFunction   ,"Maximum QCW current [A]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"temp1_max"       , configuration.temp1_max       , 0      ,100    ,0      ,callback_TTupdateFunction   ,"Max temperature 1 [*C]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"temp2_max"       , configuration.temp2_max       , 0      ,100    ,0      ,NULL                        ,"Max temperature 2 [*C]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct1_ratio"       , configuration.ct1_ratio       , 1      ,5000   ,0      ,callback_TTupdateFunction   ,"CT1 [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct2_ratio"       , configuration.ct2_ratio       , 1      ,5000   ,0      ,callback_ConfigFunction     ,"CT2 [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct3_ratio"       , configuration.ct3_ratio       , 1      ,5000   ,0      ,callback_ConfigFunction     ,"CT3 [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct1_burden"      , configuration.ct1_burden      , 1      ,1000   ,10     ,callback_TTupdateFunction   ,"CT1 burden [Ohm]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct2_burden"      , configuration.ct2_burden      , 1      ,1000   ,10     ,callback_ConfigFunction     ,"CT2 burden [Ohm]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct3_burden"      , configuration.ct3_burden      , 1      ,1000   ,10     ,callback_ConfigFunction     ,"CT3 burden [Ohm]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct2_type"        , configuration.ct2_type        , 0      ,1      ,0      ,callback_ConfigFunction     ,"CT2 type 0=current 1=voltage")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct2_current"     , configuration.ct2_current     , 0      ,20000  ,10     ,callback_ConfigFunction     ,"CT2 current @ ct_voltage")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct2_voltage"     , configuration.ct2_voltage     , 0      ,5000   ,1000   ,callback_ConfigFunction     ,"CT2 voltage @ ct_current")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ct2_offset"      , configuration.ct2_offset      , 0      ,5000   ,1000   ,callback_ConfigFunction     ,"CT2 offset voltage")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"lead_time"       , configuration.lead_time       , 0      ,2000   ,0      ,callback_ConfigFunction     ,"Lead time [nSec]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"start_freq"      , configuration.start_freq      , 0      ,5000   ,10     ,callback_ConfigFunction     ,"Resonant freq [kHz]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"start_cycles"    , configuration.start_cycles    , 0      ,20     ,0      ,callback_ConfigFunction     ,"Start Cyles [N]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_tr_duty"     , configuration.max_tr_duty     , 1      ,500    ,10     ,callback_ConfigFunction     ,"Max TR duty cycle [%]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_qcw_duty"    , configuration.max_qcw_duty    , 1      ,500    ,10     ,callback_ConfigFunction     ,"Max QCW duty cycle [%]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"temp1_setpoint"  , configuration.temp1_setpoint  , 0      ,100    ,0      ,NULL                        ,"Setpoint for fan [*C]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ps_scheme"       , configuration.ps_scheme       , 0      ,5      ,0      ,callback_ConfigFunction     ,"Power supply sheme")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"charge_delay"    , configuration.chargedelay     , 1      ,60000  ,0      ,callback_ConfigFunction     ,"Delay for the charge relay [ms]")  
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"autotune_s"      , configuration.autotune_s      , 1      ,32     ,0      ,NULL                        ,"Number of samples for Autotune")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ud_name"         , configuration.ud_name         , 0      ,0      ,0      ,NULL                        ,"Name of the Coil [15 chars]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ip_addr"         , configuration.ip_addr         , 0      ,0      ,0      ,NULL                        ,"IP-Adress of the UD3")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ip_gateway"      , configuration.ip_gw           , 0      ,0      ,0      ,NULL                        ,"Gateway adress")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ip_subnet"       , configuration.ip_subnet       , 0      ,0      ,0      ,NULL                        ,"Subnet")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ip_mac"          , configuration.ip_mac          , 0      ,0      ,0      ,NULL                        ,"MAC adress")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"min_enable"      , configuration.minprot         , 0      ,1      ,0      ,NULL                        ,"Use MIN-Protocol")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_const_i"     , configuration.max_const_i     , 0      ,2000   ,10     ,callback_i2tFunction        ,"Maximum constant current [A] 0=off")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"max_fault_i"     , configuration.max_fault_i     , 0      ,2000   ,10     ,callback_i2tFunction        ,"Maximum fault current for 10s [A]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"eth_hw"          , configuration.eth_hw          , 0      ,2      ,0      ,callback_VisibleFunction    ,"0=Disabled 1=W5500 2=ESP32")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ssid"            , configuration.ssid            , 0      ,0      ,0      ,NULL                        ,"WLAN SSID")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"passwd"          , configuration.passwd          , 0      ,0      ,0      ,NULL                        ,"WLAN password")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"baudrate"        , configuration.baudrate        , 1200   ,4000000,0      ,callback_baudrateFunction   ,"Serial baudrate")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"ivo_uart"        , configuration.ivo_uart        , 0      ,11     ,0      ,callback_ivoUART            ,"[RX][TX] 0=not inverted 1=inverted")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"spi_speed"       , configuration.spi_speed       , 10     ,160    ,10     ,callback_SPIspeedFunction   ,"SPI speed [MHz]")
    ADD_PARAM(PARAM_CONFIG  ,VISIBLE_TRUE ,"r_bus"           , configuration.r_top           , 100    ,1000000,1000   ,callback_TTupdateFunction   ,"Series resistor of voltage input [kOhm]")
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
    ADD_COMMAND("tune"	        ,command_tune           ,"Autotune [prim/sec]")
    ADD_COMMAND("tterm"	        ,command_tterm          ,"Changes terminal mode")
    ADD_COMMAND("minstat"	    ,command_minstat        ,"Prints the min statistics")
    ADD_COMMAND("ethcon"	    ,command_ethcon         ,"Prints the eth connections")
    ADD_COMMAND("config_get"    ,command_config_get     ,"Internal use")
    ADD_COMMAND("fuse_reset"    ,command_fuse           ,"Reset the internal fuse")
    ADD_COMMAND("signals"       ,command_signals        ,"For debugging")
    ADD_COMMAND("alarms"        ,command_alarms         ,"Alarms [get/roll/reset]")
    ADD_COMMAND("synthmon"      ,command_SynthMon       ,"Synthesizer status")
};


void eeprom_load(port_str *ptr){
    EEPROM_read_conf(confparam, PARAM_SIZE(confparam) ,0,ptr);
    i2t_set_limit(configuration.max_const_i,configuration.max_fault_i,10000);
    update_ivo_uart();
    update_visibilty();
    uart_baudrate(configuration.baudrate);
    spi_speed(configuration.spi_speed);
}



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
* Callback for SID frequency change
******************************************************************************/

uint8_t callback_SIDfreq(parameter_entry * params, uint8_t index, port_str *ptr){
    param.sid_divider = floor((32000.0/3.0)/(float)param.sid_freq);
    return 1;
}

/*****************************************************************************
* Callback for invert option UART
******************************************************************************/

void update_ivo_uart(){
    switch(configuration.ivo_uart){
    case 0:
        IVO_UART_Control = 0b00;
        break;
    case 1:
        IVO_UART_Control = 0b10;
        break;
    case 10:
        IVO_UART_Control = 0b01;
        break;
    case 11:
        IVO_UART_Control = 0b11;
        break;
    }
}

uint8_t callback_ivoUART(parameter_entry * params, uint8_t index, port_str *ptr){
    if(configuration.ivo_uart == 0 || configuration.ivo_uart == 1 || configuration.ivo_uart == 10 || configuration.ivo_uart == 11){
        update_ivo_uart();   
        return 1;
    }else{
        SEND_CONST_STRING("Only the folowing combinations are allowed\r\n", ptr);
        SEND_CONST_STRING("0  = no inversion\r\n", ptr);
        SEND_CONST_STRING("1  = rx inverted, tx not inverted\r\n", ptr);
        SEND_CONST_STRING("10 = rx not inverted, tx inverted\r\n", ptr);
        SEND_CONST_STRING("11 = rx and tx inverted\r\n", ptr);
        return 0;
    }
}

/*****************************************************************************
* Callback if the MIDI channel is changed
******************************************************************************/
uint8_t callback_MchFunction(parameter_entry * params, uint8_t index, port_str *ptr){
    if(param.mch<16){
        params[index+1].value = &midich[param.mch].attack;
        params[index+2].value = &midich[param.mch].decay;
        params[index+3].value = &midich[param.mch].release;
    }else{
        params[index+1].value = &midich[0].attack;
        params[index+2].value = &midich[0].decay;
        params[index+3].value = &midich[0].release;
    }
    return 1;
}

/*****************************************************************************
* Callback for mch==16
******************************************************************************/
uint8_t callback_MchCopyFunction(parameter_entry * params, uint8_t index, port_str *ptr){
    if(param.mch<16) return 1;

    for(uint8_t i=1;i<16;i++){
        midich[i].attack = midich[0].attack;
        midich[i].decay = midich[0].decay;
        midich[i].release = midich[0].release;
    }

    return 1;
}

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
* Callback if the maximum current is changed
******************************************************************************/
uint8_t callback_TTupdateFunction(parameter_entry * params, uint8_t index, port_str *ptr) {
    
    uint32_t max_current_cmp = ((4080ul * configuration.ct1_ratio) / configuration.ct1_burden)/100; //Amperes
    uint32_t max_current_meas = ((5000ul * configuration.ct1_ratio) / configuration.ct1_burden)/100; //Amperes
    
    if(configuration.max_tr_current>max_current_cmp || configuration.max_qcw_current>max_current_cmp){
        char buffer[120];
        snprintf(buffer, sizeof(buffer),"Warning: Max CT1 current with the current setup is %uA for OCD and %uA for peak measurement\r\n",max_current_cmp,max_current_meas);
        send_string(buffer, ptr);
        if(configuration.max_tr_current>max_current_cmp) configuration.max_tr_current = max_current_cmp;
        if(configuration.max_qcw_current>max_current_cmp) configuration.max_qcw_current = max_current_cmp;
    }
    uint8 sfflag = system_fault_Read();
    system_fault_Control = 0; //halt tesla coil operation during updates!
    
    configure_ZCD_to_PWM();
    
    system_fault_Control = sfflag;
    
    if (ptr->term_mode!=PORT_TERM_VT100) {
        uint8_t include_chart;
        if (ptr->term_mode==PORT_TERM_TT) {
            include_chart = pdTRUE;
        } else {
            include_chart = pdFALSE;
        }
        init_tt(include_chart,ptr);
    }
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
* Callback if the SPI speed is changed
******************************************************************************/
void spi_speed(uint32_t speed){
    speed = speed * 100000;
    float divider = (float)(BCLK__BUS_CLK__HZ/8)/(float)speed;
   
    uint32_t down_rate = (BCLK__BUS_CLK__HZ/8)/floor(divider);
    uint32_t up_rate = (BCLK__BUS_CLK__HZ/8)/ceil(divider);
   
    float down_rate_error = (down_rate/(float)speed)-1;
    float up_rate_error = (up_rate/(float)speed)-1;
    
    SPIM0_Stop();
    if(fabs(down_rate_error) < fabs(up_rate_error)){
        //selected round down divider
        SPI_CLK_SetDividerValue(floor(divider));
    }else{
        //selected round up divider
        SPI_CLK_SetDividerValue(ceil(divider));
    }
    SPIM0_Start(); 
    
}

uint8_t callback_SPIspeedFunction(parameter_entry * params, uint8_t index, port_str *ptr){
    spi_speed(configuration.spi_speed);
 
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

#define BURST_ON 0
#define BURST_OFF 1

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
* Clears the terminal screen and displays the logo
******************************************************************************/
static const uint8_t c_A = 9;
static const uint8_t c_B = 15;
static const uint8_t c_C = 35;

void print_alarm(ALARMS *temp,port_str *ptr){
    uint16_t ret;
    char buffer[80];
    Term_Move_cursor_right(c_A,ptr);
    ret = snprintf(buffer, sizeof(buffer),"%u",temp->num);
    send_buffer((uint8_t*)buffer,ret,ptr); 
    
    Term_Move_cursor_right(c_B,ptr);
    ret = snprintf(buffer, sizeof(buffer),"| %u ms",temp->timestamp);
    send_buffer((uint8_t*)buffer,ret,ptr); 
    
    Term_Move_cursor_right(c_C,ptr);
    send_char('|',ptr);
    switch(temp->alarm_level){
        case ALM_PRIO_INFO:
            Term_Color_Green(ptr);
        break;
        case ALM_PRIO_WARN:
            Term_Color_White(ptr);
        break;
        case ALM_PRIO_ALARM:
            Term_Color_Cyan(ptr);
        break;
        case ALM_PRIO_CRITICAL:
            Term_Color_Red(ptr);
        break;
    }
    if(temp->value==ALM_NO_VALUE){
        ret = snprintf(buffer, sizeof(buffer)," %s\r\n",temp->message);
    }else{
        ret = snprintf(buffer, sizeof(buffer)," %s | Value: %i\r\n",temp->message ,temp->value);
    }
    send_buffer((uint8_t*)buffer,ret,ptr); 
    Term_Color_White(ptr);
}

uint8_t command_alarms(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    CHECK_NULL(commandline);

    ALARMS temp;
    
    if (ntlibc_stricmp(commandline, "get") == 0) {

        Term_Move_cursor_right(c_A,ptr);
        SEND_CONST_STRING("Number", ptr);
        Term_Move_cursor_right(c_B,ptr);
        SEND_CONST_STRING("| Timestamp", ptr);
        Term_Move_cursor_right(c_C,ptr);
        SEND_CONST_STRING("| Message\r\n", ptr);
        
        for(uint16_t i=0;i<alarm_get_num();i++){
            if(alarm_get(i,&temp)==pdPASS){
                print_alarm(&temp,ptr);
            }
        }
    	return 1;
    }
    if (ntlibc_stricmp(commandline, "roll") == 0) {
        Term_Move_cursor_right(c_A,ptr);
        SEND_CONST_STRING("Number", ptr);
        Term_Move_cursor_right(c_B,ptr);
        SEND_CONST_STRING("| Timestamp", ptr);
        Term_Move_cursor_right(c_C,ptr);
        SEND_CONST_STRING("| Message\r\n", ptr);
        uint32_t old_num=0;
        for(uint16_t i=0;i<alarm_get_num();i++){
            if(alarm_get(i,&temp)==pdPASS){
                print_alarm(&temp,ptr);
            }
            old_num=temp.num;
        }
        while(getch(ptr,50 /portTICK_RATE_MS) != 'q'){
           if(alarm_get(alarm_get_num()-1,&temp)==pdPASS){
                if(temp.num>old_num){
                    print_alarm(&temp,ptr);
                }
                old_num=temp.num;
            } 
        }
        return 1;
    }
    if (ntlibc_stricmp(commandline, "reset") == 0) {
        alarm_clear();
        SEND_CONST_STRING("Alarms reset...\r\n", ptr);
        return 1;
    }
    
    HELP_TEXT("Usage: alarms [get|roll|reset]\r\n");
}

/*****************************************************************************
* Displays the statistics of the min protocol
******************************************************************************/
uint8_t command_minstat(char *commandline, port_str *ptr){

    char buffer[60];
    int ret=0;
    ret = snprintf(buffer, sizeof(buffer),"Dropped frames        : %lu\r\n",min_ctx.transport_fifo.dropped_frames);
    send_buffer((uint8_t*)buffer,ret,ptr);
    ret = snprintf(buffer, sizeof(buffer),"Spurious acks         : %lu\r\n",min_ctx.transport_fifo.spurious_acks);
    send_buffer((uint8_t*)buffer,ret,ptr);
    ret = snprintf(buffer, sizeof(buffer),"Resets received       : %lu\r\n",min_ctx.transport_fifo.resets_received);
    send_buffer((uint8_t*)buffer,ret,ptr);
    ret = snprintf(buffer, sizeof(buffer),"Sequence mismatch drop: %lu\r\n",min_ctx.transport_fifo.sequence_mismatch_drop);
    send_buffer((uint8_t*)buffer,ret,ptr);
    ret = snprintf(buffer, sizeof(buffer),"Max frames in buffer  : %u\r\n",min_ctx.transport_fifo.n_frames_max);
    send_buffer((uint8_t*)buffer,ret,ptr);
    ret = snprintf(buffer, sizeof(buffer),"CRC errors            : %u\r\n",min_ctx.transport_fifo.crc_fails);
    send_buffer((uint8_t*)buffer,ret,ptr);
    return 1; 
}

/*****************************************************************************
* Sends the configuration to teslaterm
******************************************************************************/
uint8_t command_config_get(char *commandline, port_str *ptr){
    char buffer[80];
	for (uint8_t current_parameter = 0; current_parameter < sizeof(confparam) / sizeof(parameter_entry); current_parameter++) {
        if(confparam[current_parameter].parameter_type == PARAM_CONFIG  && confparam[current_parameter].visible){
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
    CHECK_NULL(commandline);
    
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
        
        if (xBurst_Timer != NULL) {
			if(xTimerDelete(xBurst_Timer, 100 / portTICK_PERIOD_MS) != pdFALSE){
			    xBurst_Timer = NULL;
                burst_state = BURST_ON;
            }
        }

        SEND_CONST_STRING("Transient Disabled\r\n", ptr);    
 
		interrupter.pw = 0;
		update_interrupter();
		tr_running = 0;
		
		return 0;
	}
	HELP_TEXT("Usage: tr [start|stop]\r\n");
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
    CHECK_NULL(commandline);
        
	if (ntlibc_stricmp(commandline, "start") == 0) {
        switch_synth(SYNTH_MIDI);
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
        switch_synth(param.synth);
		return 0;
	}
	HELP_TEXT("Usage: qcw [start|stop]\r\n");
}

/*****************************************************************************
* sets the kill bit, stops the interrupter and switches the bus off
******************************************************************************/
uint8_t command_udkill(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    CHECK_NULL(commandline);
    
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
        alarm_push(ALM_PRIO_CRITICAL,warn_kill_set, ALM_NO_VALUE);
    	Term_Color_White(ptr);
        return 1;
    }else if (ntlibc_stricmp(commandline, "reset") == 0) {
        interrupter_unkill();
        reset_fault();
        system_fault_Control = 0xFF;
        Term_Color_Green(ptr);
    	SEND_CONST_STRING("Killbit reset\r\n", ptr);
        alarm_push(ALM_PRIO_INFO,warn_kill_reset, ALM_NO_VALUE);
    	Term_Color_White(ptr);
        return 1;
    }else if (ntlibc_stricmp(commandline, "get") == 0) {
        char buf[30];
        Term_Color_Red(ptr);
        int ret = snprintf(buf,sizeof(buf), "Killbit: %u\r\n",interrupter_get_kill());
        send_buffer((uint8_t*)buf,ret,ptr);
        Term_Color_White(ptr);
        return 1;
    }
    
        
    HELP_TEXT("Usage: kill [set|reset|get]\r\n");
}

/*****************************************************************************
* commands a frequency sweep for the primary coil. It searches for a peak
* and makes a second run with +-6kHz around the peak
******************************************************************************/
uint8_t command_tune(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    CHECK_NULL(commandline);
    
    
    SEND_CONST_STRING("Warning: The bridge will switch hard, make sure that the bus voltage is appropriate\r\n",ptr);
    SEND_CONST_STRING("Press [y] to proceed\r\n",ptr);
    
    xSemaphoreGive(ptr->term_block);
    if(getch(ptr, portMAX_DELAY) != 'y'){
        xSemaphoreTake(ptr->term_block, portMAX_DELAY);
        SEND_CONST_STRING("Tune aborted\r\n",ptr);
        return 0;
    }
    xSemaphoreTake(ptr->term_block, portMAX_DELAY);
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




/*****************************************************************************
* Saves confparams to eeprom
******************************************************************************/
uint8_t command_eprom(char *commandline, port_str *ptr) {
	SKIP_SPACE(commandline);
    CHECK_NULL(commandline);
    
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
	HELP_TEXT("Usage: eprom [load|save]\r\n");
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
        if(confparam[current_command].parameter_type == PARAM_DEFAULT && confparam[current_command].visible){
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
        if(confparam[current_command].parameter_type == PARAM_CONFIG && confparam[current_command].visible){
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
* Switches the bus on/off
******************************************************************************/
uint8_t command_bus(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    CHECK_NULL(commandline);

	if (ntlibc_stricmp(commandline, "on") == 0) {
		bus_command = BUS_COMMAND_ON;
		SEND_CONST_STRING("BUS ON\r\n", ptr);
        return 1;
	}
	if (ntlibc_stricmp(commandline, "off") == 0) {
		bus_command = BUS_COMMAND_OFF;
		SEND_CONST_STRING("BUS OFF\r\n", ptr);
        return 1;
	}
    
    HELP_TEXT("Usage: bus [on|off]\r\n");
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
        case PORT_TYPE_ETH:
        	xTaskCreate(tsk_overlay_TaskProc, "Overl_E", STACK_OVERLAY, ptr, PRIO_OVERLAY, &ptr->telemetry_handle);
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

/*****************************************************************************
* Initializes the teslaterm telemetry
* Spawns the overlay task for telemetry stream generation
******************************************************************************/
void init_tt(uint8_t with_chart, port_str *ptr){
    send_gauge_config(0, GAUGE0_MIN, GAUGE0_MAX, GAUGE0_NAME, ptr);
    send_gauge_config(1, GAUGE1_MIN, GAUGE1_MAX, GAUGE1_NAME, ptr);
    send_gauge_config(2, GAUGE2_MIN, GAUGE2_MAX, GAUGE2_NAME, ptr);
    send_gauge_config(3, GAUGE3_MIN, GAUGE3_MAX, GAUGE3_NAME, ptr);
    send_gauge_config(4, GAUGE4_MIN, GAUGE4_MAX, GAUGE4_NAME, ptr);
    send_gauge_config(5, GAUGE5_MIN, GAUGE5_MAX, GAUGE5_NAME, ptr);
    send_gauge_config(6, GAUGE6_MIN, GAUGE6_MAX, GAUGE6_NAME, ptr);
    
    if(with_chart==pdTRUE){
        send_chart_config(0, CHART0_MIN, CHART0_MAX, CHART0_OFFSET, CHART0_UNIT, CHART0_NAME, ptr);
        send_chart_config(1, CHART1_MIN, CHART1_MAX, CHART1_OFFSET, CHART1_UNIT, CHART1_NAME, ptr);
        send_chart_config(2, CHART2_MIN, CHART2_MAX, CHART2_OFFSET, CHART2_UNIT, CHART2_NAME, ptr);
        send_chart_config(3, CHART3_MIN, CHART3_MAX, CHART3_OFFSET, CHART3_UNIT, CHART3_NAME, ptr);
    }
    start_overlay_task(ptr);
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
	if (ntlibc_stricmp(commandline, "stop") == 0) {
        ptr->term_mode = PORT_TERM_VT100;
        stop_overlay_task(ptr);
        return 1;
	} 
    
    HELP_TEXT("Usage: tterm [start|stop|mqtt]\r\n");
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
* Signal debugging
******************************************************************************/

void send_signal_state(uint8_t signal, uint8_t inverted, port_str *ptr){
    if(inverted) signal = !signal; 
    if(signal){
        Term_Color_Red(ptr);
        SEND_CONST_STRING("true \r\n",ptr);
        Term_Color_White(ptr);  
    }else{
        Term_Color_Green(ptr);
        SEND_CONST_STRING("false\r\n",ptr);
        Term_Color_White(ptr);
    }
}
void send_signal_state_wo(uint8_t signal, uint8_t inverted, port_str *ptr){
    if(inverted) signal = !signal; 
    if(signal){
        Term_Color_Red(ptr);
        SEND_CONST_STRING("true ",ptr);
        Term_Color_White(ptr);  
    }else{
        Term_Color_Green(ptr);
        SEND_CONST_STRING("false",ptr);
        Term_Color_White(ptr);
    }
}

uint8_t command_signals(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    char buffer[80];
    Term_Disable_Cursor(ptr);
    Term_Erase_Screen(ptr);
    xSemaphoreGive(ptr->term_block);
    while(getch(ptr,250 /portTICK_RATE_MS) != 'q'){
        xSemaphoreTake(ptr->term_block, portMAX_DELAY);
        Term_Move_Cursor(1,1,ptr);
        SEND_CONST_STRING("Signal state (q for quit):\r\n", ptr);
        SEND_CONST_STRING("**************************\r\n", ptr);
        SEND_CONST_STRING("UVLO pin: ", ptr);
        send_signal_state(UVLO_status_Status,pdTRUE,ptr);
        
        SEND_CONST_STRING("Sysfault driver undervoltage: ", ptr);
        send_signal_state(sysfault.uvlo,pdFALSE,ptr);
        SEND_CONST_STRING("Sysfault Temp 1: ", ptr);
        send_signal_state_wo(sysfault.temp1,pdFALSE,ptr);
        SEND_CONST_STRING(" Temp 2: ", ptr);
        send_signal_state(sysfault.temp2,pdFALSE,ptr);
        SEND_CONST_STRING("Sysfault fuse: ", ptr);
        send_signal_state_wo(sysfault.fuse,pdFALSE,ptr);
        SEND_CONST_STRING(" charging: ", ptr);
        send_signal_state(sysfault.charge,pdFALSE,ptr);
        SEND_CONST_STRING("Sysfault watchdog: ", ptr);
        send_signal_state_wo(sysfault.watchdog,pdFALSE,ptr);
        SEND_CONST_STRING(" updating: ", ptr);
        send_signal_state(sysfault.update,pdFALSE,ptr);
        SEND_CONST_STRING("Sysfault bus undervoltage: ", ptr);
        send_signal_state(sysfault.bus_uv,pdFALSE,ptr);
        SEND_CONST_STRING("Sysfault combined: ", ptr);
        send_signal_state(system_fault_Read(),pdTRUE,ptr);
        
        SEND_CONST_STRING("Relay 1: ", ptr);
        send_signal_state_wo((relay_Read()&0b1),pdFALSE,ptr);
        SEND_CONST_STRING(" Relay 2: ", ptr);
        send_signal_state_wo((relay_Read()&0b10),pdFALSE,ptr);
        SEND_CONST_STRING(" Fan: ", ptr);
        send_signal_state(fan_ctrl_Read(),pdFALSE,ptr);
        SEND_CONST_STRING("                                    \r", ptr);
        SEND_CONST_STRING("Bus status: ", ptr);
        Term_Color_Cyan(ptr);
        switch(telemetry.bus_status){
            case BUS_BATT_OV_FLT:
                SEND_CONST_STRING("Overvoltage", ptr);
            break;
            case BUS_BATT_UV_FLT:
                SEND_CONST_STRING("Undervoltage", ptr);
            break;
            case BUS_CHARGING:
                SEND_CONST_STRING("Charging", ptr);
            break;
            case BUS_OFF:
                SEND_CONST_STRING("Off", ptr);
            break;
            case BUS_READY:
                SEND_CONST_STRING("Ready", ptr);
            break;
            case BUS_TEMP1_FAULT:
                SEND_CONST_STRING("Temperature fault", ptr);
            break;
        }
        SEND_CONST_STRING("\r\n", ptr);
        Term_Color_White(ptr); 
        SEND_CONST_STRING("                                    \r", ptr);
        snprintf(buffer,sizeof(buffer),"Temp 1: %i*C Temp 2: %i*C\r\n", telemetry.temp1, telemetry.temp2);
        send_string(buffer, ptr);
        SEND_CONST_STRING("                                    \r", ptr);
        snprintf(buffer,sizeof(buffer),"Vbus: %u mV Vbatt: %u mV\r\n", ADC_CountsTo_mVolts(ADC_sample_buf[DATA_VBUS]),ADC_CountsTo_mVolts(ADC_sample_buf[DATA_VBATT]));
        send_string(buffer, ptr);
        SEND_CONST_STRING("                                    \r", ptr);
        snprintf(buffer,sizeof(buffer),"Ibus: %u mV\r\n\r\n", ADC_CountsTo_mVolts(ADC_sample_buf[DATA_IBUS]));
        send_string(buffer, ptr);
        xSemaphoreGive(ptr->term_block);
    }
    xSemaphoreTake(ptr->term_block, portMAX_DELAY);
    Term_Enable_Cursor(ptr);
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


