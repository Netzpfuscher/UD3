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

void tsk_duty_task(void *pvParameters) {
	OnTimeCounter_Start();
    uint32_t lastTickCount = SG_Timer_ReadCounter();
    
    while(1){
        vTaskDelay(pdMS_TO_TICKS(500));
        
        //read and reset counters
        uint32_t onTime = 0xffffff - OnTimeCounter_ReadCounter();
        uint32_t period = lastTickCount - SG_Timer_ReadCounter();
        
        OnTimeCounter_WriteCounter(0xfffffff);
        lastTickCount = SG_Timer_ReadCounter();
        
        // dutycycle := onTime / period
        // the period is 3,125 times slower
        // to get the scaling (0,1% resolution) we do dutycycle * 1000
        // together with fixed point division that results in
        //      
        //      dutycycleScaled := (onTime * 1000) / ((period * 3125) / 1000)
        
        tt.n.dutycycle.value = (onTime * 1000) / ((period * 3125) / 1000);
    }
}

void tsk_duty_Start(void) {
	xTaskCreate(tsk_duty_task, "Duty", configMINIMAL_STACK_SIZE, NULL, PRIO_DUTY, NULL);
}