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

/* [] END OF FILE */

#ifndef TSK_ETH_COMMON_H
#define TSK_ETH_COMMON_H

    #include <stdint.h>
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
    #include "cli_basic.h"
    #include "stream_buffer.h" 
    
    #define NUM_ETH_CON 3

    #define ETH_HW_DISABLED 0
    #define ETH_HW_W5500    1
    #define ETH_HW_ESP32    2
    
    StreamBufferHandle_t xETH_rx[NUM_ETH_CON];
    StreamBufferHandle_t xETH_tx[NUM_ETH_CON];
    
    void process_midi(uint8_t* ptr, uint16_t len);
    void process_sid(uint8_t* ptr, uint16_t len);
    

#endif