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
#include "clock.h"
#include "hardware.h"
#include "telemetry.h"
#include <project.h>
#include <stdint.h>
#include <stdlib.h>
#include "helper/printf.h"
#include "helper/debug.h"

#include "cyapicallbacks.h"
#include <cytypes.h>

#include "tasks/tsk_analog.h"
#include "tasks/tsk_overlay.h"
#include "tasks/tsk_priority.h"
#include "tasks/tsk_midi.h"
#include "tasks/tsk_cli.h"
#include "tasks/tsk_min.h"
#include "tasks/tsk_fault.h"
#include "tasks/tsk_hwGauge.h"
#include "tasks/tsk_display.h"
#include "min_id.h"
#include "helper/teslaterm.h"
#include "math.h"
#include "alarmevent.h"
#include "version.h"
#include "qcw.h"
#include "system.h"
#include "SidFilter.h"

#include "helper/digipot.h"


#define UNUSED_VARIABLE(N) \
	do {                   \
		(void)(N);         \
	} while (0)
        
	
uint8_t callback_ConfigFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_DefaultFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_TuneFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_TTupdateFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_OfftimeFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_i2tFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_baudrateFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_SPIspeedFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_VisibleFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_MchFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_MchCopyFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_ivoUART(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_ivoLED(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);

void update_ivo();

cli_config configuration;
cli_parameter param;

enum uart_ivo{
    UART_IVO_NONE=0,
    UART_IVO_TX=1,
    UART_IVO_RX=10,
    UART_IVO_RX_TX=11
};


/*****************************************************************************
* Initializes parameters with default values
******************************************************************************/
void init_config(){
    configuration.watchdog = 1;
    configuration.watchdog_timeout = 1000;
    configuration.max_tr_pw = 1000;
    configuration.max_tr_prf = 800;
    configuration.max_qcw_pw = 1000;
    configuration.max_tr_current = 400;
    configuration.min_tr_current = 100;
    configuration.max_qcw_current = 300;
    configuration.temp1_max = 40;
    configuration.temp2_max = 40;
    configuration.ct1_ratio = 600;
    configuration.ct2_ratio = 1000;
    configuration.ct1_burden = 33;
    configuration.ct2_burden = 500;
    configuration.lead_time = 200;
    configuration.start_freq = 630;
    configuration.start_cycles = 3;
    configuration.max_tr_duty = 100;
    configuration.max_qcw_duty = 350;
    configuration.temp1_setpoint = 30;
    configuration.pid_temp_set = 45;
    configuration.pid_temp_mode = 0;
    configuration.pid_temp_p = 0.2;
    configuration.pid_temp_i = 0.2;
    configuration.temp2_setpoint = 30;
    configuration.temp2_mode = 0;
    configuration.ps_scheme = 2;
    configuration.autotune_s = 1;
    configuration.baudrate = 460800;
    configuration.r_top = 500000;
    strncpy(configuration.ud_name,"UD3-Tesla", sizeof(configuration.ud_name));
    configuration.ct2_type = CT2_TYPE_CURRENT;
    configuration.ct2_voltage = 4000;
    configuration.ct2_offset = 0;
    configuration.ct2_current = 0;
    configuration.chargedelay = 1000;
    configuration.ivo_uart = UART_IVO_NONE;
    configuration.enable_display = 0;
    configuration.ext_interrupter = 0;
    configuration.pca9685 = 0;
    configuration.max_fb_errors = 0;
    configuration.ivo_led = 0;
    configuration.uvlo_analog = 0;
    
    configuration.SigGen_minOtOffset = 0;
    
    configuration.min_fb_current = 25;
    
    configuration.compressor_attac = 20;
    configuration.compressor_sustain = 44;
    configuration.compressor_release = 20;
    configuration.compressor_maxDutyOffset = 64;
    
    configuration.ntc_b = 3977;
    configuration.ntc_r25 = 10000;
    
    configuration.idac = 185;
    
    configuration.vdrive = 15.0f;
    
    configuration.hw_rev = SYS_detect_hw_rev();
    configuration.autostart = pdFALSE;
    
    configuration.drive_factor = 1.0f;
    
    interrupter.mod = INTR_MOD_PW;
    
    param.pw = 0;
    param.vol = 0;
    param.burst_on = 0;
    param.burst_off = 500;
    param.tune_start = 400;
    param.tune_end = 1000;
    param.tune_pw = 50;
    param.tune_delay = 50;
    param.offtime = 3;
    
    param.qcw_repeat = 500;
    param.synth = SYNTH_OFF;
    
    param.qcw_holdoff = 0;
    param.qcw_max = 255;
    param.qcw_offset = 0;
    param.qcw_ramp = 200;
    param.qcw_freq = 500;
    param.qcw_vol = 0;
    
    update_ivo();
    
    ramp.changed = pdTRUE;
    qcw_regenerate_ramp();
}

// clang-format off

/*****************************************************************************
* Parameter struct
******************************************************************************/

parameter_entry confparam[] = {
    //       Parameter Type ,Visible,"Text   "         , Value ptr                     ,Min     ,Max    ,Div    ,Callback Function           ,Help text
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"pw"              , param.pw                      , 0      ,10000  ,0      ,callback_PWFunction         ,"Pulsewidth [us]")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"pwd"             , param.pwd                     , 0      ,60000  ,0      ,callback_PWFunction         ,"Pulse Period [us]")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"vol"             , param.vol                     , 0      ,MAX_VOL,0      ,callback_VolFunction        ,"Volume [0-0xffff]")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"bon"             , param.burst_on                , 0      ,1000   ,0      ,callback_BurstFunction      ,"Burst mode ontime [ms] 0=off")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"boff"            , param.burst_off               , 0      ,1000   ,0      ,callback_BurstFunction      ,"Burst mode offtime [ms]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"tune_start"      , param.tune_start              , 5      ,5000   ,10     ,callback_TuneFunction       ,"Start frequency [kHz]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"tune_end"        , param.tune_end                , 5      ,5000   ,10     ,callback_TuneFunction       ,"End frequency [kHz]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"tune_pw"         , param.tune_pw                 , 0      ,800    ,0      ,NULL                        ,"Tune pulsewidth")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"tune_delay"      , param.tune_delay              , 1      ,200    ,0      ,NULL                        ,"Tune delay")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"offtime"         , param.offtime                 , 3      ,250    ,0      ,callback_PWFunction         ,"Offtime for MIDI")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"qcw_ramp"        , param.qcw_ramp                , 1      ,10000  ,100    ,callback_rampFunction       ,"QCW Ramp inc/125us")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"qcw_offset"      , param.qcw_offset              , 0      ,255    ,0      ,callback_rampFunction       ,"QCW Ramp start value")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"qcw_hold"        , param.qcw_holdoff             , 0      ,255    ,0      ,callback_rampFunction       ,"QCW Ramp time to start ramp [125 us]")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"qcw_max"         , param.qcw_max                 , 0      ,255    ,0      ,callback_rampFunction       ,"QCW Ramp end value")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"qcw_freq"        , param.qcw_freq                , 0      ,4000   ,0      ,callback_rampFunction       ,"QCW Ramp modulation frequency")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"qcw_vol"         , param.qcw_vol                 , 0      ,255    ,0      ,callback_rampFunction       ,"QCW Ramp modulation volume")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"qcw_repeat"      , param.qcw_repeat              , 0      ,1000   ,0      ,NULL                        ,"QCW pulse repeat time [ms] <100=single shot")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"synth"           , param.synth                   , 0      ,3      ,0      ,callback_SynthFunction      ,"0=off 1=MIDI 2=SID 3=TR")    
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"sid_hpv_enabled" , SID_filterData.hpvEnabledGlobally, 0      ,1      ,0      ,NULL                     ,"use hpv for playing square sid voices")    
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"sid_noise_volume", SID_filterData.noiseVolume    , 0      ,MAX_VOL,0      ,NULL                        ,"sid noise volume [0-MAX_VOL] = [0-32767]")    
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"sid_ch1_volume"  , SID_filterData.channelVolume[0], 0     ,MAX_VOL,0      ,NULL                        ,"sid channel 1 volume [0-MAX_VOL] = [0-32767]")    
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"sid_ch2_volume"  , SID_filterData.channelVolume[1], 0     ,MAX_VOL,0      ,NULL                        ,"sid channel 2 volume [0-MAX_VOL] = [0-32767]")    
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"sid_ch3_volume"  , SID_filterData.channelVolume[2], 0     ,MAX_VOL,0      ,NULL                        ,"sid channel 3 volume [0-MAX_VOL] = [0-32767]")     
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"watchdog"        , configuration.watchdog        , 0      ,1      ,0      ,callback_ConfigFunction     ,"Watchdog Enable")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"watchdog_timeout", configuration.watchdog_timeout, 1      ,10000  ,0      ,callback_ConfigFunction     ,"Watchdog timeout [ms]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_tr_pw"       , configuration.max_tr_pw       , 0      ,10000  ,0      ,callback_ConfigFunction     ,"Maximum TR PW [uSec]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_tr_prf"      , configuration.max_tr_prf      , 0      ,3000   ,0      ,callback_ConfigFunction     ,"Maximum TR frequency [Hz]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_qcw_pw"      , configuration.max_qcw_pw      , 0      ,5000   ,100    ,callback_ConfigFunction     ,"Maximum QCW PW [ms]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_tr_current"  , configuration.max_tr_current  , 0      ,8000   ,0      ,callback_TTupdateFunction   ,"Maximum TR current [A]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"min_tr_current"  , configuration.min_tr_current  , 0      ,8000   ,0      ,callback_ConfigFunction     ,"Minimum TR current [A]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_qcw_current" , configuration.max_qcw_current , 0      ,8000   ,0      ,callback_TTupdateFunction   ,"Maximum QCW current [A]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"temp1_max"       , configuration.temp1_max       , 0      ,100    ,0      ,callback_TTupdateFunction   ,"Max temperature 1 [*C]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"temp2_max"       , configuration.temp2_max       , 0      ,100    ,0      ,NULL                        ,"Max temperature 2 [*C]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct1_ratio"       , configuration.ct1_ratio       , 1      ,5000   ,0      ,callback_TTupdateFunction   ,"CT1 (feedback) [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct2_ratio"       , configuration.ct2_ratio       , 1      ,5000   ,0      ,callback_ConfigFunction     ,"CT2 (bus) [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct1_burden"      , configuration.ct1_burden      , 1      ,1000   ,10     ,callback_TTupdateFunction   ,"CT1 (feedback) burden [Ohm]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct2_burden"      , configuration.ct2_burden      , 1      ,1000   ,10     ,callback_ConfigFunction     ,"CT2 (bus) burden [Ohm]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct2_type"        , configuration.ct2_type        , 0      ,1      ,0      ,callback_ConfigFunction     ,"CT2 type 0=current 1=voltage")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct2_current"     , configuration.ct2_current     , 0      ,20000  ,10     ,callback_ConfigFunction     ,"CT2 current @ ct_voltage")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct2_voltage"     , configuration.ct2_voltage     , 0      ,5000   ,1000   ,callback_ConfigFunction     ,"CT2 voltage @ ct_current")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct2_offset"      , configuration.ct2_offset      , 0      ,5000   ,1000   ,callback_ConfigFunction     ,"CT2 offset voltage")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"lead_time"       , configuration.lead_time       , 0      ,4000   ,0      ,callback_ConfigFunction     ,"Lead time [nSec]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"start_freq"      , configuration.start_freq      , 0      ,5000   ,10     ,callback_ConfigFunction     ,"Resonant freq [kHz]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"start_cycles"    , configuration.start_cycles    , 0      ,20     ,0      ,callback_ConfigFunction     ,"Start Cyles [N]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_tr_duty"     , configuration.max_tr_duty     , 1      ,500    ,10     ,callback_ConfigFunction     ,"Max TR duty cycle [%]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_qcw_duty"    , configuration.max_qcw_duty    , 1      ,500    ,10     ,callback_ConfigFunction     ,"Max QCW duty cycle [%]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"temp1_setpoint"  , configuration.temp1_setpoint  , 0      ,100    ,0      ,NULL                        ,"Setpoint for fan [*C]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"pid_temp_set"    , configuration.pid_temp_set    , 0      ,100    ,0      ,NULL                        ,"PID temp setpoint")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"pid_temp_mode"   , configuration.pid_temp_mode   , 0      ,4      ,0      ,NULL                        ,"PID temp mode  0=disabled 1=PID3_Temp1 2=PID4_Temp1 3=PID3_Temp2 4=PID4_Temp2")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"pid_temp_p"      , configuration.pid_temp_p      , 0      ,200    ,0      ,callback_temp_pid           ,"Temperature PI")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"pid_temp_i"      , configuration.pid_temp_i      , 0      ,200    ,0      ,callback_temp_pid           ,"Temperature PI")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"temp2_setpoint"  , configuration.temp2_setpoint  , 0      ,100    ,0      ,NULL                        ,"Setpoint for TH2 [*C] 0=disabled")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"temp2_mode"      , configuration.temp2_mode      , 0      ,4      ,0      ,NULL                        ,"TH2 setpoint mode  0=disabled 1=FAN 3=Relay3 4=Relay4")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ps_scheme"       , configuration.ps_scheme       , 0      ,5      ,0      ,callback_ConfigFunction     ,"Power supply scheme")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"charge_delay"    , configuration.chargedelay     , 1      ,60000  ,0      ,callback_ConfigFunction     ,"Delay for the charge relay [ms]")  
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"autotune_s"      , configuration.autotune_s      , 1      ,32     ,0      ,NULL                        ,"Number of samples for Autotune")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ud_name"         , configuration.ud_name         , 0      ,0      ,0      ,NULL                        ,"Name of the Coil [15 chars]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"baudrate"        , configuration.baudrate        , 1200   ,4000000,0      ,callback_baudrateFunction   ,"Serial baudrate")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ivo_uart"        , configuration.ivo_uart        , 0      ,11     ,0      ,callback_ivoUART            ,"[RX][TX] 0=not inverted 1=inverted")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"r_bus"           , configuration.r_top           , 100    ,1000000,1000   ,callback_TTupdateFunction   ,"Series resistor of voltage input [kOhm]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ena_display"     , configuration.enable_display  , 0      ,6      ,0      ,NULL                        ,"Enables the WS2812 display")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ena_ext_int"     , configuration.ext_interrupter , 0      ,2      ,0      ,callback_ext_interrupter    ,"Enable external interrupter 2=inverted")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"qcw_coil"        , configuration.is_qcw          , 0      ,1      ,0      ,NULL                        ,"Is QCW 1=true 0=false")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"pca9685"         , configuration.pca9685         , 0      ,1      ,0      ,NULL                        ,"0=off 1=on")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_fb_errors"   , configuration.max_fb_errors   , 0      ,60000  ,0      ,NULL                        ,"0=off, number of feedback errors per second to sysfault")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ntc_b"           , configuration.ntc_b           , 0      ,10000  ,0      ,callback_ntc                ,"NTC beta [k]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ntc_r25"         , configuration.ntc_r25         , 0      ,33000  ,1000   ,callback_ntc                ,"NTC R25 [kOhm]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ntc_idac"        , configuration.idac            , 0      ,2000   ,0      ,callback_ntc                ,"iDAC measured [uA]")
    ADD_PARAM(PARAM_CONFIG  ,pdFALSE,"d_factor"        , configuration.drive_factor    , 0      ,10     ,0      ,callback_ConfigFunction     ,"Factor for drive voltage measurement")
    ADD_PARAM(PARAM_CONFIG  ,pdFALSE,"hwGauge_cfg"     , hwGauges.rawData              , 0      ,0      ,0      ,callback_hwGauge            ,"gauge configs, configure with the \"hwGauge\" command")
    ADD_PARAM(PARAM_CONFIG  ,pdFALSE,"display_cfg"     , DISP_zones.rawData            , 0      ,0      ,0      ,callback_display            ,"display/led configs, configure with the \"display\" command")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"vdrive"          , configuration.vdrive          , 10     ,24     ,0      ,callback_ConfigFunction     ,"Change Vdrive voltage (digipot)")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ivo_led"         , configuration.ivo_led         , 0      ,1      ,0      ,callback_ivoLED             ,"LED invert option")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"uvlo_analog"     , configuration.uvlo_analog     , 0      ,32000  ,1000   ,NULL                        ,"UVLO from ADC 0=GPIO UVLO")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"hw_rev"          , configuration.hw_rev          , 0      ,2      ,0      ,callback_ConfigFunction     ,"Hardware revision 0=3.0-3.1a 1=3.1b 2=3.1c")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"autostart"       , configuration.autostart       , 0      ,1      ,0      ,NULL                        ,"Autostart")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"min_fb_current"  , configuration.min_fb_current  , 0      ,255    ,0      ,callback_ConfigFunction     ,"Current at which to switch to feedback")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"minDutyOffset"   , configuration.SigGen_minOtOffset  , 0      ,100    ,0      ,callback_siggen         ,"Minimum pulsewidth in percent of maximum pulsewidth")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"comp_attac"      , configuration.compressor_attac  , 0      ,255    ,0      ,NULL                      ,"Compressor Attac Setting")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"comp_sustain"    , configuration.compressor_sustain  , 0      ,255    ,0      ,NULL                    ,"Compressor Sustain Setting")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"comp_release"    , configuration.compressor_release  , 0      ,255    ,0      ,NULL                    ,"Compressor Release Setting")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"comp_dutyOffset" , configuration.compressor_maxDutyOffset  , 0      ,255    ,0      ,NULL              ,"Maximum Dutycycle offset before hard limit")
};

   
void eeprom_load(TERMINAL_HANDLE * handle){
    EEPROM_read_conf(confparam, PARAM_SIZE(confparam) ,0,handle);
    if(param.offtime<3) param.offtime=3;
    update_ivo();
    update_visibilty();
    uart_baudrate(configuration.baudrate);
    ramp.changed = pdTRUE;
    qcw_regenerate_ramp();
    init_telemetry();
    callback_temp_pid(confparam,0,handle);
    callback_ext_interrupter(confparam,0,handle);
    callback_ConfigFunction(confparam,0,handle);
}



