/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

/**
 * @file tsk_eth_common.h
 * @brief Common Ethernet/MIN protocol functions
 *
 * Shared functions for processing MIDI and SID data received over
 * Ethernet or MIN protocol connections.
 */

#ifndef TSK_ETH_COMMON_H
#define TSK_ETH_COMMON_H

    #include <stdint.h>
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
    #include "cli_basic.h"
    #include "stream_buffer.h" 
    
  
   
    /**
     * @brief Process MIDI data from network/MIN stream
     * @param ptr Pointer to MIDI data buffer
     * @param len Length of data
     */
    void process_midi(uint8_t* ptr, uint16_t len);
    
    /**
     * @brief Process SID data from MIN protocol stream
     * @param ptr Pointer to SID data buffer
     * @param len Length of data
     */
    void process_min_sid(uint8_t* ptr, uint16_t len);
    

#endif