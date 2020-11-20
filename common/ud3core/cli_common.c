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
#include "tasks/tsk_uart.h"
#include "tasks/tsk_midi.h"
#include "tasks/tsk_cli.h"
#include "tasks/tsk_min.h"
#include "tasks/tsk_fault.h"
#include "min_id.h"
#include "helper/teslaterm.h"
#include "math.h"
#include "alarmevent.h"
#include "version.h"
#include "qcw.h"
#include "system.h"


#define UNUSED_VARIABLE(N) \
	do {                   \
		(void)(N);         \
	} while (0)
        
	
uint8_t callback_ConfigFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_DefaultFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_TuneFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_TTupdateFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_TRFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_TRPFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_OfftimeFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_BurstFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_i2tFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_baudrateFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_SPIspeedFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_VisibleFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_MchFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_MchCopyFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
uint8_t callback_ivoUART(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);

void update_ivo_uart();

uint8_t burst_state = 0;


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
    configuration.max_qcw_pw = 1000;
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
    configuration.baudrate = 460800;
    configuration.spi_speed = 16;
    configuration.r_top = 500000;
    strncpy(configuration.ud_name,"UD3-Tesla", sizeof(configuration.ud_name));
    strncpy(configuration.synth_filter,"f<0f>20000", sizeof(configuration.ud_name));  //No filter
    configuration.minprot = pdFALSE;
    configuration.max_const_i = 0;
    configuration.max_fault_i = 250;
    configuration.ct2_type = CT2_TYPE_CURRENT;
    configuration.ct2_voltage = 4000;
    configuration.ct2_offset = 0;
    configuration.ct2_current = 0;
    configuration.chargedelay = 1000;
    configuration.ivo_uart = 0;  //Nothing inverted
    configuration.line_code = 0; //UART
    configuration.enable_display = 0;
    configuration.pid_curr_p = 50;
    configuration.pid_curr_i = 5;
    configuration.max_dc_curr = 0;
    configuration.ext_interrupter = 0;
    configuration.noise_w = SID_NOISE_WEIGHT;
    
    param.pw = 0;
    param.pwp = 0;
    param.pwd = 50000;
    param.burst_on = 0;
    param.burst_off = 500;
    param.tune_start = 400;
    param.tune_end = 1000;
    param.tune_pw = 50;
    param.tune_delay = 50;
    param.offtime = 3;
    
    param.qcw_repeat = 500;
    param.transpose = 0;
    param.mch = 0;
    param.synth = SYNTH_MIDI;
    
    param.qcw_holdoff = 0;
    param.qcw_max = 255;
    param.qcw_offset = 0;
    param.qcw_ramp = 200;
    
    i2t_set_limit(configuration.max_const_i,configuration.max_fault_i,10000);
    update_ivo_uart();
    
    ramp.changed = pdTRUE;
    qcw_regenerate_ramp();
}

// clang-format off

/*****************************************************************************
* Parameter struct
******************************************************************************/

