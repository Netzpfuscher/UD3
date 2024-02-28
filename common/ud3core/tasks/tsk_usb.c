/* ======================================================================== */
/*
 * Copyright (c) 2015, E2ForLife.com
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the E2ForLife.com nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL E2FORLIFE.COM BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* ======================================================================== */
#include <CyLib.h>
#include <cytypes.h>
#include <stdio.h>

#include "USBMIDI_1.h"
#include "USBMIDI_1_cdc.h"
#include "tsk_priority.h"
#include "tsk_usb.h"
#include "tsk_fault.h"
#include "alarmevent.h"


/* ======================================================================== */

#include "cli_common.h"
#include "interrupter.h"
#include "tsk_midi.h"
#include "tsk_cli.h"
uint8 tsk_usb_initVar;
xTaskHandle tsk_usb_TaskHandle;

uint32_t usb_bytes_rx=0;
uint32_t usb_bytes_tx=0;


/* ======================================================================== */
void tsk_usb_Start(void) {
	if (tsk_usb_initVar != 1) {
		tsk_usb_Init();
	}

	/* Check for enumeration after initialization */
	tsk_usb_Enable();
}
/* ------------------------------------------------------------------------ */
void tsk_usb_Init(void) {
	/*  Initialize the USB COM port */

	if (USBMIDI_1_initVar == 0) {
/* Start USBFS Operation with 3V operation */
#if (CYDEV_VDDIO1_MV < 5000)
		USBMIDI_1_Start(0u, USBMIDI_1_3V_OPERATION);
#else
		USBMIDI_1_Start(0u, USBMIDI_1_5V_OPERATION);
#endif
	}

	xTaskCreate(tsk_usb_Task, "USB-Svc", STACK_USB, NULL, PRIO_USB, &tsk_usb_TaskHandle);

	tsk_usb_initVar = 1;
}
/* ------------------------------------------------------------------------ */
void tsk_usb_Enable(void) {
	if (tsk_usb_initVar != 0) {
		/*
		 * COMIO was initialized, and now is bing enabled for use.
		 * Enter user extension enables within the merge region below.
		 */
		/* `#START COMIO_ENABLE` */

		/* `#END` */
	}
}
/* ======================================================================== */
void tsk_usb_Task(void *pvParameters) {
	uint16 count;
	uint8 buffer[tsk_usb_BUFFER_LEN];
    alarm_push(ALM_PRIO_INFO, "TASK: USB started", ALM_NO_VALUE);
	for (;;) {
		/* Handle enumeration of USB port */
		if (USBMIDI_1_IsConfigurationChanged() != 0u) /* Host could send double SET_INTERFACE request */
		{
			if (USBMIDI_1_GetConfiguration() != 0u) /* Init IN endpoints when device configured */
			{
				/* Enumeration is done, enable OUT endpoint for receive data from Host */
				USBMIDI_1_CDC_Init();
				USBMIDI_1_MIDI_Init();
			}
		}
		/*
		 *
		 */
		if (USBMIDI_1_GetConfiguration() != 0u) {
/*
			 * Process received data from the USB, and store it in to the
			 * receiver message Q.
			 */

#if (!USBMIDI_1_EP_MANAGEMENT_DMA_AUTO)
			USBMIDI_1_MIDI_OUT_Service();
#endif

			if (USBMIDI_1_DataIsReady() != 0u) {
				count = USBMIDI_1_GetAll(buffer);
				if (count != 0u) {
					/* insert data in to Receive FIFO */
                    LED_com_Write(LED_ON);
                    usb_bytes_rx+=count;
                    xStreamBufferSend(usb_port.rx, &buffer,count, 0);
				}
			}
			/*
			 * Send a block of data ack through the USB port to the PC,
			 * by checkig to see if there is data to send, then sending
			 * up to the BUFFER_LEN of data (64 bytes)
			 */
            
            count = xStreamBufferBytesAvailable(usb_port.tx);

			/* When component is ready to send more data to the PC */
			if ((USBMIDI_1_CDCIsReady() != 0u) && (count > 0)) {
				/*
				 * Read the data from the transmit queue and buffer it
				 * locally so that the data can be utilized.
				 */
                count = xStreamBufferReceive(usb_port.tx,&buffer,tsk_usb_BUFFER_LEN,0);
				/* Send data back to host */
                LED_com_Write(LED_ON);
                usb_bytes_tx+=count;
				USBMIDI_1_PutData(buffer, count);

				/* If the last sent packet is exactly maximum packet size, 
            	 *  it shall be followed by a zero-length packet to assure the
             	 *  end of segment is properly identified by the terminal.
             	 */
				if (count == tsk_usb_BUFFER_LEN) {
					/* Wait till component is ready to send more data to the PC */
					while (USBMIDI_1_CDCIsReady() == 0u) {
						vTaskDelay(1);
					}
					USBMIDI_1_PutData(NULL, 0u); /* Send zero-length packet to PC */
				}
			}
		}
        if(xStreamBufferBytesAvailable(usb_port.tx)==0 || xStreamBufferBytesAvailable(usb_port.rx)==0 || USBMIDI_1_CDCIsReady()==0){
		    vTaskDelay(2 / portTICK_PERIOD_MS);
        }
	}
}
/* ======================================================================== */
/* [] END OF FILE */