void update_visibilty(void){

    switch(configuration.ct2_type){
        case CT2_TYPE_CURRENT:
            set_visibility(confparam,CONF_SIZE, "ct2_current",pdFALSE);
            set_visibility(confparam,CONF_SIZE, "ct2_voltage",pdFALSE);
            set_visibility(confparam,CONF_SIZE, "ct2_offset",pdFALSE);
            set_visibility(confparam,CONF_SIZE, "ct2_ratio",pdTRUE);
            set_visibility(confparam,CONF_SIZE, "ct2_burden",pdTRUE);
        break;
        case CT2_TYPE_VOLTAGE:
            set_visibility(confparam,CONF_SIZE, "ct2_ratio",pdFALSE);
            set_visibility(confparam,CONF_SIZE, "ct2_burden",pdFALSE);
            set_visibility(confparam,CONF_SIZE, "ct2_current",pdTRUE);
            set_visibility(confparam,CONF_SIZE, "ct2_voltage",pdTRUE);
            set_visibility(confparam,CONF_SIZE, "ct2_offset",pdTRUE);
        break;
    }
    
    set_visibility(confparam, CONF_SIZE, "vdrive", configuration.hw_rev > 0 ? pdTRUE : pdFALSE);   

}

// clang-format on

void init_tt_if_enabled(TERMINAL_HANDLE* handle) {
    if (portM->term_mode!=PORT_TERM_VT100) {
        uint8_t include_chart;
        if (portM->term_mode==PORT_TERM_TT) {
            include_chart = pdTRUE;
        } else {
            include_chart = pdFALSE;
        }
        init_tt(include_chart,handle);
    }
}

