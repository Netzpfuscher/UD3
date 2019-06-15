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

#include "cyapicallbacks.h"
#include <cytypes.h>

#include "tsk_analog.h"
#include "tsk_fault.h"
#include "alarmevent.h"

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "interrupter.h"

#include <stdlib.h>
#include "math.h"


xTaskHandle tsk_analog_TaskHandle;
uint8 tsk_analog_initVar = 0u;

SemaphoreHandle_t adc_ready_Semaphore;

xQueueHandle adc_data;

TimerHandle_t xCharge_Timer;

#define NEW_DATA_RATE_MS 3.125  //ms


/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */
#include "ZCDtoPWM.h"
#include "cli_common.h"
#include "telemetry.h"
#include "tsk_priority.h"
#include "tsk_midi.h"
#include <device.h>

/* Defines for MUX_DMA */
#define MUX_DMA_BYTES_PER_BURST 1
#define MUX_DMA_REQUEST_PER_BURST 1
#define MUX_DMA_SRC_BASE (CYDEV_SRAM_BASE)
#define MUX_DMA_DST_BASE (CYDEV_PERIPH_BASE)

/* DMA Configuration for ADC_DMA */
#define ADC_DMA_BYTES_PER_BURST 2
#define ADC_DMA_REQUEST_PER_BURST 1
#define ADC_DMA_SRC_BASE (CYDEV_PERIPH_BASE)
#define ADC_DMA_DST_BASE (CYDEV_SRAM_BASE)



#define SAMPLES_COUNT 2048

//#define BUSV_R_TOP 500000UL
#define BUSV_R_BOT 5000UL

#define INITIAL 0 /* Initial value of the filter memory. */

typedef struct
{
	uint16_t rms;
	uint64_t sum_squares;
} rms_t;

int16 ADC_sample[4];
uint16 ADC_sample_buf[4];
rms_t current_idc;
uint8_t ADC_mux_ctl[4] = {0x05, 0x02, 0x03, 0x00};

uint32_t i2t_limit=0;
uint32_t i2t_warning=0;
uint32_t i2t_leak=0;
uint8_t i2t_warning_level=60;
uint32_t i2t_integral;

void i2t_set_limit(uint32_t const_current, uint32_t ovr_current, uint32_t limit_ms){
    i2t_leak = const_current * const_current;
    i2t_limit= floor((float)((float)limit_ms/NEW_DATA_RATE_MS)*(float)((ovr_current*ovr_current)-i2t_leak));
    i2t_set_warning(i2t_warning_level);
}

void i2t_set_warning(uint8_t percent){
    i2t_warning_level = percent;
    i2t_warning = floor((float)i2t_limit*((float)percent/100.0));
}

void i2t_reset(){
    i2t_integral=0;
}

uint8_t i2t_calculate(){
    uint32_t squaredCurrent = (uint32_t)telemetry.batt_i * (uint32_t)telemetry.batt_i;
    i2t_integral = i2t_integral + squaredCurrent;
	if(i2t_integral > i2t_leak)
	{
		i2t_integral -= i2t_leak;
	}
	else
	{
		i2t_integral = 0;
	}
    
    telemetry.i2t_i=(100*(i2t_integral>>8))/(i2t_limit>>8);
    
    if(i2t_integral < i2t_warning)
	{
		return I2T_NORMAL;
	}
	else
	{
		if(i2t_integral < i2t_limit)
		{
			return I2T_WARNING;
		}
		else
		{
            i2t_integral=i2t_limit;
			return I2T_LIMIT;
		}
	}
  
}

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */

CY_ISR(ADC_data_ready_ISR) {
	
    xQueueSendFromISR(adc_data, ADC_sample_buf, NULL);

    if (uxQueueMessagesWaitingFromISR(adc_data) > 25) {
		xSemaphoreGiveFromISR(adc_ready_Semaphore, NULL);
	}
}