parameter_entry confparam[] = {
    //       Parameter Type ,Visible,"Text   "         , Value ptr                     ,Min     ,Max    ,Div    ,Callback Function           ,Help text
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"pw"              , param.pw                      , 0      ,800    ,0      ,callback_TRFunction         ,"Pulsewidth [us]")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"pwp"             , param.pwp                     , 0      ,8000   ,10     ,callback_TRPFunction        ,"Pulsewidth [%]")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"pwd"             , param.pwd                     , 0      ,60000  ,0      ,callback_TRFunction         ,"Pulsewidthdelay")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"bon"             , param.burst_on                , 0      ,1000   ,0      ,callback_BurstFunction      ,"Burst mode ontime [ms] 0=off")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"boff"            , param.burst_off               , 0      ,1000   ,0      ,callback_BurstFunction      ,"Burst mode offtime [ms]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"tune_start"      , param.tune_start              , 5      ,5000   ,10     ,callback_TuneFunction       ,"Start frequency [kHz]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"tune_end"        , param.tune_end                , 5      ,5000   ,10     ,callback_TuneFunction       ,"End frequency [kHz]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"tune_pw"         , param.tune_pw                 , 0      ,800    ,0      ,NULL                        ,"Tune pulsewidth")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"tune_delay"      , param.tune_delay              , 1      ,200    ,0      ,NULL                        ,"Tune delay")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"offtime"         , param.offtime                 , 3      ,250    ,0      ,NULL                        ,"Offtime for MIDI")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"qcw_ramp"        , param.qcw_ramp                , 1      ,10000  ,100    ,callback_rampFunction       ,"QCW Ramp inc/125us")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"qcw_offset"      , param.qcw_offset              , 0      ,255    ,0      ,callback_rampFunction       ,"QCW Ramp start value")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"qcw_hold"        , param.qcw_holdoff             , 0      ,255    ,0      ,callback_rampFunction       ,"QCW Ramp time to start ramp [125 us]")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"qcw_max"         , param.qcw_max                 , 0      ,255    ,0      ,callback_rampFunction       ,"QCW Ramp end value")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"qcw_repeat"      , param.qcw_repeat              , 0      ,1000   ,0      ,NULL                        ,"QCW pulse repeat time [ms] <100=single shot")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"transpose"       , param.transpose               , -48    ,48     ,0      ,NULL                        ,"Transpose MIDI")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"synth"           , param.synth                   , 0      ,4      ,0      ,callback_SynthFunction      ,"0=off 1=MIDI 2=SID")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"mch"             , param.mch                     , 0      ,16     ,0      ,callback_MchFunction        ,"MIDI channel 16=write to all channels")
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"attack"          , midich[0].attack              , 0      ,127    ,0      ,callback_MchCopyFunction    ,"MIDI attack time of mch") //WARNING: Must be mch index +1
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"decay"           , midich[0].decay               , 0      ,127    ,0      ,callback_MchCopyFunction    ,"MIDI decay time of mch")  //WARNING: Must be mch index +2
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"release"         , midich[0].release             , 0      ,127    ,0      ,callback_MchCopyFunction    ,"MIDI release time of mch")//WARNING: Must be mch index +3
    ADD_PARAM(PARAM_DEFAULT ,pdTRUE ,"display"         , param.display                 , 0      ,4      ,0      ,NULL                        ,"Actual display frame")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"watchdog"        , configuration.watchdog        , 0      ,1      ,0      ,callback_ConfigFunction     ,"Watchdog Enable")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_tr_pw"       , configuration.max_tr_pw       , 0      ,3000   ,0      ,callback_ConfigFunction     ,"Maximum TR PW [uSec]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_tr_prf"      , configuration.max_tr_prf      , 0      ,3000   ,0      ,callback_ConfigFunction     ,"Maximum TR frequency [Hz]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_qcw_pw"      , configuration.max_qcw_pw      , 0      ,5000   ,100    ,callback_ConfigFunction     ,"Maximum QCW PW [ms]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_tr_current"  , configuration.max_tr_current  , 0      ,8000   ,0      ,callback_TTupdateFunction   ,"Maximum TR current [A]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"min_tr_current"  , configuration.min_tr_current  , 0      ,8000   ,0      ,callback_ConfigFunction     ,"Minimum TR current [A]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_qcw_current" , configuration.max_qcw_current , 0      ,8000   ,0      ,callback_TTupdateFunction   ,"Maximum QCW current [A]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"temp1_max"       , configuration.temp1_max       , 0      ,100    ,0      ,callback_TTupdateFunction   ,"Max temperature 1 [*C]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"temp2_max"       , configuration.temp2_max       , 0      ,100    ,0      ,NULL                        ,"Max temperature 2 [*C]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct1_ratio"       , configuration.ct1_ratio       , 1      ,5000   ,0      ,callback_TTupdateFunction   ,"CT1 [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct2_ratio"       , configuration.ct2_ratio       , 1      ,5000   ,0      ,callback_ConfigFunction     ,"CT2 [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct3_ratio"       , configuration.ct3_ratio       , 1      ,5000   ,0      ,callback_ConfigFunction     ,"CT3 [N Turns]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct1_burden"      , configuration.ct1_burden      , 1      ,1000   ,10     ,callback_TTupdateFunction   ,"CT1 burden [Ohm]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct2_burden"      , configuration.ct2_burden      , 1      ,1000   ,10     ,callback_ConfigFunction     ,"CT2 burden [Ohm]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct3_burden"      , configuration.ct3_burden      , 1      ,1000   ,10     ,callback_ConfigFunction     ,"CT3 burden [Ohm]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct2_type"        , configuration.ct2_type        , 0      ,1      ,0      ,callback_ConfigFunction     ,"CT2 type 0=current 1=voltage")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct2_current"     , configuration.ct2_current     , 0      ,20000  ,10     ,callback_ConfigFunction     ,"CT2 current @ ct_voltage")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct2_voltage"     , configuration.ct2_voltage     , 0      ,5000   ,1000   ,callback_ConfigFunction     ,"CT2 voltage @ ct_current")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ct2_offset"      , configuration.ct2_offset      , 0      ,5000   ,1000   ,callback_ConfigFunction     ,"CT2 offset voltage")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"lead_time"       , configuration.lead_time       , 0      ,2000   ,0      ,callback_ConfigFunction     ,"Lead time [nSec]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"start_freq"      , configuration.start_freq      , 0      ,5000   ,10     ,callback_ConfigFunction     ,"Resonant freq [kHz]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"start_cycles"    , configuration.start_cycles    , 0      ,20     ,0      ,callback_ConfigFunction     ,"Start Cyles [N]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_tr_duty"     , configuration.max_tr_duty     , 1      ,500    ,10     ,callback_ConfigFunction     ,"Max TR duty cycle [%]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_qcw_duty"    , configuration.max_qcw_duty    , 1      ,500    ,10     ,callback_ConfigFunction     ,"Max QCW duty cycle [%]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"temp1_setpoint"  , configuration.temp1_setpoint  , 0      ,100    ,0      ,NULL                        ,"Setpoint for fan [*C]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ps_scheme"       , configuration.ps_scheme       , 0      ,5      ,0      ,callback_ConfigFunction     ,"Power supply scheme")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"charge_delay"    , configuration.chargedelay     , 1      ,60000  ,0      ,callback_ConfigFunction     ,"Delay for the charge relay [ms]")  
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"autotune_s"      , configuration.autotune_s      , 1      ,32     ,0      ,NULL                        ,"Number of samples for Autotune")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ud_name"         , configuration.ud_name         , 0      ,0      ,0      ,NULL                        ,"Name of the Coil [15 chars]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"min_enable"      , configuration.minprot         , 0      ,1      ,0      ,NULL                        ,"Use MIN-Protocol")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_const_i"     , configuration.max_const_i     , 0      ,2000   ,10     ,callback_i2tFunction        ,"Maximum constant current [A] 0=off")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_fault_i"     , configuration.max_fault_i     , 0      ,2000   ,10     ,callback_i2tFunction        ,"Maximum fault current for 10s [A]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"baudrate"        , configuration.baudrate        , 1200   ,4000000,0      ,callback_baudrateFunction   ,"Serial baudrate")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ivo_uart"        , configuration.ivo_uart        , 0      ,11     ,0      ,callback_ivoUART            ,"[RX][TX] 0=not inverted 1=inverted")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"linecode"        , configuration.line_code       , 0      ,1      ,0      ,callback_ivoUART            ,"0=UART 1=Manchester")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"spi_speed"       , configuration.spi_speed       , 10     ,160    ,10     ,callback_SPIspeedFunction   ,"SPI speed [MHz]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"r_bus"           , configuration.r_top           , 100    ,1000000,1000   ,callback_TTupdateFunction   ,"Series resistor of voltage input [kOhm]")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ena_display"     , configuration.enable_display  , 0      ,6      ,0      ,NULL                        ,"Enables the WS2812 display")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"synth_filter"    , configuration.synth_filter    , 0      ,0      ,0      ,callback_synthFilter        ,"Synthesizer filter string")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"pid_curr_p"      , configuration.pid_curr_p      , 0      ,200    ,0      ,callback_pid                ,"Current PI")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"pid_curr_i"      , configuration.pid_curr_i      , 0      ,200    ,0      ,callback_pid                ,"Current PI")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"max_dc_curr"     , configuration.max_dc_curr     , 0      ,2000   ,10     ,callback_pid                ,"Maximum DC-Bus current [A] 0=off")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"ena_ext_int"     , configuration.ext_interrupter , 0      ,2      ,0      ,callback_ext_interrupter    ,"Enable external interrupter 2=inverted")
    ADD_PARAM(PARAM_CONFIG  ,pdTRUE ,"sid_noise"       , configuration.noise_w         , 0     , 8      ,0      ,NULL                        ,"SID noise weight")
    ADD_PARAM(PARAM_CONFIG  ,pdFALSE,"d_calib"         , vdriver_lut                   , 0      ,0      ,0      ,NULL                        ,"For voltage measurement")
};

   
void eeprom_load(TERMINAL_HANDLE * handle){
    EEPROM_read_conf(confparam, PARAM_SIZE(confparam) ,0,handle);
    i2t_set_limit(configuration.max_const_i,configuration.max_fault_i,10000);
    update_ivo_uart();
    update_visibilty();
    uart_baudrate(configuration.baudrate);
    spi_speed(configuration.spi_speed);
    callback_synthFilter(NULL,0, handle);
    ramp.changed = pdTRUE;
    qcw_regenerate_ramp();
    init_telemetry();
    callback_pid(confparam,0,handle);
    callback_ext_interrupter(confparam,0,handle);
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

}

