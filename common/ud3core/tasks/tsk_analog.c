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
#include "helper/FastPID.h"

#include <stdlib.h>
#include "math.h"


xTaskHandle tsk_analog_TaskHandle;
uint8 tsk_analog_initVar = 0u;

SemaphoreHandle_t adc_ready_Semaphore;

xQueueHandle adc_data;

TimerHandle_t xCharge_Timer;


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

#define INITIAL 0 /* Initial value of the filter memory. */

typedef struct
{
	uint16_t rms;
	uint64_t sum_squares;
} rms_t;


adc_sample_t ADC_sample_buf_0[ADC_BUFFER_CNT];
adc_sample_t ADC_sample_buf_1[ADC_BUFFER_CNT];
adc_sample_t *ADC_active_sample_buf = ADC_sample_buf_0;

rms_t current_idc;
rms_t voltage_bus;
rms_t voltage_batt;

uint8_t ADC_mux_ctl[4] = {0x05, 0x02, 0x03, 0x00};
static uint32_t drive_top_r_corrected = DRIVEV_R_TOP;

static uint32_t vdriver_raw;

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */

CY_ISR(ADC_data_ready_ISR) {
    if(ADC_active_sample_buf==ADC_sample_buf_0 ){
        ADC_active_sample_buf = ADC_sample_buf_1;
    } else {
        ADC_active_sample_buf = ADC_sample_buf_0;
    }
	xSemaphoreGiveFromISR(adc_ready_Semaphore, NULL);
}


uint32_t read_bus_mv(uint16_t raw_adc) {
	uint32_t bus_voltage;
	bus_voltage = ((configuration.r_top + BUSV_R_BOT) * raw_adc) / (BUSV_R_BOT * 819 / 1000);
	return bus_voltage;
}

void tsk_analog_recalc_drive_top(float factor){
    float scaled_val = (float)DRIVEV_R_TOP * factor;
    drive_top_r_corrected = roundf(scaled_val);
}

uint16_t read_driver_mv(){
	uint32_t driver_voltage = (drive_top_r_corrected + DRIVEV_R_BOT) * vdriver_raw;
    
    if(configuration.hw_rev > 0){
        driver_voltage /= (DRIVEV_R_BOT * 819 / 1000);
    } else {
        driver_voltage /= (DRIVEV_R_BOT_V3 * 819 / 1000);
    }
    
    return driver_voltage;
}

uint32_t CT1_Get_Current() {
    int32_t counts = ADC_peak_GetResult16();
	return ((ADC_peak_CountsTo_mVolts(counts) * configuration.ct1_ratio) / configuration.ct1_burden) / 100;
}

float CT1_Get_Current_f() {
    int32_t counts = ADC_peak_GetResult16();
	return ((float)(ADC_peak_CountsTo_Volts(counts) * 10) / (float)(configuration.ct1_burden) * configuration.ct1_ratio);
}

void init_rms_filter(rms_t *ptr, uint16_t init_val) {
	ptr->rms = init_val;
	ptr->sum_squares = 1UL * SAMPLES_COUNT * init_val * init_val;
}

uint16_t rms_filter(rms_t *ptr, int16_t sample) {
	ptr->sum_squares -= ptr->sum_squares / SAMPLES_COUNT;
	ptr->sum_squares += (int32_t)sample * sample;
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
    
    uint32_t vdriver_accu=0;
    
    for(uint8_t i=0;i<ADC_BUFFER_CNT;i++){

		// read the battery voltage
		tt.n.batt_v.value = read_bus_mv(rms_filter(&voltage_batt, ADC_active_sample_buf[i].v_batt)) / 1000;

		// read the bus voltage
		tt.n.bus_v.value = read_bus_mv(rms_filter(&voltage_bus, ADC_active_sample_buf[i].v_bus)) / 1000;

		// read the battery current
        if(configuration.ct2_type==CT2_TYPE_CURRENT){
		    tt.n.batt_i.value = (((uint32_t)rms_filter(&current_idc, ADC_active_sample_buf[i].i_bus) * params.idc_ma_count) / 100);
        }else{
            tt.n.batt_i.value = ((((int32_t)rms_filter(&current_idc, ADC_active_sample_buf[i].i_bus-params.ct2_offset_cnt)) * params.idc_ma_count) / 100);
        }

		tt.n.avg_power.value = tt.n.batt_i.value * tt.n.bus_v.value / 10;
        
        vdriver_accu += ADC_active_sample_buf[i].v_driver;  
        
	}
    
    vdriver_raw = vdriver_accu / ADC_BUFFER_CNT;
    
    tt.n.primary_i.value = CT1_Get_Current();
      
   
	control_precharge();

}



void initialize_analogs(void) {
    ADC_peak_Start();
	Sample_Hold_1_Start();
	Comp_1_Start();

	/* Variable declarations for ADC_DMA */
	/* Move these variable declarations to the top of the function */
	uint8 ADC_DMA_Chan;
	uint8 ADC_DMA_TD[2];

	ADC_DMA_Chan = ADC_DMA_DmaInitialize(ADC_DMA_BYTES_PER_BURST, ADC_DMA_REQUEST_PER_BURST, HI16(ADC_DMA_SRC_BASE), HI16(ADC_DMA_DST_BASE));
	ADC_DMA_TD[0] = CyDmaTdAllocate();
    ADC_DMA_TD[1] = CyDmaTdAllocate();
	CyDmaTdSetConfiguration(ADC_DMA_TD[0], 8*ADC_BUFFER_CNT, ADC_DMA_TD[1], ADC_DMA__TD_TERMOUT_EN | TD_INC_DST_ADR);
    CyDmaTdSetConfiguration(ADC_DMA_TD[1], 8*ADC_BUFFER_CNT, ADC_DMA_TD[0], ADC_DMA__TD_TERMOUT_EN | TD_INC_DST_ADR);
	CyDmaTdSetAddress(ADC_DMA_TD[0], LO16((uint32)ADC_SAR_WRK0_PTR), LO16((uint32)ADC_sample_buf_0));
    CyDmaTdSetAddress(ADC_DMA_TD[1], LO16((uint32)ADC_SAR_WRK0_PTR), LO16((uint32)ADC_sample_buf_1));
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

	init_rms_filter(&current_idc, INITIAL);
    
    ADC_Start();
}