CY_ISR(isr_primary) {
    telemetry.primary_i = CT1_Get_Current(CT_PRIMARY);
    /*if(telemetry.primary_i > configuration.max_tr_current){
        ct1_dac_val[0]--;
    }*/
}



uint32_t read_bus_mv(uint16_t raw_adc) {
	uint32_t bus_voltage;
	bus_voltage = ((configuration.r_top + BUSV_R_BOT) * raw_adc) / (BUSV_R_BOT * 819 / 1000);
	return bus_voltage;
}

uint16_t CT1_Get_Current(uint8_t channel) {
	uint32_t result = ADC_DelSig_1_GetResult32();

	if (channel == CT_PRIMARY) {
		return ((ADC_DelSig_1_CountsTo_mVolts(result) * configuration.ct1_ratio) / configuration.ct1_burden) / 100;
	} else {
		return ((ADC_DelSig_1_CountsTo_mVolts(result) * configuration.ct3_ratio) / configuration.ct3_burden) / 100;
	}
}

float CT1_Get_Current_f(uint8_t channel) {
	uint32_t result = ADC_DelSig_1_GetResult32();

	if (channel == CT_PRIMARY) {
		return ((float)(ADC_DelSig_1_CountsTo_Volts(result) * 10) / (float)(configuration.ct1_burden) * configuration.ct1_ratio);
	} else {
		return ((float)(ADC_DelSig_1_CountsTo_Volts(result) * 10) / (float)(configuration.ct3_burden) * configuration.ct3_ratio);
	}
}

void init_rms_filter(rms_t *ptr, uint16_t init_val) {
	ptr->rms = init_val;
	ptr->sum_squares = 1UL * SAMPLES_COUNT * init_val * init_val;
}

uint16_t rms_filter(rms_t *ptr, uint16_t sample) {
	ptr->sum_squares -= ptr->sum_squares / SAMPLES_COUNT;
	ptr->sum_squares += (uint32_t)sample * sample;
	if (ptr->rms == 0)
		ptr->rms = 1; /* do not divide by zero */
	ptr->rms = (ptr->rms + ptr->sum_squares / SAMPLES_COUNT / ptr->rms) / 2;
	return ptr->rms;
}

void init_average_filter(uint32_t *ptr, uint16_t init_val) {
	*ptr = 1UL * SAMPLES_COUNT * init_val;
}

uint16_t average_filter(uint32_t *ptr, uint16_t sample) {
	*ptr -= *ptr / SAMPLES_COUNT;
	*ptr += (uint32_t)sample;
	*ptr = *ptr / SAMPLES_COUNT;
	return *ptr;
}

void calculate_rms(void) {
    static uint16_t count=0;
	while (uxQueueMessagesWaiting(adc_data)) {

		xQueueReceive(adc_data, ADC_sample, portMAX_DELAY);

		// read the battery voltage
		telemetry.batt_v = read_bus_mv(ADC_sample[DATA_VBATT]) / 1000;

		// read the bus voltage
		telemetry.bus_v = read_bus_mv(ADC_sample[DATA_VBUS]) / 1000;

		// read the battery current
        if(configuration.ct2_type==CT2_TYPE_CURRENT){
		    telemetry.batt_i = (((uint32_t)rms_filter(&current_idc, ADC_sample[DATA_IBUS]) * params.idc_ma_count) / 100);
        }else{
            ADC_sample[DATA_IBUS]-=params.ct2_offset_cnt;
            if(ADC_sample[DATA_IBUS]<0) ADC_sample[DATA_IBUS]=0;
            telemetry.batt_i = (((uint32_t)rms_filter(&current_idc, ADC_sample[DATA_IBUS]) * params.idc_ma_count) / 100);
        }

		telemetry.avg_power = telemetry.batt_i * telemetry.bus_v / 10;
	}
    
    if(configuration.max_const_i){  //Only do i2t calculation if enabled
        if(count<100){
            int16_t e= abs(telemetry.batt_i-configuration.max_const_i);
            count += e;
        }else{
            count = 0;   
        }
        switch (i2t_calculate()){
            case I2T_LIMIT:
                interrupter_kill();   
                break;
            case I2T_WARNING:
                if(telemetry.batt_i>configuration.max_const_i && !count){
                    if(param.temp_duty<configuration.max_tr_duty) param.temp_duty++; 
                    if(tr_running==1){
                        update_interrupter();
                    }else{
                        update_midi_duty();
                    }
                }
                break;
            case I2T_NORMAL:
                if(telemetry.batt_i<configuration.max_const_i && !count){
                    if(param.temp_duty>0) param.temp_duty--; 
                    if(tr_running==1){
                        update_interrupter();
                    }else{
                        update_midi_duty();
                    }
                }
                break;
                
        }
    }
    
	control_precharge();

}