/*****************************************************************************
* Callback for invert option UART
******************************************************************************/

void update_ivo(){
    switch(configuration.ivo_uart){
    case UART_IVO_NONE:
        clear_bit(IVO_Control, IVO_UART_TX_BIT);
        clear_bit(IVO_Control, IVO_UART_RX_BIT);
        break;
    case UART_IVO_TX:
        set_bit(IVO_Control, IVO_UART_TX_BIT);
        clear_bit(IVO_Control, IVO_UART_RX_BIT);
        break;
    case UART_IVO_RX:
        set_bit(IVO_Control, IVO_UART_RX_BIT);
        clear_bit(IVO_Control, IVO_UART_TX_BIT);
        break;
    case UART_IVO_RX_TX:
        set_bit(IVO_Control, IVO_UART_RX_BIT);
        set_bit(IVO_Control, IVO_UART_TX_BIT);
        break;
    }  
    if(configuration.ivo_led){
        set_bit(IVO_Control, IVO_LED_BIT);
    }else{
        clear_bit(IVO_Control, IVO_LED_BIT);
    }
    
}

uint8_t callback_ivoUART(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    if(configuration.ivo_uart == UART_IVO_NONE || configuration.ivo_uart == UART_IVO_TX || configuration.ivo_uart == UART_IVO_RX || configuration.ivo_uart == UART_IVO_RX_TX){
        update_ivo();   
        return 1;
    }else{
        ttprintf("Only the folowing combinations are allowed\r\n");
        ttprintf("0  = no inversion\r\n");
        ttprintf("1  = rx inverted, tx not inverted\r\n");
        ttprintf("10 = rx not inverted, tx inverted\r\n");
        ttprintf("11 = rx and tx inverted\r\n");
        return 0;
    }
}