// clang-format on


/*****************************************************************************
* Callback for invert option UART
******************************************************************************/

void update_ivo_uart(){
    IVO_UART_Control=0;
    switch(configuration.ivo_uart){
    case 0:
        break;
    case 1:
        set_bit(IVO_UART_Control,1);
        break;
    case 10:
        set_bit(IVO_UART_Control,0);
        break;
    case 11:
        set_bit(IVO_UART_Control,0);
        set_bit(IVO_UART_Control,1);
        break;
    }
    
    if(configuration.line_code==1){
        set_bit(IVO_UART_Control,2);
        set_bit(IVO_UART_Control,3);
        toggle_bit(IVO_UART_Control,0);
    }
}

uint8_t callback_ivoUART(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    if(configuration.ivo_uart == 0 || configuration.ivo_uart == 1 || configuration.ivo_uart == 10 || configuration.ivo_uart == 11){
        update_ivo_uart();   
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

/*****************************************************************************
* Callback if the MIDI channel is changed
******************************************************************************/
uint8_t callback_MchFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
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
uint8_t callback_MchCopyFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
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
    system_fault_Control = 0; //halt tesla coil operation during updates!
    
    configure_ZCD_to_PWM();
    
    system_fault_Control = sfflag;
    
    if (portM->term_mode!=PORT_TERM_VT100) {
        uint8_t include_chart;
        if (portM->term_mode==PORT_TERM_TT) {
            include_chart = pdTRUE;
        } else {
            include_chart = pdFALSE;
        }
        init_tt(include_chart,handle);
    }
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

uint8_t callback_SPIspeedFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    spi_speed(configuration.spi_speed);
 
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
    uint32_t uart_frequency = BCLK__BUS_CLK__HZ / divider_selected;
    uint32_t delay_tmr = ((BCLK__BUS_CLK__HZ / uart_frequency)*3)/4;

    Mantmr_WritePeriod(delay_tmr-3);
    UART_CLK_SetDividerValue(divider_selected);
    
    tt.n.rx_datarate.max = baudrate / 8;
    tt.n.tx_datarate.max = baudrate / 8;
    
    UART_Start();  
    
}

uint8_t callback_baudrateFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    uart_baudrate(configuration.baudrate);
 
    return 1;
}