void initialize_analogs(void) {
	
	CT_MUX_Start();
	ADC_DelSig_1_Start();
	Sample_Hold_1_Start();
	Comp_1_Start();

	/* Variable declarations for ADC_DMA */
	/* Move these variable declarations to the top of the function */
	uint8 ADC_DMA_Chan;
	uint8 ADC_DMA_TD[1];

	ADC_DMA_Chan = ADC_DMA_DmaInitialize(ADC_DMA_BYTES_PER_BURST, ADC_DMA_REQUEST_PER_BURST, HI16(ADC_DMA_SRC_BASE), HI16(ADC_DMA_DST_BASE));
	ADC_DMA_TD[0] = CyDmaTdAllocate();
	CyDmaTdSetConfiguration(ADC_DMA_TD[0], 8, ADC_DMA_TD[0], ADC_DMA__TD_TERMOUT_EN | TD_INC_DST_ADR);
	CyDmaTdSetAddress(ADC_DMA_TD[0], LO16((uint32)ADC_SAR_WRK0_PTR), LO16((uint32)ADC_sample_buf));
	CyDmaChSetInitialTd(ADC_DMA_Chan, ADC_DMA_TD[0]);
	CyDmaChEnable(ADC_DMA_Chan, 1);

	/* Variable declarations for MUX_DMA */
	/* Move these variable declarations to the top of the function */
	uint8 MUX_DMA_Chan;
	uint8 MUX_DMA_TD[1];

	/* DMA Configuration for MUX_DMA */
	MUX_DMA_Chan = MUX_DMA_DmaInitialize(MUX_DMA_BYTES_PER_BURST, MUX_DMA_REQUEST_PER_BURST, HI16(MUX_DMA_SRC_BASE), HI16(MUX_DMA_DST_BASE));
	MUX_DMA_TD[0] = CyDmaTdAllocate();
	CyDmaTdSetConfiguration(MUX_DMA_TD[0], 4, MUX_DMA_TD[0], CY_DMA_TD_INC_SRC_ADR);
	CyDmaTdSetAddress(MUX_DMA_TD[0], LO16((uint32)ADC_mux_ctl), LO16((uint32)Amux_Ctrl_Control_PTR));
	CyDmaChSetInitialTd(MUX_DMA_Chan, MUX_DMA_TD[0]);
	CyDmaChEnable(MUX_DMA_Chan, 1);

	ADC_data_ready_StartEx(ADC_data_ready_ISR);

	CT_MUX_Select(CT_PRIMARY);

	init_rms_filter(&current_idc, INITIAL);
    
    ADC_Start();
}

uint32 low_battery_counter;
int16 initial_vbus, final_vbus, delta_vbus;
uint32 charging_counter;
uint8_t timer_triggerd=0;