uint8_t callback_ivoLED(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    update_ivo();
    return 1;
}


/*****************************************************************************
* Callback if the offtime parameter is changed
******************************************************************************/
uint8_t callback_VisibleFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    update_visibilty();
    return 1;
}


/*****************************************************************************
* Callback if the maximum current is changed
******************************************************************************/
uint8_t callback_TTupdateFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle) {
    
    uint32_t max_current_cmp = ((4080ul * configuration.ct1_ratio) / configuration.ct1_burden)/100; //Amperes
    uint32_t max_current_meas = ((5000ul * configuration.ct1_ratio) / configuration.ct1_burden)/100; //Amperes
    
    if(configuration.max_tr_current>max_current_cmp || configuration.max_qcw_current>max_current_cmp){
        ttprintf("Warning: Max CT1 current with the current setup is %uA for OCD and %uA for peak measurement\r\n",max_current_cmp,max_current_meas);
        if(configuration.max_tr_current>max_current_cmp) configuration.max_tr_current = max_current_cmp;
        if(configuration.max_qcw_current>max_current_cmp) configuration.max_qcw_current = max_current_cmp;
    }
    uint8 sfflag = system_fault_Read();
    
    sysflt_set(pdTRUE); //halt tesla coil operation during updates!
    
    configure_ZCD_to_PWM();
    
    system_fault_Control = sfflag;
    
    init_tt_if_enabled(handle);
	return 1;
}