enum burst_state{
    BURST_ON,
    BURST_OFF
};

/*****************************************************************************
* Callback if a transient mode parameter is changed
* Updates the interrupter hardware
******************************************************************************/
uint8_t callback_TRFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle) {
    
    uint32_t temp;
    temp = (1000 * param.pw) / configuration.max_tr_pw;
    param.pwd = temp;

	interrupter.pw = param.pw;
	interrupter.prd = param.pwd;
    
    update_midi_duty();
    
	if (tr_running==1) {
		update_interrupter();
	}
    if(configuration.ext_interrupter){
        interrupter_update_ext();
    }
	return pdPASS;
}

/*****************************************************************************
* Callback if a transient mode parameter is changed (percent ontime)
* Updates the interrupter hardware
******************************************************************************/
uint8_t callback_TRPFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle) {
    
    uint32_t temp;
    temp = (configuration.max_tr_pw * param.pwp) / 1000;
    param.pw = temp;
    
    interrupter.pw = param.pw;
    
    update_midi_duty();
    
	if (tr_running==1) {
		update_interrupter();
	}
    if(configuration.ext_interrupter){
        interrupter_update_ext();
    }
    
	return pdPASS;
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
uint8_t callback_BurstFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle) {
    if(tr_running){
        if(xBurst_Timer==NULL && param.burst_on > 0){
            burst_state = BURST_ON;
            xBurst_Timer = xTimerCreate("Bust-Tmr", param.burst_on / portTICK_PERIOD_MS, pdFALSE,(void * ) 0, vBurst_Timer_Callback);
            if(xBurst_Timer != NULL){
                xTimerStart(xBurst_Timer, 0);
                ttprintf("Burst Enabled\r\n");
                tr_running=2;
            }else{
                interrupter.pw = 0;
                ttprintf("Cannot create burst Timer\r\n");
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
                    ttprintf("\r\nBurst Disabled\r\n");
                }else{
                    ttprintf("Cannot delete burst Timer\r\n");
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
                    ttprintf("\r\nBurst Disabled\r\n");
                }else{
                    ttprintf("Cannot delete burst Timer\r\n");
                    burst_state = BURST_ON;
                }
            }

        }
    }
	return pdPASS;
}