void initialize_charging(void) {
	telemetry.bus_status = BUS_OFF;
	if (configuration.ps_scheme == BAT_BOOST_BUS_SCHEME) {
		SLR_Control = 0;
		SLRPWM_Start();
		if (configuration.slr_fswitch == 0) {
			configuration.slr_fswitch = 500; //just in case it wasnt ever programmed
		}
		uint16 x;
		x = (320000 / (configuration.slr_fswitch));
		if ((x % 2) != 0)
			x++; //we want x to be even
		SLRPWM_WritePeriod(x + 1);
		SLRPWM_WriteCompare(x >> 1);
	}
	initial_vbus = 0;
	final_vbus = 0;
	charging_counter = 0;
}

void bat_precharge_bus_scheme(){
    uint32 v_threshold;
    v_threshold = (float)telemetry.batt_v * 0.95;
	if (telemetry.batt_v >= (configuration.batt_lockout_v - 1)) {
		if (telemetry.bus_v >= v_threshold) {
            if(!timer_triggerd && telemetry.bus_status == BUS_CHARGING){
                timer_triggerd=1;
                alarm_push(ALM_PRIO_INFO,warn_bus_charging);
			    xTimerStart(xCharge_Timer,0);
            }
		} else if (telemetry.bus_status != BUS_READY) {
			telemetry.bus_status = BUS_CHARGING;
			relay_Write(RELAY_CHARGE);
		}
		low_battery_counter = 0;
	} else {
		low_battery_counter++;
		if (low_battery_counter > LOW_BATTERY_TIMEOUT) {
			relay_Write(RELAY_OFF);
			telemetry.bus_status = BUS_BATT_UV_FLT;
			bus_command = BUS_COMMAND_FAULT;
		}
	}
}

void bat_boost_bus_scheme(){
    if (telemetry.bus_v < (configuration.slr_vbus - 15)) {
		SLR_Control = 1;
		telemetry.bus_status = BUS_READY; //its OK to operate the TC when charging from SLR
	} else if (telemetry.bus_v > configuration.slr_vbus) {
		SLR_Control = 0;
		telemetry.bus_status = BUS_READY;
	}

	if (telemetry.batt_v >= (configuration.batt_lockout_v - 1)) {
		low_battery_counter = 0;
	} else {
		low_battery_counter++;
		if (low_battery_counter > LOW_BATTERY_TIMEOUT) {
			SLR_Control = 0;
			telemetry.bus_status = BUS_BATT_UV_FLT;
			bus_command = BUS_COMMAND_FAULT;
		}
	}
}

void ac_precharge_bus_scheme(){
	//we cant know the AC line voltage so we will watch the bus voltage climb and infer when its charged by it not increasing fast enough
	//this logic is part of a charging counter
    if(telemetry.bus_status == BUS_READY && telemetry.bus_v <20){
        telemetry.bus_status=BUS_BATT_UV_FLT;
        if(telemetry.sys_fault[SYS_FAULT_BUS_UV]==0){
            alarm_push(ALM_PRIO_ALARM,warn_bus_undervoltage);
        }
        telemetry.sys_fault[SYS_FAULT_BUS_UV]=1;
        bus_command=BUS_COMMAND_FAULT;
    }
	if (charging_counter == 0)
		initial_vbus = telemetry.bus_v;
	charging_counter++;
	if (charging_counter > AC_PRECHARGE_TIMEOUT) {
		final_vbus = telemetry.bus_v;
		delta_vbus = final_vbus - initial_vbus;
		if ((delta_vbus < 4) && (telemetry.bus_v > 20) && telemetry.bus_status == BUS_CHARGING) {
            if(!timer_triggerd){
                timer_triggerd=1;
			    xTimerStart(xCharge_Timer,0);
            }
		} else if (telemetry.bus_status != BUS_READY) {
            if(telemetry.bus_status != BUS_CHARGING){
                alarm_push(ALM_PRIO_INFO,warn_bus_charging);
            }
			relay_Write(RELAY_CHARGE);
			telemetry.bus_status = BUS_CHARGING;
		}
		charging_counter = 0;
	}
}

void ac_dual_meas_scheme(){
}