/*****************************************************************************
* Callback if a autotune parameter is changed
******************************************************************************/
uint8_t callback_TuneFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle) {
    if(param.tune_start >= param.tune_end){
        ttprintf("ERROR: tune_start > tune_end\r\n");
		return 1;
	} else
		return 1;
}



/*****************************************************************************
* Callback if the baudrate is changed
******************************************************************************/
void uart_baudrate(uint32_t baudrate){
    float divider = (float)(BCLK__BUS_CLK__HZ/8)/(float)baudrate;
    uint16_t divider_selected=1;
    
    uint32_t down_rate = (BCLK__BUS_CLK__HZ/8)/floor(divider);
    uint32_t up_rate = (BCLK__BUS_CLK__HZ/8)/ceil(divider);
   
    float down_rate_error = (down_rate/(float)baudrate)-1;
    float up_rate_error = (up_rate/(float)baudrate)-1;
    
    UART_Stop();
    if(fabs(down_rate_error) < fabs(up_rate_error)){
        //selected round down divider
        divider_selected = floor(divider);
    }else{
        //selected round up divider
        divider_selected = ceil(divider);
    }

    UART_CLK_SetDividerValue(divider_selected);
    
    tt.n.rx_datarate.max = baudrate / 8;
    tt.n.tx_datarate.max = baudrate / 8;
    
    UART_Start();  
    
}

uint8_t callback_baudrateFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    uart_baudrate(configuration.baudrate);
 
    return 1;
}


/*****************************************************************************
* Callback if a configuration relevant parameter is changed
******************************************************************************/
uint8_t callback_ConfigFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    uint8 sfflag = system_fault_Read();
    sysflt_set(pdTRUE); //halt tesla coil operation during updates!
    
    if(configuration.hw_rev > 0){
        dcdc_ena_Write(0); //disable DCDC
        digipot_set_voltage(configuration.vdrive);
    }
    
    WD_enable(configuration.watchdog);
    configure_interrupter();
	initialize_charging();
	configure_ZCD_to_PWM();
    update_visibilty();
    reconfig_charge_timer();
    
    recalc_telemetry_limits();

    init_tt_if_enabled(handle);
    
    if(configuration.hw_rev > 0){
        dcdc_ena_Write(1); //enable DCDC
    }
    tsk_analog_recalc_drive_top(configuration.drive_factor);
    
    if(configuration.is_qcw){
        qcw_regenerate_ramp();   
    }
    
	system_fault_Control = sfflag;
    return 1;
}

/*****************************************************************************
* Default function if a parameter is changes (not used)
******************************************************************************/
uint8_t callback_DefaultFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    
    return 1;
}


void con_info(TERMINAL_HANDLE * handle){
    #define COL_A 9
    #define COL_B 15
    ttprintf("\r\nConnected clients:\r\n");
    TERM_sendVT100Code(handle, _VT100_CURSOR_SET_COLUMN, COL_A);
    ttprintf("Num");
    TERM_sendVT100Code(handle, _VT100_CURSOR_SET_COLUMN, COL_B);
    ttprintf("| Remote IP\r\n");
    
    for(uint8_t i=0;i<NUM_MIN_CON;i++){
        if(socket_info[i].socket==SOCKET_CONNECTED){
            TERM_sendVT100Code(handle, _VT100_CURSOR_SET_COLUMN, COL_A);
            ttprintf("\033[36m%d", i);
            TERM_sendVT100Code(handle, _VT100_CURSOR_SET_COLUMN, COL_B);
            ttprintf("\033[32m%s\r\n", socket_info[i].info);
        }
    }
    TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);
}

void con_numcon(TERMINAL_HANDLE * handle){
    uint8_t cnt=0;
    for(uint8_t i=0;i<NUM_MIN_CON;i++){
        if(socket_info[i].socket==SOCKET_CONNECTED){
            cnt++;
        }
    }
    ttprintf("CLI-Sessions: %u/%u\r\n",cnt ,NUM_MIN_CON);
}

