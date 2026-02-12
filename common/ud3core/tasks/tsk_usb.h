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

/**
 * @file tsk_usb.h
 * @brief USB CDC communication task
 *
 * Handles USB CDC (Communications Device Class) for serial communication
 * over USB. Provides terminal and data transfer functionality.
 */

#if !defined(tsk_usb_H)
#define tsk_usb_H

#include <cytypes.h>
#include <device.h>
    
/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/**
 * @brief Start the USB task
 */
void tsk_usb_Start(void);

/**
 * @brief Initialize USB hardware
 */
void tsk_usb_Init(void);

/**
 * @brief Enable USB communication
 */
void tsk_usb_Enable(void);

/**
 * @brief Main USB task entry point
 *
 * @param pvParameters Task parameters (unused)
 */
void tsk_usb_Task(void *pvParameters);

extern uint32_t usb_bytes_rx; //!< Total USB bytes received
extern uint32_t usb_bytes_tx; //!< Total USB bytes transmitted

/**
 * @brief USB buffer length
 *
 * Set to maximum packet size of IN and OUT bulk endpoints (64 bytes).
 */
#define tsk_usb_BUFFER_LEN (64u)


/* ------------------------------------------------------------------------ */

#endif
/* [] END OF FILE */
