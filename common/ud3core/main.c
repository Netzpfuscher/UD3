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

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "DMA.h"
#include "ZCDtoPWM.h"
#include "interrupter.h"
#include "telemetry.h"
#include <device.h>

#include "cli_common.h"
#include "alarmevent.h"
#include "tasks/tsk_analog.h"
#include "tasks/tsk_cli.h"
#include "tasks/tsk_fault.h"
#include "tasks/tsk_midi.h"
#include "tasks/tsk_thermistor.h"
#include "tasks/tsk_uart.h"
#include "tasks/tsk_usb.h"
#include "tasks/tsk_eth.h"
#include "tasks/tsk_min.h"
#include "tasks/tsk_display.h"

/*
 * Installs the RTOS interrupt handlers and starts the peripherals.
 */
static void prvFreeRTOSSetup( void );


int main() {
    
    prvFreeRTOSSetup();
    alarm_init();
	system_fault_Control = 0; //this should suppress any start-up sparking until the system is ready
	init_config();
    EEPROM_1_Start();
    Mantmr_Start();
    
    null_port.type = PORT_TYPE_NULL;
    null_port.tx = NULL;
    null_port.rx = NULL;
    
    eeprom_load(&null_port);
   
    
	initialize_DMA();		  //sets up all DMA channels
	initialize_interrupter(); //initializes hardware related to the interrupter
	initialize_ZCD_to_PWM();  //initializes hardware related to ZCD to PWM
    
	initialize_charging();

	CyGlobalIntEnable; //enables interrupts

	//calls that must always happen after updating the configuration/settings
	configure_ZCD_to_PWM();

	rx_blink_Control = 1;


	//Starting Tasks
    if(configuration.minprot || configuration.eth_hw==ETH_HW_ESP32){
        tsk_min_Start();        //Handles UART-Hardware and queues with MIN-Protocol
    }else{
	    tsk_uart_Start();       //Handles UART-Hardware and queues
    }
    
	tsk_usb_Start();        //Handles USB-Hardware and queues
    if(configuration.eth_hw==ETH_HW_W5500){
        tsk_eth_Start();        //Handles Ethernet-Hardware and queues
    }
    
    tsk_cli_Start();		//Commandline interface
    
	tsk_midi_Start();       //MIDI synth
	
	tsk_analog_Start();		//Reads bus voltage and currents
	tsk_thermistor_Start(); //Reads thermistors
	tsk_fault_Start();		//Handles fault conditions
    
    if(configuration.enable_display){
        tsk_display_Start();
    }
    
    
    alarm_push(ALM_PRIO_INFO, warn_general_startup, ALM_NO_VALUE);
	vTaskStartScheduler();
    
	for (;;) {
		//should never reach this point
	}
}

void prvFreeRTOSSetup( void )
{
/* Port layer functions that need to be copied into the vector table. */
extern void xPortPendSVHandler( void );
extern void xPortSysTickHandler( void );
extern void vPortSVCHandler( void );
extern cyisraddress CyRamVectors[];

	/* Install the OS Interrupt Handlers. */
	CyRamVectors[ 11 ] = ( cyisraddress ) vPortSVCHandler;
	CyRamVectors[ 14 ] = ( cyisraddress ) xPortPendSVHandler;
	CyRamVectors[ 15 ] = ( cyisraddress ) xPortSysTickHandler;

	/* Start-up the peripherals. */

}


void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	/* The stack space has been execeeded for a task, considering allocating more. */
	taskDISABLE_INTERRUPTS();
    for(;;);
}/*---------------------------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* The heap space has been execeeded. */
	taskDISABLE_INTERRUPTS();
    for(;;);
}
/*---------------------------------------------------------------------------*/

void vConfigureTimerForRunTimeStats(void){
    
}

/* [] END OF FILE */
