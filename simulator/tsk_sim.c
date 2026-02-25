#include "tsk_sim.h"


#include "FreeRTOS.h"
#include "qcw.h"
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
#include <math.h>

#define MAX_BUS_CHARGE 4000
#define MAINS_FREQ 50.0
#define BUS_RC_TIME_CONSTANT 0.1  // RC time constant in seconds (adjust for desired charge speed)

static int bus_charge = 0;
static int bus_i = 0;
static float sine_phase = 0.0;
static float bus_voltage = 0.0;  // Current bus voltage (smoothed)

static void populate_buffer(adc_sample_t* ptr){
	float bus_target = 0.0;
	
	if (tt.n.bus_status.value == BUS_READY) {
		bus_target = MAX_BUS_CHARGE;
	} else if (tt.n.bus_status.value == BUS_CHARGING) {
		bus_target = MAX_BUS_CHARGE;
	} else {
		bus_target = 0;
	}
	
	// RC charging simulation: V(t) = V_target * (1 - e^(-t/RC))
	// Using discrete exponential smoothing: V_new = V_old + (V_target - V_old) * alpha
	// where alpha = dt / (RC + dt) approximates RC charging
	float dt = 1.0 / ADC_SAMPLE_CLK;
	float alpha = dt / (BUS_RC_TIME_CONSTANT + dt);
	
	if(tt.n.midi_voices.value && system_fault_Control){
		float temp = (((55000 - param.pwd)/1000) * param.pw)/10;
		temp = temp * ((float)tt.n.bus_v.value / 493.0);
		bus_i = temp;
		temp = (((55000 - param.pwd)/1000) * param.pw)/10;
		OnTimeCounter_WriteCounter(0xffffff - temp);
	}else{
		bus_i = 0;
	}
	
	for(int i = 0; i<ADC_BUFFER_CNT;i++){
		// Update bus voltage with RC charging behavior
		float rectified_sine = fabs(sin(sine_phase));
		float instantaneous_target = bus_target * rectified_sine;
		bus_voltage += (instantaneous_target - bus_voltage) * alpha;
		
		sine_phase += 2.0 * M_PI * MAINS_FREQ / ADC_SAMPLE_CLK;
		if (sine_phase >= 2.0 * M_PI) {
			sine_phase -= 2.0 * M_PI;
		}
		
		ptr[i].v_bus = (int)bus_voltage;
		ptr[i].i_bus = bus_i;
	}
	xSemaphoreGive(adc_ready_Semaphore);
}

static bool read_buffer_0 = false;

adc_sample_t* tsk_analog_get_readable_buffer() {
    return read_buffer_0 ? ADC_sample_buf_0 : ADC_sample_buf_1;
}

void tsk_sim(void *pvParameters) {
	while(1){
		if(adc_ready_Semaphore){
			if(!read_buffer_0){
				populate_buffer(ADC_sample_buf_0);
			}else{
				populate_buffer(ADC_sample_buf_1);		
			}
            read_buffer_0 = !read_buffer_0;
		}
		
		vTaskDelay(10);
	}
}

void sim_isr_synth() {
    uint32_t r = SG_Timer_ReadCounter();
	for(int i=0;i<20;i++){
		clock_tick();
	}
    if(QCW_enable_Control){
        qcw_handle();
        return;
    }
    // TODO do we need to call anything here now? Probably not?
}

void tsk_sim_isr(void *pvParameters) {
	while(1){
	
		sim_isr_synth();
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