/*****************************************************************************
* Callback if a configuration relevant parameter is changed
******************************************************************************/
uint8_t callback_ConfigFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    uint8 sfflag = system_fault_Read();
    system_fault_Control = 0; //halt tesla coil operation during updates!
    WD_enable(configuration.watchdog);
    initialize_interrupter();
	initialize_charging();
	configure_ZCD_to_PWM();
    update_visibilty();
    reconfig_charge_timer();
    
    recalc_telemetry_limits();
    
	system_fault_Control = sfflag;
    return 1;
}

/*****************************************************************************
* Default function if a parameter is changes (not used)
******************************************************************************/
uint8_t callback_DefaultFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    
    return 1;
}

/*****************************************************************************
* Callback for overcurrent module
******************************************************************************/
uint8_t callback_i2tFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    i2t_set_limit(configuration.max_const_i,configuration.max_fault_i,10000);
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
  
    while(Term_check_break(handle,200)){
        TERM_sendVT100Code(handle, _VT100_CURSOR_POS1,0);
        ttprintf("MIN monitor    (press [CTRL+C] for quit)\r\n");
        ttprintf("%sDropped frames        : %lu\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0), min_ctx.transport_fifo.dropped_frames);
        ttprintf("%sSpurious acks         : %lu\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),min_ctx.transport_fifo.spurious_acks);
        ttprintf("%sResets received       : %lu\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),min_ctx.transport_fifo.resets_received);
        ttprintf("%sSequence mismatch drop: %lu\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),min_ctx.transport_fifo.sequence_mismatch_drop);
        ttprintf("%sMax frames in buffer  : %u\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),min_ctx.transport_fifo.n_frames_max);
        ttprintf("%sCRC errors            : %u\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),min_ctx.transport_fifo.crc_fails);
        ttprintf("%sRemote Time           : %u\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),time.remote);
        ttprintf("%sLocal Time            : %u\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),l_time);
        ttprintf("%sDiff Time             : %i\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),time.diff);
        ttprintf("%sResync count          : %u\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),time.resync);
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
   // uint16_t vdriver_lut[9] = {0,3500,7000,10430,13840,17310,20740,24200,27657};
    uint32_t temp;
    TERM_sendVT100Code(handle, _VT100_CURSOR_DISABLE, 0);
    TERM_sendVT100Code(handle, _VT100_CLS, 0);
    ttprintf("Driver voltage measurement calibration [y] for next [a] for abort\r\n");
    ttprintf("Set Vdriver to 7V\r\n");
    if(getch(handle, portMAX_DELAY) != 'y') return TERM_CMD_EXIT_SUCCESS;
    temp = 7000*512/ADC_active_sample_buf[0].v_driver;
    vdriver_lut[2]= temp;
    vdriver_lut[1]= temp/2;
    ttprintf("Set Vdriver to 10V\r\n");
    if(getch(handle, portMAX_DELAY) != 'y') return TERM_CMD_EXIT_SUCCESS;
    temp = 10000*768/ADC_active_sample_buf[0].v_driver;
    vdriver_lut[3]= temp;
    ttprintf("Set Vdriver to 14V\r\n");
    if(getche(handle, portMAX_DELAY) != 'y') return TERM_CMD_EXIT_SUCCESS;
    temp = 14000*1024/ADC_active_sample_buf[0].v_driver;
    vdriver_lut[4]= temp;
    ttprintf("Set Vdriver to 17V\r\n");
    if(getch(handle, portMAX_DELAY) != 'y') return TERM_CMD_EXIT_SUCCESS;
    temp = 17000*1280/ADC_active_sample_buf[0].v_driver;
    vdriver_lut[5]= temp;
    ttprintf("Set Vdriver to 20V\r\n");
    if(getch(handle, portMAX_DELAY) != 'y') return TERM_CMD_EXIT_SUCCESS;
    temp = 20000*1536/ADC_active_sample_buf[0].v_driver;
    vdriver_lut[6]= temp;
    ttprintf("Set Vdriver to 24V\r\n");
    if(getch(handle, portMAX_DELAY) != 'y') return TERM_CMD_EXIT_SUCCESS;
    temp = 24000*1792/ADC_active_sample_buf[0].v_driver;
    vdriver_lut[7]= temp;
    temp = temp*2048/1792;
    vdriver_lut[8]= temp;
    ttprintf("Calibration finished\r\n");
    TERM_sendVT100Code(handle, _VT100_CURSOR_ENABLE, 0);
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
* starts or stops the transient mode (classic mode)
* also spawns a timer for the burst mode.
******************************************************************************/
uint8_t CMD_tr(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args) {
    
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf("Transient [start/stop]");
        return TERM_CMD_EXIT_SUCCESS;
    }
    
    if(strcmp(args[0], "start") == 0){
        isr_interrupter_Disable();
        interrupter.pw = param.pw;
		interrupter.prd = param.pwd;
        update_interrupter();

		tr_running = 1;
        
        callback_BurstFunction(NULL, 0, handle);
        
		ttprintf("Transient Enabled\r\n");
       
        return TERM_CMD_EXIT_SUCCESS;
    }

	if(strcmp(args[0], "stop") == 0){
        isr_interrupter_Enable();
        if (xBurst_Timer != NULL) {
			if(xTimerDelete(xBurst_Timer, 100 / portTICK_PERIOD_MS) != pdFALSE){
			    xBurst_Timer = NULL;
                burst_state = BURST_ON;
            }
        }

        ttprintf("Transient Disabled\r\n");    
 
		interrupter.pw = 0;
		update_interrupter();
		tr_running = 0;
		
		return TERM_CMD_EXIT_SUCCESS;
	}
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
    	USBMIDI_1_callbackLocalMidiEvent(0, (uint8_t*)kill_msg);
    	bus_command = BUS_COMMAND_OFF;
        
        QCW_delete_timer();
        
    	interrupter1_control_Control = 0;
    	QCW_enable_Control = 0;
    	TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_GREEN);
    	ttprintf("Killbit set\r\n");
        alarm_push(ALM_PRIO_CRITICAL,warn_kill_set, ALM_NO_VALUE);
    	TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);
        return TERM_CMD_EXIT_SUCCESS;
    }else if(strcmp(args[0], "reset") == 0){
        interrupter_unkill();
        reset_fault();
        system_fault_Control = 0xFF;
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_GREEN);
    	ttprintf("Killbit reset\r\n");
        alarm_push(ALM_PRIO_INFO,warn_kill_reset, ALM_NO_VALUE);
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
        ttprintf("Usage: eprom [load|save]\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    }
    EEPROM_1_UpdateTemperature();
	uint8 sfflag = system_fault_Read();
	system_fault_Control = 0; //halt tesla coil operation during updates!
	if(strcmp(args[0], "save") == 0){
        EEPROM_check_hash(confparam,PARAM_SIZE(confparam),handle);
	    EEPROM_write_conf(confparam, PARAM_SIZE(confparam),0, handle);

		system_fault_Control = sfflag;
		return TERM_CMD_EXIT_SUCCESS;
	}
	if(strcmp(args[0], "load") == 0){
        uint8 sfflag = system_fault_Read();
        system_fault_Control = 0; //halt tesla coil operation during updates!
		EEPROM_read_conf(confparam, PARAM_SIZE(confparam) ,0,handle);
        
        initialize_interrupter();
	    initialize_charging();
	    configure_ZCD_to_PWM();
	    system_fault_Control = sfflag;
		return TERM_CMD_EXIT_SUCCESS;
	}
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
* Resets the software fuse
******************************************************************************/
uint8_t CMD_fuse(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    i2t_reset();
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
        Relay3_Write(value);
        return TERM_CMD_EXIT_SUCCESS;
    }
    if(r_number == 4){
        Relay4_Write(value);
        return TERM_CMD_EXIT_SUCCESS;
    }
    return TERM_CMD_EXIT_SUCCESS;
}


/*****************************************************************************
* Signal debugging
******************************************************************************/

void send_signal_state(uint8_t signal, uint8_t inverted, TERMINAL_HANDLE * handle){
    if(inverted) signal = !signal; 
    if(signal){
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_RED);
        ttprintf("true \r\n");
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);  
    }else{
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_GREEN);
        ttprintf("false\r\n");
        TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, _VT100_WHITE);;
    }
}
void send_signal_state_wo(uint8_t signal, uint8_t inverted, TERMINAL_HANDLE * handle){
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
        send_signal_state_wo_new(UVLO_status_Status,pdTRUE,handle);
        ttprintf(" Crystal clock: ");
        send_signal_state_new((CY_GET_XTND_REG8((void CYFAR *)CYREG_FASTCLK_XMHZ_CSR) & 0x80u),pdTRUE,handle);
        
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
        ttprintf("Sysfault bus undervoltage: ");
        send_signal_state_new(sysfault.bus_uv,pdFALSE,handle);
        ttprintf("Sysfault interlock: ");
        send_signal_state_wo_new(sysfault.interlock,pdFALSE,handle);
        ttprintf(" link: ");
        send_signal_state_wo_new(sysfault.link_state,pdFALSE,handle);
        ttprintf(" combined: ");
        send_signal_state_new(system_fault_Read(),pdTRUE,handle);
        
        ttprintf("Relay 1: ");
        send_signal_state_wo_new((relay_Read()&0b1),pdFALSE,handle);
        ttprintf(" Relay 2: ");
        send_signal_state_wo_new((relay_Read()&0b10),pdFALSE,handle);
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
        ttprintf("Temp 1: %i*C Temp 2: %i*C\r\n", tt.n.temp1.value, tt.n.temp2.value);
        ttprintf("                                    \r");
        ttprintf("Vbus: %u mV Vbatt: %u mV\r\n", ADC_CountsTo_mVolts(ADC_active_sample_buf[0].v_bus),ADC_CountsTo_mVolts(ADC_active_sample_buf[0].v_batt));
        ttprintf("                                    \r");
        ttprintf("Ibus: %u mV Vdriver: %u mV\r\n\r\n", ADC_CountsTo_mVolts(ADC_active_sample_buf[0].i_bus),tt.n.driver_v.value);

    }while(Term_check_break(handle,250));
    
    TERM_sendVT100Code(handle, _VT100_RESET_ATTRIB, 0);

    return TERM_CMD_EXIT_SUCCESS;
}