void ac_precharge_fixed_delay(){
    if(relay_Read()==RELAY_OFF){
        alarm_push(ALM_PRIO_INFO,warn_bus_charging);
        xTimerStart(xCharge_Timer,0);
        relay_Write(RELAY_CHARGE);
        telemetry.bus_status = BUS_CHARGING;
    }    
}

void vCharge_Timer_Callback(TimerHandle_t xTimer){
    timer_triggerd=0;
    if(bus_command== BUS_COMMAND_ON){
        if(relay_Read()==RELAY_CHARGE){
            alarm_push(ALM_PRIO_INFO,warn_bus_ready);
            relay_Write(RELAY_ON);
            telemetry.bus_status = BUS_READY;
        }
    }else{
        relay_Write(RELAY_OFF);
        alarm_push(ALM_PRIO_INFO,warn_bus_off);
        telemetry.bus_status = BUS_OFF;
    }
}

void control_precharge(void) { //this gets called from tsk_analogs.c when the ADC data set is ready, 8khz rep rate

	if (bus_command == BUS_COMMAND_ON) {
        switch(configuration.ps_scheme){
            case BAT_PRECHARGE_BUS_SCHEME:
                bat_precharge_bus_scheme();
            break;
            case BAT_BOOST_BUS_SCHEME:
                bat_boost_bus_scheme();
            break;
            case AC_PRECHARGE_BUS_SCHEME:
                ac_precharge_bus_scheme();
            break;
            case AC_DUAL_MEAS_SCHEME:
                ac_dual_meas_scheme();
            break;
            case AC_PRECHARGE_FIXED_DELAY:
                ac_precharge_fixed_delay();
            break;
        } 
	} else {
		if ((relay_Read()==RELAY_ON || relay_Read()==RELAY_CHARGE) && timer_triggerd==0){
			relay_Write(RELAY_CHARGE);
            timer_triggerd=1;
            xTimerStart(xCharge_Timer,0);
		}
		SLR_Control = 0;
	}
}

void reconfig_charge_timer(){
    if(xCharge_Timer!=NULL){
        xTimerChangePeriod(xCharge_Timer,configuration.chargedelay / portTICK_PERIOD_MS,0);
    }
}

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * This is the main procedure that comprises the task.  Place the code required
 * to preform the desired function within the merge regions of the task procedure
 * to add functionality to the task.
 */
void tsk_analog_TaskProc(void *pvParameters) {
	/*
	 * Add and initialize local variables that are allocated on the Task stack
	 * the the section below.
	 */
	/* `#START TASK_VARIABLES` */

	adc_data = xQueueCreate(128, sizeof(ADC_sample));
    
    xCharge_Timer = xTimerCreate("Chrg-Tmr", configuration.chargedelay / portTICK_PERIOD_MS, pdFALSE,(void * ) 0, vCharge_Timer_Callback);

    /* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */

	adc_ready_Semaphore = xSemaphoreCreateBinary();

	initialize_analogs();
    
    isr_primary_current_StartEx(isr_primary);
    
    alarm_push(ALM_PRIO_INFO,warn_task_analog);

	/* `#END` */

	for (;;) {
		/* `#START TASK_LOOP_CODE` */
		xSemaphoreTake(adc_ready_Semaphore, portMAX_DELAY);

		calculate_rms();

		/* `#END` */

		//vTaskDelay( 10/ portTICK_PERIOD_MS);
	}
}
/* ------------------------------------------------------------------------ */
void tsk_analog_Start(void) {
	/*
	 * Insert task global memeory initialization here. Since the OS does not
	 * initialize ANY global memory, execute the initialization here to make
	 * sure that your task data is properly 
	 */
	/* `#START TASK_GLOBAL_INIT` */

	/* `#END` */

	if (tsk_analog_initVar != 1) {

		/*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_analog_TaskProc, "Analog", STACK_ANALOG, NULL, PRIO_ANALOG, &tsk_analog_TaskHandle);
		tsk_analog_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
