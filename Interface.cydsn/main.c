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
#include "min.h"
#include "min_id.h"

#define DISP 0

#define DISPLAY_ADDRESS 0x3C // 011110+SA0+RW - 0x3C or 0x3D

#define MNU_TIMEOUT 9

#define CONNECTION_TIMEOUT 150      //ms
#define CONNECTION_HEARTBEAT 50     //ms

volatile uint8_t disp_ref;
volatile uint8_t timeout;
volatile uint8_t timeout_byte;
volatile uint8_t timeout_wd;
volatile uint8_t watchdog = 1;
char sbuf[128];
uint16_t byte_tx = 0;
uint16_t byte_rx = 0;
uint16_t byte_msg = 0;

void USB_Send_Data(uint8_t* data, uint16_t len);
void USB_send_string(char* str);
struct min_context min_ctx;


void USBMIDI_1_callbackLocalMidiEvent(uint8 cable, uint8 *midiMsg) {
    min_send_frame(&min_ctx, MIN_ID_MIDI, midiMsg, 3);
	byte_msg += 1;
}

#define SYSTICK_RELOAD (BCLK__BUS_CLK__HZ / 10)

uint8 wd_reset[3] = {0xF0, 0x0F, 0x0F};

void tick(void) {

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

uint16_t min_tx_space(uint8_t port){
    return (UART_1_TX_BUFFER_SIZE-UART_1_GetTxBufferSize());
}

void min_tx_byte(uint8_t port, uint8_t byte)
{
    UART_1_PutChar(byte);
}
/*
void min_debug_print(const char *msg, ...){

    USB_send_string(msg);

}
*/
uint32_t min_time_ms(void)
{
  uint32_t ticks = 4294967296 - Frame_Tmr_ReadCounter();
  return ticks;
}

void min_tx_start(uint8_t port){
}
void min_tx_finished(uint8_t port){
}
uint32_t reset;

void min_application_handler(uint8_t min_id, uint8_t *min_payload, uint8_t len_payload, uint8_t port)
{
    switch(min_id){
        case MIN_ID_WD:

        break;
        case MIN_ID_MIDI:
            //USBMIDI_1_callbackLocalMidiEvent(0, min_payload);
        break;
        case MIN_ID_TERM:
            USB_Send_Data(min_payload, len_payload);
        break;
        case MIN_ID_RESET:
            reset  = min_time_ms();  
        break;

    }
}

void USB_send_string(char* str){

    USB_Send_Data((uint8_t*)str,strlen(str));
}

void USB_Send_Data(uint8_t* data, uint16_t len){

    /* When component is ready to send more data to the PC */
            uint8_t wait_cycles=100;
            while (USBMIDI_1_CDCIsReady() == 0u){
                CyDelayUs(10);
                if (!wait_cycles) return;
                wait_cycles--;
            };
            
			if (len > 0) {
	            
                while(len > tsk_usb_BUFFER_LEN){
                    while (USBMIDI_1_CDCIsReady() == 0u) {
					}
                    if(len>tsk_usb_BUFFER_LEN){
                        USBMIDI_1_PutData(data, tsk_usb_BUFFER_LEN);
                        data += tsk_usb_BUFFER_LEN;
                        len -= tsk_usb_BUFFER_LEN;
                    }
                }
                if(len){
                    while (USBMIDI_1_CDCIsReady() == 0u) {
                    } 
                    USBMIDI_1_PutData(data, len);
                }

				/* If the last sent packet is exactly maximum packet size, 
            	 *  it shall be followed by a zero-length packet to assure the
             	 *  end of segment is properly identified by the terminal.
             	 */
				if (len == tsk_usb_BUFFER_LEN) {
					/* Wait till component is ready to send more data to the PC */
					while (USBMIDI_1_CDCIsReady() == 0u) {
						//vTaskDelay( 10 / portTICK_RATE_MS );
					}
					USBMIDI_1_PutData(NULL, 0u); /* Send zero-length packet to PC */
				}
			}
   
}

int main(void) {
	I2COLED_Start();
	PWM_1_Start();
	PWM_2_Start();
	CyGlobalIntEnable; /* Enable global interrupts. */

    #if DISP
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
    #endif
    
    min_init_context(&min_ctx, 0);
    
    
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

    Frame_Tmr_Start();
    
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
    uint32_t last=0;

	const char *mnu_str[3];
	mnu_str[1] = "Kill";
	mnu_str[2] = "Watchdog";
	mnu_str[0] = "Bootloader";
    
    min_transport_reset(&min_ctx,1);
    min_transport_reset(&min_ctx,1);
    min_transport_reset(&min_ctx,1);
	for (;;) {

		btn = Status_Reg_1_Read();
        
        #if DISP
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
			min_queue_frame(&min_ctx, MIN_ID_WD, &timeout_wd , 1);
		}

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
        #endif

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
                    min_send_frame(&min_ctx, MIN_ID_TERM, buffer, count);
					byte_tx += count;
				}
			}

		}
        if(UART_1_GetRxBufferSize()){
            buffer[0] = UART_1_GetByte();
            min_poll(&min_ctx,buffer,1);
        }else{
            min_poll(&min_ctx,buffer,0);
        }
        
        if((min_time_ms()-reset)>CONNECTION_TIMEOUT){
            min_transport_reset(&min_ctx,true);
        }
        
        if((min_time_ms()-last) > CONNECTION_HEARTBEAT){
            last = min_time_ms();
            min_send_frame(&min_ctx,MIN_ID_RESET,buffer,1);
        }
      
		/* Place your application code here. */
	}
}

/* [] END OF FILE */
