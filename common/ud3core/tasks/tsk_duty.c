/**
 * @file tsk_duty.c
 * @brief Duty cycle measurement task implementation
 *
 * Monitors interrupter on-time using a hardware counter (OnTimeCounter).
 * Calculates duty cycle percentage with 0.1% resolution every 100ms.
 */

#include "cyapicallbacks.h"
#include <cytypes.h>

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "cli_common.h"
#include "tsk_cli.h"
#include "telemetry.h"
#include "tsk_priority.h"
#include "grid_limit.h"
#include "interrupter.h"
#include "qcw.h"
#include "tasks/tsk_analog.h"
#include "alarmevent.h"

#define TSK_DUTY_PERIOD_MS  100   //ms

/* ------------------------------------------------------------------------ */
/* Grid / fuse current limiter                                             */
/* ------------------------------------------------------------------------ */

#define GRID_SCALE_MAX 1000u

static volatile uint16_t grid_scale = GRID_SCALE_MAX;
static volatile uint8_t grid_tripped = 0;
static float grid_heat = 0.0f;
static int32_t grid_power_filt = 0;
static float grid_i_est = 0.0f;
static uint16_t grid_severe_ms = 0;

static float grid_phase_factor(void){
    return (configuration.grid_phases == 3) ? 1.7320508f : 1.0f;
}

static void grid_trip(void){
    if(grid_tripped) return;
    grid_tripped = 1;
    grid_scale = 0;
    tt.n.i2t_i.value = 100;
    alarm_push(ALM_PRIO_CRITICAL, "GRID: Fuse I2t limit exceeded", (int32_t)(grid_i_est * 10.0f));
    interrupter_kill();
    bus_command = BUS_COMMAND_OFF;
}

void grid_regulator_reset(void){
    grid_scale = GRID_SCALE_MAX;
    grid_tripped = 0;
    grid_heat = 0.0f;
    grid_power_filt = 0;
    grid_i_est = 0.0f;
    grid_severe_ms = 0;
    tt.n.i2t_i.value = 0;
    tt.n.grid_i.value = 0;
}

uint16_t grid_regulator_get_scale(void){
    return grid_tripped ? 0 : grid_scale;
}

uint16_t grid_regulator_effective_max_duty_tenths(void){
    uint32_t scale = grid_tripped ? 0 : grid_scale;
    return (uint16_t)(((uint32_t)configuration.max_tr_duty * scale) / GRID_SCALE_MAX);
}

uint8_t grid_regulator_is_tripped(void){
    return grid_tripped;
}

void grid_regulator_update(void){
    if(configuration.grid_fuse == 0 || configuration.grid_voltage == 0 ||
       configuration.grid_pf == 0 || configuration.grid_eff == 0){
        grid_regulator_reset();
        return;
    }
    if(grid_tripped) return;

    float volts = (float)configuration.grid_voltage;
    float pf = (float)configuration.grid_pf / 100.0f;
    float eff = (float)configuration.grid_eff / 100.0f;
    float phase = grid_phase_factor();
    float denom = volts * pf * eff * phase;
    if(denom <= 0.0f){
        grid_regulator_reset();
        return;
    }

    int32_t power = tt.n.avg_power.value;
    if(power < 0) power = 0;
    grid_power_filt += (power - grid_power_filt) / 8;
    grid_i_est = (float)grid_power_filt / denom;

    float fuse = (float)configuration.grid_fuse;
    float i_limit = fuse * (float)configuration.grid_margin / 100.0f;

    if(grid_i_est > i_limit){
        float over = (grid_i_est - i_limit) / i_limit;
        uint32_t step = 5u + (uint32_t)(over * 25.0f);
        if(step > 100u) step = 100u;
        if(grid_scale > step) grid_scale -= step;
        else grid_scale = 0;
    }else if(grid_i_est < (i_limit * 0.85f)){
        if(grid_scale < GRID_SCALE_MAX) grid_scale += 2u;
        if(grid_scale >= GRID_SCALE_MAX) grid_scale = GRID_SCALE_MAX;
    }

    /* Normalized I2t: heat above the fuse rating, cool below it. */
    float x = grid_i_est / fuse;
    if(x > 1.02f){
        grid_heat += (x * x - 1.0f) * 0.1f;
    }else{
        grid_heat -= 0.1f;
        if(grid_heat < 0.0f) grid_heat = 0.0f;
    }

    float heat_limit = 1.25f * (float)configuration.grid_i2t_time;
    if(heat_limit <= 0.0f) heat_limit = 1.25f;
    if(grid_heat >= heat_limit){
        grid_trip();
        return;
    }

    if(x > 2.0f){
        if(grid_severe_ms < 60000u) grid_severe_ms += 100u;
        if(grid_severe_ms >= 500u){
            grid_trip();
            return;
        }
    }else{
        grid_severe_ms = 0;
    }

    int32_t pct = (int32_t)((grid_heat * 100.0f) / heat_limit);
    if(pct < 0) pct = 0;
    if(pct > 100) pct = 100;
    tt.n.i2t_i.value = pct;
    tt.n.grid_i.value = (int32_t)(grid_i_est * 10.0f);

    /* QCW cannot be duty-scaled; stop the pulse if it is over the limit. */
    if(QCW_enable_Control && grid_i_est > i_limit){
        qcw_stop();
    }
}

void tsk_duty_task(void *pvParameters) {
	OnTimeCounter_Start();
    
    while(1){
        vTaskDelay(pdMS_TO_TICKS(TSK_DUTY_PERIOD_MS));
        
        //read and reset counters
        uint32_t onTime_us = 0xffffff - OnTimeCounter_ReadCounter();
        
        OnTimeCounter_WriteCounter(0xffffff);
        // dutycycle := onTime_us / period
        // the period is 1000 times slower
        // to get the scaling (0,1% resolution) we do dutycycle * 1000
        // together with fixed point division that results in
        //      
        //      dutycycleScaled := (onTime_us * 1000) / ((TSK_DUTY_PERIOD_MS * 1e6) / 1000)
        
        tt.n.dutycycle.value = (onTime_us * 1000) / ((TSK_DUTY_PERIOD_MS * 1e6) / 1000);
        grid_regulator_update();
       
    }
}

void tsk_duty_Start(void) {
	xTaskCreate(tsk_duty_task, "Duty", STACK_DUTY, NULL, PRIO_DUTY, NULL);
}
