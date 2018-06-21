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
#if !defined(tsk_usb_H)
#define tsk_usb_H

#include <cytypes.h>
#include <device.h>
    
/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define tsk_usb_YES (1)
#define tsk_usb_NO (0)

#define tsk_usb_BLOCKING_GETS (tsk_usb_NO)

void tsk_usb_Start(void);
void tsk_usb_Init(void);
void tsk_usb_Enable(void);

void tsk_usb_Task(void *pvParameters);

xQueueHandle qUSB_tx;
xQueueHandle qUSB_rx;

/*
 * Driver API interfaces.
 */
cystatus tsk_usb_Read(uint8 *value, uint32 length);
cystatus tsk_usb_ReadByte(uint8 *value);
uint32 tsk_usb_DataWaiting(void);
void tsk_usb_UnRead(uint8 *value, uint32 len);

cystatus tsk_usb_WriteByte(uint8 ch);
cystatus tsk_usb_Write(uint8 *str, uint32 len);

/* The size of the buffer is equal to maximum packet size of the 
*  IN and OUT bulk endpoints. 
*/
#define tsk_usb_BUFFER_LEN (64u)
#define tsk_usb_RX_SIZE ()
#define tsk_usb_TX_SIZE ()

/* ------------------------------------------------------------------------ */

#endif
/* [] END OF FILE */