/*****************************************************************************
* Displays the statistics of the min protocol
******************************************************************************/
uint8_t con_minstat(TERMINAL_HANDLE * handle){
    TERM_sendVT100Code(handle, _VT100_CLS,0);
    TERM_sendVT100Code(handle, _VT100_CURSOR_DISABLE,0);
  
    while(true){

        uint8_t c = getch(handle, 200 /portTICK_RATE_MS);
        if(c == CTRL_C || c == 'q'){  //0x03 = CTRL+C
            break;   
        }else if ( c == 'r'){
            min_ctx.transport_fifo.dropped_frames = 0;
            min_ctx.transport_fifo.spurious_acks = 0;
            min_ctx.transport_fifo.resets_received = 0;
            min_ctx.transport_fifo.sequence_mismatch_drop = 0;
            min_ctx.transport_fifo.n_frames_max = 0;
            min_ctx.transport_fifo.crc_fails = 0;
            min_time.resync = 0;
        }
        
        TERM_sendVT100Code(handle, _VT100_CURSOR_POS1,0);
        ttprintf("MIN monitor    (press [CTRL+C] for quit or press [r] for reset stats)\r\n");
        ttprintf("%sDropped frames        : %lu\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0), min_ctx.transport_fifo.dropped_frames);
        ttprintf("%sSpurious acks         : %lu\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),min_ctx.transport_fifo.spurious_acks);
        ttprintf("%sResets received       : %lu\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),min_ctx.transport_fifo.resets_received);
        ttprintf("%sSequence mismatch drop: %lu\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),min_ctx.transport_fifo.sequence_mismatch_drop);
        ttprintf("%sMax frames in buffer  : %u\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),min_ctx.transport_fifo.n_frames_max);
        ttprintf("%sCRC errors            : %u\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),min_ctx.transport_fifo.crc_fails);
        ttprintf("%sRemote Time           : %u\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),min_time.remote);
        ttprintf("%sLocal Time            : %u\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),l_time);
        ttprintf("%sDiff Time             : %i\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),min_time.diff);
        ttprintf("%sResync count          : %u\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),min_time.resync);
    }
    TERM_sendVT100Code(handle, _VT100_CURSOR_ENABLE,0);
    return 1; 
}

/*****************************************************************************
* Prints the ethernet connections
******************************************************************************/
uint8_t CMD_con(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args) {
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf("con [info|numcon|min]\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    }

    if(strcmp(args[0], "info") == 0){
        con_info(handle);
        return TERM_CMD_EXIT_SUCCESS;
    }

    if(strcmp(args[0], "numcon") == 0){
        con_numcon(handle);
        return TERM_CMD_EXIT_SUCCESS;
    }

    if(strcmp(args[0], "min") == 0){
        con_minstat(handle);
        return TERM_CMD_EXIT_SUCCESS;
    }
    return TERM_CMD_EXIT_SUCCESS;
}

/*****************************************************************************
* Calibrate Vdriver
******************************************************************************/
uint8_t CMD_calib(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args) {
    
    #define NUM_CALIB_SAMPLES 16    
    
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf("calib [measured voltage]\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    }

    float voltage = atof(args[0]);
    tsk_analog_recalc_drive_top(1.0f);
    
    ttprintf("Collecting samples for %f drive Voltage ...\r\n", voltage);
    Term_check_break(handle,100);
    
    
    uint32_t i=NUM_CALIB_SAMPLES;
    
    uint32_t accu = 0;
    
    while(i){
        uint32_t sample = read_driver_mv();
        accu += sample;
        ttprintf("Sample %u: %u mv\r\n", i, sample); 
        
        if(Term_check_break(handle,100) == pdFALSE){
            ttprintf("Canceled by user\r\n");
            goto abort;
        }
        i--;
    }
    
    accu = accu / NUM_CALIB_SAMPLES;
    
    configuration.drive_factor =  voltage * 1000.0f / (float)accu;
    
    ttprintf ("New factor: %f\r\n", configuration.drive_factor);
    
    abort:
    
    tsk_analog_recalc_drive_top(configuration.drive_factor);
    
    return TERM_CMD_EXIT_SUCCESS;
}

/*****************************************************************************
* Sends the features to teslaterm
******************************************************************************/
uint8_t CMD_features(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args) {
	for (uint8_t i = 0; i < sizeof(version)/sizeof(char*); i++) {
       send_features(version[i],handle); 
    }
    return TERM_CMD_EXIT_SUCCESS; 
}

/*****************************************************************************
* Sends the configuration to teslaterm
******************************************************************************/
uint8_t CMD_config_get(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    char buffer[80];
	for (uint8_t current_parameter = 0; current_parameter < sizeof(confparam) / sizeof(parameter_entry); current_parameter++) {
        if(confparam[current_parameter].parameter_type == PARAM_CONFIG  && confparam[current_parameter].visible){
            print_param_buffer(buffer, confparam, current_parameter);
            send_config(buffer,confparam[current_parameter].help, handle);
        }
    }
    send_config("NULL","NULL", handle);
    return TERM_CMD_EXIT_SUCCESS; 
}

/*****************************************************************************
* Kicks the controller into the bootloader
******************************************************************************/
uint8_t CMD_bootloader(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
#if USE_BOOTLOADER
    Bootloadable_Load();
#endif
	return TERM_CMD_EXIT_SUCCESS;
}


/*****************************************************************************
* sets the kill bit, stops the interrupter and switches the bus off
******************************************************************************/
uint8_t CMD_udkill(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args) {
    
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf("Usage: kill [set|reset|get]\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    }
    
    if(strcmp(args[0], "set") == 0){
        interrupter_kill();
    	queue_midi_message((uint8_t*)kill_msg);
    	bus_command = BUS_COMMAND_OFF;
        
        QCW_delete_timer();
        
    	interrupter1_control_Control = 0;
    	QCW_enable_Control = 0;
    	TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_GREEN);
    	ttprintf("Killbit set\r\n");
        alarm_push(ALM_PRIO_CRITICAL, "FAULT: Killbit set", ALM_NO_VALUE);
    	TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);
        return TERM_CMD_EXIT_SUCCESS;
    }else if(strcmp(args[0], "reset") == 0){
        interrupter_unkill();
        reset_fault();
        sysflt_clr(pdTRUE); 
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_GREEN);
    	ttprintf("Killbit reset\r\n");
        alarm_push(ALM_PRIO_INFO, "INFO: Killbit reset", ALM_NO_VALUE);
    	TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);
        return TERM_CMD_EXIT_SUCCESS;
    }else if(strcmp(args[0], "get") == 0){
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_RED);
        ttprintf("Killbit: %u\r\n",sysfault.interlock);
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);;
        return TERM_CMD_EXIT_SUCCESS;
    }
    return TERM_CMD_EXIT_SUCCESS;
}


/*****************************************************************************
* Get a value from a parameter or print all parameters
******************************************************************************/
uint8_t CMD_get(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args) {
    
    if(argCount==0){
        print_param_help(confparam, PARAM_SIZE(confparam), handle);
        return TERM_CMD_EXIT_SUCCESS;
    }
    
    if(strcmp(args[0], "-?") == 0){
        ttprintf("Usage: get [parameter]\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    }
    
	for (uint8_t current_parameter = 0; current_parameter < sizeof(confparam) / sizeof(parameter_entry); current_parameter++) {
		if (strcmp(args[0], confparam[current_parameter].name) == 0) {
			//Parameter found:
			print_param(confparam,current_parameter,handle);
			return TERM_CMD_EXIT_SUCCESS;
		}
	}
	TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_RED);
	ttprintf("E: unknown param\r\n");
	TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);
	return 0;
}

/*****************************************************************************
* Set a new value to a parameter
******************************************************************************/
uint8_t CMD_set(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args) {

	if(argCount<2){
        ttprintf("Usage: set [parameter] [value]\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    }
  
	for (uint8_t current_parameter = 0; current_parameter < sizeof(confparam) / sizeof(parameter_entry); current_parameter++) {
		if (strcmp(args[0], confparam[current_parameter].name) == 0) {
			//parameter name found:

			if (updateDefaultFunction(confparam, args[1],current_parameter, handle)){
                if(confparam[current_parameter].callback_function){
                    if (confparam[current_parameter].callback_function(confparam, current_parameter, handle)){
                        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_GREEN);
                        ttprintf("OK\r\n");
                        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);
                        return TERM_CMD_EXIT_SUCCESS;
                    }else{
                        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_RED);
                        ttprintf("ERROR: Callback\r\n");
                        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);
                        return TERM_CMD_EXIT_SUCCESS;
                    }
                }else{
                    TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_GREEN);
                    ttprintf("OK\r\n");
                    TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);
                    return TERM_CMD_EXIT_SUCCESS;
                }
			} else {
				TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_RED);
				ttprintf("NOK\r\n");
				TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);
				return TERM_CMD_EXIT_SUCCESS;
			}
		}
	}
	TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_RED);
	ttprintf("E: unknown param\r\n");
	TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);
	return TERM_CMD_EXIT_SUCCESS;
}


