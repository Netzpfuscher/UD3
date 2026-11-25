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
#include "ZCDtoPWM.h"
#include "sim_hw.h"
#include <math.h>

#define MAX_BUS_CHARGE 4000
#define MAINS_FREQ 50.0
/*
 * The ADC buffer contains 25 samples at 32 kHz, i.e. 0.78 ms of simulated
 * time.  Delaying 10 ms per buffer made the bus voltage, bus current and
 * therefore the grid estimate run ~12.8x slower than real time.  Use one
 * tick per buffer so the model runs close to real time.
 */
#define SIM_ADC_TASK_DELAY_MS 1
#define BUS_RC_TIME_CONSTANT 0.1  // RC time constant in seconds (adjust for desired charge speed)

static int bus_charge = 0;
static int bus_i = 0;
static float sine_phase = 0.0;
static float bus_voltage = 0.0;  // Current bus voltage (smoothed)

/*
 * Simplified primary-current model.
 *
 * A DRSSTC primary current ramps roughly as V_bus * t_on / L_pri during a
 * burst. We use that as the per-pulse peak, clamp it to the DAC current
 * limit configured by the firmware, and hold the peak with a fast attack
 * and slower decay so the normal telemetry path sees a realistic value.
 */
#define SIM_PRIMARY_L_HENRY   70e-6f
#define SIM_PRIMARY_ATTACK    0.45f
#define SIM_PRIMARY_DECAY     0.12f

static float sim_primary_current_amps = 0.0f;

static float sim_current_limit_amps(uint8_t dac){
	if(configuration.ct1_ratio == 0 || configuration.ct1_burden == 0) return 0.0f;
	// Inverse of the DAC calculation in configure_CT1().
	return ((float)dac * DAC_VOLTS_PER_STEP * 10.0f * (float)configuration.ct1_ratio) / (float)configuration.ct1_burden;
}

static void update_primary_current(void){
	//A one-shot pulse (autotune/oneshot) owns the detector until it is read.
	if(sim_hw_primary_hold_active()) return;
	float target = 0.0f;
	bool active = false;
	float on_time_us = (float)SigGen_getPeakPulseWidth();

	if(QCW_enable_Control && ramp.index < QCW_RAMP_SAMPLES){
		float limit = sim_current_limit_amps(ct1_dac_val[2]);
		target = limit * ((float)ramp.data[ramp.index] / 255.0f);
		active = true;
	}else if(param.synth != SYNTH_OFF && tt.n.midi_voices.value > 0 &&
	         on_time_us > 0.0f && tt.n.bus_v.value > 0){
		float peak = (float)tt.n.bus_v.value * on_time_us * 1e-6f / SIM_PRIMARY_L_HENRY;
		float limit = sim_current_limit_amps(ct1_dac_val[0]);
		if(limit <= 0.0f) limit = (float)configuration.max_tr_current;
		target = (peak < limit) ? peak : limit;
		active = true;
	}

	// A latched fault or a killed output must decay to zero.
	if(system_fault_Control == 0) active = false;
	if(!active) target = 0.0f;

	float alpha = (target > sim_primary_current_amps) ? SIM_PRIMARY_ATTACK : SIM_PRIMARY_DECAY;
	sim_primary_current_amps += (target - sim_primary_current_amps) * alpha;
	if(sim_primary_current_amps < 0.5f) sim_primary_current_amps = 0.0f;

	// Convert amperes back to the 12-bit ADC_peak counts using the same
	// ct1_ratio/ct1_burden calibration the firmware applies.
	float counts = 0.0f;
	if(configuration.ct1_ratio > 0){
		counts = sim_primary_current_amps * (float)configuration.ct1_burden * 100.0f / (float)configuration.ct1_ratio;
	}
	if(counts > 4095.0f) counts = 4095.0f;
	sim_hw_set_primary_current_counts((uint16_t)(counts + 0.5f));
}

static void populate_buffer(adc_sample_t* ptr){
	update_primary_current();
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
		/* Follow the limited pulse width, not the raw parameter: the duty
		 * compressor and grid limiter act on limitedPulseWidth_us. */
		uint32_t on_time_us = SigGen_getPeakPulseWidth();
		float on_time_counts = (((55000 - param.pwd)/1000) * (float)on_time_us)/10;
		bus_i = on_time_counts * ((float)tt.n.bus_v.value / 493.0);
		OnTimeCounter_WriteCounter(0xffffff - (uint32_t)on_time_counts);
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
		
		vTaskDelay(pdMS_TO_TICKS(SIM_ADC_TASK_DELAY_MS));
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



