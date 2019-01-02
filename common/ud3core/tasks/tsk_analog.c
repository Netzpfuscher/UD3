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

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "interrupter.h"

xTaskHandle tsk_analog_TaskHandle;
uint8 tsk_analog_initVar = 0u;

SemaphoreHandle_t adc_ready_Semaphore;

xQueueHandle adc_data;


/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */
#include "ZCDtoPWM.h"
#include "charging.h"
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

#define DATA_VBATT 1
#define DATA_VBUS 0
#define DATA_IBUS 3

#define SAMPLES_COUNT 2048

#define BUSV_R_TOP 500000UL
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
	bus_voltage = ((BUSV_R_TOP + BUSV_R_BOT) * raw_adc) / (BUSV_R_BOT * 819 / 1000);
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

void regulate_current(){
    int16_t e = (telemetry.batt_i/10)-configuration.max_inst_i;
    if(e>0){
        int16_t temp = (int16_t)configuration.max_tr_duty - 1;
        if(temp>-1) configuration.max_tr_duty = temp;
        if(tr_running){
            update_interrupter();
        }
        if(telemetry.midi_voices){
            update_midi_duty();
        }
   
    }
   
}

void calculate_rms(void) {
    static uint8_t count=0;
	while (uxQueueMessagesWaiting(adc_data)) {

		xQueueReceive(adc_data, ADC_sample, portMAX_DELAY);

		// read the battery voltage
		telemetry.batt_v = read_bus_mv(ADC_sample[DATA_VBATT]) / 1000;

		// read the bus voltage
		telemetry.bus_v = read_bus_mv(ADC_sample[DATA_VBUS]) / 1000;

		// read the battery current

		telemetry.batt_i = (((uint32_t)rms_filter(&current_idc, ADC_sample[DATA_IBUS]) * params.idc_ma_count) / 100);

		telemetry.avg_power = telemetry.batt_i * telemetry.bus_v / 10;
	}
	control_precharge();
    
    if(count>10){
        count=0;
    if(configuration.max_inst_i>0){
        if(tr_running || telemetry.midi_voices){
            regulate_current();
        }
    }
    }else{
        count++;
    }
   
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

	/* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */

	adc_ready_Semaphore = xSemaphoreCreateBinary();

	initialize_analogs();
    
    isr_primary_current_StartEx(isr_primary);

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