/*****************************************************************************
* Saves confparams to eeprom
******************************************************************************/
uint8_t CMD_eeprom(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args) {
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf("Usage: eeprom [load|save]\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    }
    EEPROM_1_UpdateTemperature();
	uint8 sfflag = system_fault_Read();
	sysflt_set(pdTRUE); //halt tesla coil operation during updates!
    
	if(strcmp(args[0], "save") == 0){
        EEPROM_check_hash(confparam,PARAM_SIZE(confparam),handle);
	    EEPROM_write_conf(confparam, PARAM_SIZE(confparam),0, handle);
        
	}else if(strcmp(args[0], "load") == 0){
		EEPROM_read_conf(confparam, PARAM_SIZE(confparam) ,0,handle);
        
        configure_interrupter();
	    initialize_charging();
	    configure_ZCD_to_PWM();
	}
    
    system_fault_Control = sfflag;
	return TERM_CMD_EXIT_SUCCESS;
}

/*****************************************************************************
* Switches the bus on/off
******************************************************************************/
uint8_t CMD_bus(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf("Usage: bus [on|off]\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    }

	if(strcmp(args[0], "on") == 0){
		bus_command = BUS_COMMAND_ON;
		ttprintf("BUS ON\r\n");
        return TERM_CMD_EXIT_SUCCESS;
	}
	if(strcmp(args[0], "off") == 0){
		bus_command = BUS_COMMAND_OFF;
		ttprintf("BUS OFF\r\n");
        return TERM_CMD_EXIT_SUCCESS;
	}
    return TERM_CMD_EXIT_SUCCESS;
}

/*****************************************************************************
* Loads the default parametes out of flash
******************************************************************************/
uint8_t CMD_load_defaults(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    ttprintf("Default parameters loaded\r\n");
    init_config();
    return TERM_CMD_EXIT_SUCCESS;
}

/*****************************************************************************
* Reset of the controller
******************************************************************************/
uint8_t CMD_reset(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    CySoftwareReset();
    return TERM_CMD_EXIT_SUCCESS;
}

/*****************************************************************************
* Switches the user relay 3 or 4
******************************************************************************/
uint8_t CMD_relay(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    if(argCount<2 || strcmp(args[0], "-?") == 0){
        ttprintf("Usage: relay 3/4 [1|0]\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    }
    
    uint8_t r_number = atoi(args[0]);
    uint8_t value = atoi(args[1]);
    if(value > 1){
        ttprintf("Usage: relay 3/4 [1|0]\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    }
    
    if(r_number == 3){
        temp_pwm_WriteCompare1(value ? 255 : 0);
        return TERM_CMD_EXIT_SUCCESS;
    }
    if(r_number == 4){
        temp_pwm_WriteCompare2(value ? 255 : 0);
        return TERM_CMD_EXIT_SUCCESS;
    }
    return TERM_CMD_EXIT_SUCCESS;
}

/*****************************************************************************
* Switches the user relay 3 or 4
******************************************************************************/
uint8_t CMD_pwm(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    if(argCount<2 || strcmp(args[0], "-?") == 0){
        ttprintf("Usage: pwm 3/4 [0-255]\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    }
    
    uint8_t pwm_number = atoi(args[0]);
    uint8_t value = atoi(args[1]);
    if(value > 255){
        value = 255;
    }
    
    if(pwm_number == 3){
        temp_pwm_WriteCompare1(value);
        return TERM_CMD_EXIT_SUCCESS;
    }
    if(pwm_number == 4){
        temp_pwm_WriteCompare2(value);
        return TERM_CMD_EXIT_SUCCESS;
    }
    return TERM_CMD_EXIT_SUCCESS;
}


/*****************************************************************************
* Read hardware revision bits
******************************************************************************/
uint8_t CMD_hwrev(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    ttprintf("Hardware revision is set to: %u = %s\r\n", configuration.hw_rev, SYS_get_rev_string(configuration.hw_rev));
    
    uint8_t rev = SYS_detect_hw_rev();
    
    ttprintf("Hardware revision bits: %u = %s\r\n", rev, SYS_get_rev_string(rev));
    

    return TERM_CMD_EXIT_SUCCESS;
}

/*****************************************************************************
* Signal debugging
******************************************************************************/

void send_signal_state_new(uint8_t signal, uint8_t inverted, TERMINAL_HANDLE * handle){
    if(inverted) signal = !signal; 
    if(signal){
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_RED);
        ttprintf("true \r\n");
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE); 
    }else{
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_GREEN);
        ttprintf("false\r\n");
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);
    }
}
void send_signal_state_wo_new(uint8_t signal, uint8_t inverted, TERMINAL_HANDLE * handle){
    if(inverted) signal = !signal; 
    if(signal){
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_RED);
        ttprintf("true ");
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);
    }else{
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_GREEN);
        ttprintf("false");
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);
    }
}

