#include "tsk_sim.h"


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "tasks/tsk_analog.h"
#include "semphr.h"
#include "telemetry.h"
#include "interrupter.h"
#include "cli_common.h"
#include "cli_basic.h"
#include "clock.h"
#include "SignalGenerator.h"

static int bus_charge = 0;
static int bus_i = 0;

static void populate_buffer(adc_sample_t* ptr){
	if(tt.n.bus_status.value == BUS_CHARGING){
		if(bus_charge<4000) bus_charge+=((4000-bus_charge)/100);
	}else if(tt.n.bus_status.value == BUS_OFF){
		if(bus_charge>0) bus_charge-=((bus_charge)/100);
		
	}
	
	if(interrupter.mode!=INTR_MODE_OFF){
		float temp = (((55000 - param.pwd)/1000) * param.pwp)/10;
		bus_i = temp;
	}else{
		bus_i = 0;
	}
	
	for(int i = 0; i<ADC_BUFFER_CNT;i++){
		if(bus_charge<0) bus_charge =0;
		ptr[i].v_bus = bus_charge;
		ptr[i].i_bus = bus_i;
	}
	xSemaphoreGive(adc_ready_Semaphore);
}



void tsk_sim(void *pvParameters) {
	while(1){
		if(adc_ready_Semaphore){
			if(ADC_active_sample_buf != ADC_sample_buf_0){
				populate_buffer(ADC_sample_buf_0);
				ADC_active_sample_buf = ADC_sample_buf_0;
			}else{
				populate_buffer(ADC_sample_buf_1);		
				ADC_active_sample_buf = ADC_sample_buf_1;
			}
		}
		
		vTaskDelay(10);
	}
}

void isr_synth() {
    uint32_t r = SG_Timer_ReadCounter();
	for(int i=0;i<20;i++){
		clock_tick();
	}
    if(qcw_reg){
        qcw_handle();
        return;
    }
    switch(param.synth){
        case SYNTH_MIDI:
            synthcode_MIDI();
            break;
        case SYNTH_SID:
            synthcode_SID(r);
            break;
        case SYNTH_MIDI_QCW:
            //synthcode_QMIDI(r);
            break;
        case SYNTH_SID_QCW:
            synthcode_QSID(r);
            break;
        default:
            break;
    }
}

void tsk_sim_isr(void *pvParameters) {
	while(1){
	
		isr_synth();
		vTaskDelay(1);
	}
}


xTaskHandle tsk_sim_TaskHandle;
xTaskHandle tsk_sim_isr_TaskHandle;



static int tsk_sim_started = 0;


void tsk_sim_Start(){
	
	
	if(!tsk_sim_started){
		

		xTaskCreate(tsk_sim, "SIM", 1024, NULL, 3, &tsk_sim_TaskHandle);
		xTaskCreate(tsk_sim_isr, "SIM_ISR", 1024, NULL, 3, &tsk_sim_isr_TaskHandle);

	}
	
}



