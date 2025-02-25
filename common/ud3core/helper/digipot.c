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

#include "digipot.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cli_common.h"

#define R_DCDC_TOP 82000.0
#define R_DCDC_BOTTOM_V3 2700.0
#define R_DCDC_BOTTOM 3300.0
#define R_DIGIPOT 5000.0

void digipot_write(uint8_t value){
 
    uint8_t bit_to_write=0;
    uint8_t bit_shift = 7;
    
    digipot_ncs_Write(0); //select digipot
    
    CyDelayUs(1);
    
    for(uint32_t i=0;i<8;i++){
        
        digipot_clk_Write(0);
        
        bit_to_write = (value >> bit_shift) & 0x01;
        
        digipot_data_Write(bit_to_write);
        
        CyDelayUs(1); 
        
        digipot_clk_Write(1);
        
        bit_shift--;
        
        CyDelayUs(1); 
    }
    
    digipot_clk_Write(0);
    digipot_data_Write(0);
    
    digipot_ncs_Write(1); //deselect digipot
    
}

void digipot_set_voltage(float voltage){
    
    float r_bottom = R_DCDC_BOTTOM;
    
    if(configuration.hw_rev < 2){
        r_bottom = R_DCDC_BOTTOM_V3;
    }
    
    float r_pot = (voltage * r_bottom - 0.8 * r_bottom - 0.8 * R_DCDC_TOP) / -voltage + 0.8;
    
    float data = 255.0 - 255.0 / R_DIGIPOT * r_pot;
    
    if(data > 255.0) data = 255.0;
    if(data < 0.0) data = 0.0;
    
    digipot_write(data);
    
}