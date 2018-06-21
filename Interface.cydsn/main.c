/*
 * UD3 Interface
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

#include "project.h"
#define tsk_usb_BUFFER_LEN 64
#include "ssd1306.h"
#include <stdio.h>

#define DISPLAY_ADDRESS 0x3C // 011110+SA0+RW - 0x3C or 0x3D

#define MNU_TIMEOUT 9

volatile uint8_t disp_ref;
volatile uint8_t timeout;
volatile uint8_t timeout_byte;
volatile uint8_t timeout_wd;
volatile uint8_t watchdog = 1;
char sbuf[128];
uint16_t byte_tx = 0;
uint16_t byte_rx = 0;
uint16_t byte_msg = 0;

void USBMIDI_1_callbackLocalMidiEvent(uint8 cable, uint8 *midiMsg) {
	UART_1_PutArray(midiMsg, 3);
	byte_msg += 1;
}

#define SYSTICK_RELOAD (BCLK__BUS_CLK__HZ / 10)

uint8 wd_reset[3] = {0xF0, 0x0F, 0x0F};

void tick(void) {
	static uint8_t cnt = 0;
	static uint8_t dcnt = 0;

	if (timeout > 0) {
		timeout--;
	}
	if (timeout_byte > 0) {
		timeout_byte--;
	}
	if (timeout_wd > 0) {
		timeout_wd--;
	}
}

int main(void) {
	I2COLED_Start();
	PWM_1_Start();
	PWM_2_Start();
	CyGlobalIntEnable; /* Enable global interrupts. */

	display_init(DISPLAY_ADDRESS);

	display_clear();
	display_update();
	gfx_setRotation(2);

	gfx_setTextColor(WHITE);
	gfx_setTextSize(3);
	gfx_setCursor(0, 0);
	gfx_println("UD3");
	gfx_setTextSize(2);
	gfx_println("Interface");
	display_update();
	CyDelay(2000);
	display_clear();
	display_update();
	/* Place your initialization/startup code here (e.g. MyInst_Start()) */
	UART_1_Start();
	if (USBMIDI_1_initVar == 0) {
/* Start USBFS Operation with 3V operation */
#if (CYDEV_VDDIO1_MV < 5000)
		USBMIDI_1_Start(0u, USBMIDI_1_3V_OPERATION);
#else
		USBMIDI_1_Start(0u, USBMIDI_1_5V_OPERATION);
#endif
	}

	uint16 count;
	uint8 buffer[tsk_usb_BUFFER_LEN];

	uint16 idx;

	CySysTickStart();

	CySysTickSetReload(SYSTICK_RELOAD);

	CySysTickSetCallback(0, tick);
	CySysTickClear();

	uint8_t old_btn = 1;
	uint8_t btn = 1;
	uint8_t mnu = 0;
	uint8_t execute_mnu = 0;
	uint16_t byte_s_tx = 0;
	uint16_t byte_s_rx = 0;
	uint16_t byte_s_msg = 0;

	const char *mnu_str[3];
	mnu_str[1] = "Kill";
	mnu_str[2] = "Watchdog";
	mnu_str[0] = "Bootloader";

	for (;;) {

		btn = Status_Reg_1_Read();
		if (btn == 0 && old_btn) {
			mnu += 1;
			if (mnu > 2)
				mnu = 0;
			timeout = MNU_TIMEOUT;
			execute_mnu = 1;
			display_clear();
			gfx_setTextSize(2);
			gfx_setCursor(0, 20);
			gfx_println(mnu_str[mnu]);
			display_update();
		}
		old_btn = btn;

		if (!timeout && execute_mnu) {
			execute_mnu = 0;

			switch (mnu) {
			case 1:
				UART_1_PutString("\r\nkill\r\n");
				break;
			case 2:
				if (watchdog == 0) {
					watchdog = 1;
				} else {
					watchdog = 0;
				}
				break;
			case 0:
				Bootloadable_1_Load();
				break;
			}
			mnu = 0;
		}
		if (!timeout_byte) {
			byte_s_rx = byte_rx;
			byte_s_tx = byte_tx;
			byte_s_msg = byte_msg;
			byte_rx = 0;
			byte_tx = 0;
			byte_msg = 0;
			timeout_byte = 9;
			disp_ref = 1;
		}
		if (!timeout_wd && watchdog) {
			timeout_wd = 1;
			UART_1_PutArray(wd_reset, 3);
		}

		//        if(!Status_Reg_1_Read()){
		//            display_clear();
		//            gfx_setTextSize(2);
		//            gfx_setCursor(0,0);
		//            gfx_println("Bootloader");
		//            display_update();
		//            CyDelay(2000);
		//            Bootloadable_1_Load();
		//        }

		if (disp_ref && !timeout) {
			display_clear();
			gfx_setTextSize(1);
			disp_ref = 0;
			gfx_setCursor(0, 0);
			if (watchdog) {
				gfx_println("Watchdog:         ON");
			} else {
				gfx_println("Watchdog:        OFF");
			}

			sprintf(sbuf, "Bytes TX: %8i/s", byte_s_tx);
			gfx_println(sbuf);
			sprintf(sbuf, "Bytes RX: %8i/s", byte_s_rx);
			gfx_println(sbuf);
			sprintf(sbuf, "MIDI MSG: %8i/s", byte_s_msg);
			gfx_println(sbuf);

			display_update();
		}

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
					UART_1_PutArray(buffer, count);
					byte_tx += count;

					//					for(idx=0;idx<count;++idx) {
					//						xQueueSend( qUSB_rx, (void*)&buffer[idx],0);
					//					}
				}
			}
			/*
    		 * Send a block of data ack through the USB port to the PC,
    		 * by checkig to see if there is data to send, then sending
    		 * up to the BUFFER_LEN of data (64 bytes)
    		 */
			count = UART_1_GetRxBufferSize();
			count = (count > tsk_usb_BUFFER_LEN) ? tsk_usb_BUFFER_LEN : count;

			/* When component is ready to send more data to the PC */
			if ((USBMIDI_1_CDCIsReady() != 0u) && (count > 0)) {
				/*
    			 * Read the data from the transmit queue and buffer it
    			 * locally so that the data can be utilized.
    			 */

				for (idx = 0; idx < count; ++idx) {
					buffer[idx] = UART_1_GetChar();
					byte_rx += 1;
					//xQueueReceive( qUSB_tx,&buffer[idx],0);
				}
				/* Send data back to host */
				USBMIDI_1_PutData(buffer, count);

				/* If the last sent packet is exactly maximum packet size, 
            	 *  it shall be followed by a zero-length packet to assure the
             	 *  end of segment is properly identified by the terminal.
             	 */
				if (count == tsk_usb_BUFFER_LEN) {
					/* Wait till component is ready to send more data to the PC */
					while (USBMIDI_1_CDCIsReady() == 0u) {
						//vTaskDelay( 10 / portTICK_RATE_MS );
					}
					USBMIDI_1_PutData(NULL, 0u); /* Send zero-length packet to PC */
				}
			}
		}

		/* Place your application code here. */
	}
}

/* [] END OF FILE */