uint8_t CMD_signals(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    TERM_sendVT100Code(handle, _VT100_CLS, 0);
    TERM_sendVT100Code(handle, _VT100_CURSOR_DISABLE, 0);
    do{
        TERM_sendVT100Code(handle, _VT100_CURSOR_POS1, 0);
        ttprintf("Signal state [CTRL+C] for quit:\r\n");
        ttprintf("**************************\r\n");
        ttprintf("UVLO pin: ");
        send_signal_state_wo_new(UVLO_Read(), pdTRUE, handle);
        ttprintf(" Clock failure: ");
        #ifndef SIMULATOR
        send_signal_state_new(!(CY_GET_XTND_REG8((void CYFAR *)CYREG_FASTCLK_XMHZ_CSR) & 0x80u),pdTRUE,handle);
        #else
        send_signal_state_new(1,pdTRUE,handle);
        #endif
        ttprintf("Sysfault driver undervoltage: ");
        send_signal_state_new(sysfault.uvlo,pdFALSE,handle);
        ttprintf("Sysfault Temp 1: ");
        send_signal_state_wo_new(sysfault.temp1,pdFALSE,handle);
        ttprintf(" Temp 2: ");
        send_signal_state_new(sysfault.temp2,pdFALSE,handle);
        ttprintf("Sysfault fuse: ");
        send_signal_state_wo_new(sysfault.fuse,pdFALSE,handle);
        ttprintf(" charging: ");
        send_signal_state_new(sysfault.charge,pdFALSE,handle);
        ttprintf("Sysfault watchdog: ");
        send_signal_state_wo_new(sysfault.watchdog,pdFALSE,handle);
        ttprintf(" updating: ");
        send_signal_state_new(sysfault.update,pdFALSE,handle);
        ttprintf("Sysfault bus uvlo: ");
        send_signal_state_wo_new(sysfault.bus_uv,pdFALSE,handle);
        ttprintf(" feedback: ");
        send_signal_state_new(sysfault.feedback,pdFALSE,handle);
        ttprintf("Sysfault interlock: ");
        send_signal_state_wo_new(sysfault.interlock,pdFALSE,handle);
        ttprintf(" link: ");
        send_signal_state_wo_new(sysfault.link_state,pdFALSE,handle);
        ttprintf(" combined: ");
        send_signal_state_new(system_fault_Read(),pdTRUE,handle);
        ttprintf("Feedback errors: %4u\r\n", feedback_error_cnt);
        ttprintf("Relay 1: ");
        send_signal_state_wo_new(relay_read_bus(),pdFALSE,handle);
        ttprintf(" Relay 2: ");
        send_signal_state_wo_new(relay_read_charge_end(),pdFALSE,handle);
        ttprintf(" Relay 3: ");
        send_signal_state_wo_new(Relay3_Read(),pdFALSE,handle);
        ttprintf(" Relay 4: ");
        send_signal_state_new(Relay4_Read(),pdFALSE,handle);
        ttprintf("Fan: ");
        send_signal_state_wo_new(Fan_Read(),pdFALSE,handle);
        ttprintf(" Bus status: ");
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_CYAN);
        switch(tt.n.bus_status.value){
            case BUS_BATT_OV_FLT:
                ttprintf("Overvoltage      ");
            break;
            case BUS_BATT_UV_FLT:
                ttprintf("Undervoltage     ");
            break;
            case BUS_CHARGING:
                ttprintf("Charging         ");
            break;
            case BUS_OFF:
                ttprintf("Off              ");
            break;
            case BUS_READY:
                ttprintf("Ready            ");
            break;
            case BUS_TEMP1_FAULT:
                ttprintf("Temperature fault");
            break;
        }
        ttprintf("\r\n");
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);
        ttprintf("                                    \r");
        ttprintf("Temp 1: %i.%i*C Temp 2: %i.%i*C\r\n", tt.n.temp1.value/10, tt.n.temp1.value%10, tt.n.temp2.value/10, tt.n.temp2.value%10);
        ttprintf("                                    \r");
        ttprintf("Vbus: %u mV Vbatt: %u mV\r\n", ADC_CountsTo_mVolts(ADC_active_sample_buf[0].v_bus),ADC_CountsTo_mVolts(ADC_active_sample_buf[0].v_batt));
        ttprintf("                                    \r");
        ttprintf("Ibus: %u mV Vdriver: %u mV\r\n\r\n", ADC_CountsTo_mVolts(ADC_active_sample_buf[0].i_bus),tt.n.driver_v.value);

    }while(Term_check_break(handle,250));
    
    TERM_sendVT100Code(handle, _VT100_RESET_ATTRIB, 0);

    return TERM_CMD_EXIT_SUCCESS;
}

