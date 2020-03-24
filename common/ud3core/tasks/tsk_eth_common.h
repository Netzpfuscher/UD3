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
    
    #define NUM_MIN_CON 3

    
    #define STREAMBUFFER_RX_SIZE    256     //bytes
    #define STREAMBUFFER_TX_SIZE    1024    //bytes
    
   
    void process_midi(uint8_t* ptr, uint16_t len);
    void process_sid(uint8_t* ptr, uint16_t len);
    

#endif