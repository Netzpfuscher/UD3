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

#define TSK_DUTY_PERIOD_MS  100   //ms

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
       
    }
}

void tsk_duty_Start(void) {
	xTaskCreate(tsk_duty_task, "Duty", configMINIMAL_STACK_SIZE, NULL, PRIO_DUTY, NULL);
}