int16 initial_vbus, final_vbus, delta_vbus;
uint32 charging_counter;
uint8_t timer_triggerd=0;


void initialize_charging(void) {
	tt.n.bus_status.value = BUS_OFF;
	initial_vbus = 0;
	final_vbus = 0;
	charging_counter = 0;
}



void ac_precharge_bus_scheme(){
	//we cant know the AC line voltage so we will watch the bus voltage climb and infer when its charged by it not increasing fast enough
	//this logic is part of a charging counter
    if(tt.n.bus_status.value == BUS_READY && tt.n.bus_v.value <20){
        tt.n.bus_status.value=BUS_BATT_UV_FLT;
        if(sysfault.bus_uv==0){
            alarm_push(ALM_PRIO_ALARM, "BUS: Undervoltage",tt.n.bus_v.value);
        }
        sysfault.bus_uv=1;
        bus_command=BUS_COMMAND_FAULT;
    }
	if (charging_counter == 0)
		initial_vbus = tt.n.bus_v.value;
	charging_counter++;
	if (charging_counter > AC_PRECHARGE_TIMEOUT) {
		final_vbus = tt.n.bus_v.value;
		delta_vbus = final_vbus - initial_vbus;
		if ((delta_vbus < 4) && (tt.n.bus_v.value > 20) && tt.n.bus_status.value == BUS_CHARGING) {
            if(!timer_triggerd){
                timer_triggerd=1;
			    xTimerStart(xCharge_Timer,0);
            }
		} else if (tt.n.bus_status.value != BUS_READY) {
            if(tt.n.bus_status.value != BUS_CHARGING){
                alarm_push(ALM_PRIO_INFO, "BUS: Charging", ALM_NO_VALUE);
            }
            sysfault.charge=1;
			relay_write_bus(1);
			tt.n.bus_status.value = BUS_CHARGING;
		}
		charging_counter = 0;
	}
}

void ac_dual_meas_scheme(){
}

void ac_precharge_fixed_delay(){
    if(!relay_read_bus() && !relay_read_charge_end()){
        alarm_push(ALM_PRIO_INFO, "BUS: Charging", ALM_NO_VALUE);
        sysfault.charge=1;
        xTimerStart(xCharge_Timer,0);
        relay_write_bus(1);
        tt.n.bus_status.value = BUS_CHARGING;
    }    
}

void vCharge_Timer_Callback(TimerHandle_t xTimer){
    timer_triggerd=0;
    if(bus_command== BUS_COMMAND_ON){
        if(relay_read_bus()){
            alarm_push(ALM_PRIO_INFO, "BUS: Ready", ALM_NO_VALUE);
            relay_write_charge_end(1);
            tt.n.bus_status.value = BUS_READY;
            sysfault.charge=0;
            sysfault.bus_uv=0;
        }
    }else{
        relay_write_bus(0);
        relay_write_charge_end(0);
        sysfault.charge=0;
        alarm_push(ALM_PRIO_INFO, "BUS: Off", ALM_NO_VALUE);
        tt.n.bus_status.value = BUS_OFF;
    }
}

void control_precharge(void) { //this gets called from tsk_analogs.c when the ADC data set is ready, 8khz rep rate

	if (bus_command == BUS_COMMAND_ON) {
        switch(configuration.ps_scheme){
            case BAT_PRECHARGE_BUS_SCHEME:
                //Not implemented
            break;
            case BAT_BOOST_BUS_SCHEME:
                //Not implemented
            break;
            case AC_PRECHARGE_BUS_SCHEME:
                ac_precharge_bus_scheme();
            break;
            case AC_DUAL_MEAS_SCHEME:
                ac_dual_meas_scheme();      // Not implemented
            break;
            case AC_PRECHARGE_FIXED_DELAY:
                ac_precharge_fixed_delay();
            break;
        } 
	} else {
		if ((relay_read_charge_end() || relay_read_bus()) && timer_triggerd==0){
            sysfault.charge=1;
			relay_write_bus(1);
            relay_write_charge_end(0);
            timer_triggerd=1;
            xTimerStart(xCharge_Timer,0);
		}
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

	//adc_data = xQueueCreate(128, sizeof(ADC_sample));
    
    xCharge_Timer = xTimerCreate("Chrg-Tmr", configuration.chargedelay / portTICK_PERIOD_MS, pdFALSE,(void * ) 0, vCharge_Timer_Callback);

    /* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */
    
	adc_ready_Semaphore = xSemaphoreCreateBinary();
    
	initialize_analogs();
    
    CyGlobalIntEnable;

    
    alarm_push(ALM_PRIO_INFO,"TASK: Analog started" , ALM_NO_VALUE);

	/* `#END` */

	for (;;) {
		/* `#START TASK_LOOP_CODE` */
		xSemaphoreTake(adc_ready_Semaphore, portMAX_DELAY);
		calculate_rms();

		/* `#END` */
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
