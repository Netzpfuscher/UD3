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
#include "alarmevent.h"
#include "tsk_fault.h"
#include "tsk_analog.h"

#define HYPERVISOR_CHECK_TIME   100     //ms
#define FAULT_RESET_TIME        2000    //ms
#define BUS_AUTOSTART_TIME      2000    //ms

void tsk_hypervisor_task(void *pvParameters) {
    
    alarm_push(ALM_PRIO_INFO,"TASK: Hypervisor started" , ALM_NO_VALUE);
    
    int32_t fault_reset_time = FAULT_RESET_TIME;
    int32_t bus_autostart_time = BUS_AUTOSTART_TIME;
    
    interrupter_update_ext();
    

    WD_enable(pdFALSE);
    while(1){

        if(tsk_fault_is_fault()){
            fault_reset_time -= HYPERVISOR_CHECK_TIME;
            if(fault_reset_time<0) fault_reset_time = 0;
        }else{
            fault_reset_time = FAULT_RESET_TIME;
        }
        
        if(fault_reset_time == 0){
            reset_fault();
            fault_reset_time = FAULT_RESET_TIME;
        }
        
        if(bus_command == BUS_COMMAND_FAULT){
            bus_command = BUS_COMMAND_OFF;
            bus_autostart_time = BUS_AUTOSTART_TIME;
        }
        
        if(bus_command == BUS_COMMAND_OFF && tsk_fault_is_fault() == 0){
            bus_autostart_time -= HYPERVISOR_CHECK_TIME;
            if(bus_autostart_time < 0) bus_autostart_time = 0;
        }
        
        if(bus_autostart_time == 0){
            bus_command = BUS_COMMAND_ON;
            bus_autostart_time = BUS_AUTOSTART_TIME;
        }
               
        
        vTaskDelay(pdMS_TO_TICKS(HYPERVISOR_CHECK_TIME));
    }
}

void tsk_hypervisor_Start(void) {
    if(configuration.autostart == pdTRUE){
	    xTaskCreate(tsk_hypervisor_task, "Hyper", configMINIMAL_STACK_SIZE, NULL, PRIO_FAULT, NULL);
    }
}
