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
    
  
   
    void process_midi(uint8_t* ptr, uint16_t len);
    void process_min_sid(uint8_t* ptr, uint16_t len);
    

#endif