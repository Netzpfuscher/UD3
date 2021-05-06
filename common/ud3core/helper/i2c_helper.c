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

#include "i2c_helper.h"
#include <device.h>
#include "FreeRTOS.h"
#include "task.h"

void I2C_Write(uint8_t address, uint8_t registerAddress, uint8_t data){
    I2C_MasterSendStart(address,I2C_WRITE_XFER_MODE);
    I2C_MasterWriteByte(registerAddress);
    I2C_MasterWriteByte(data);
    I2C_MasterSendStop();
    
}

uint8_t I2C_Read(uint8_t address, uint8_t registerAddress){
    uint8_t ret;
    I2C_MasterSendStart(address,I2C_WRITE_XFER_MODE);
    I2C_MasterWriteByte(registerAddress);
    I2C_MasterSendRestart(address,I2C_READ_XFER_MODE);
    ret = I2C_MasterReadByte(I2C_NAK_DATA);
    I2C_MasterSendStop();
    return ret; 
}

void I2C_Write_Blk(uint8_t address, uint8_t * buffer, uint8_t cnt){
    I2C_MasterClearStatus();
    I2C_MasterWriteBuf(address,buffer,cnt,I2C_MODE_COMPLETE_XFER);
    while(!(I2C_MasterStatus() & I2C_MSTAT_WR_CMPLT)){
        vTaskDelay(1);   
    }
}

void I2C_Write_nBlk(uint8_t address, uint8_t * buffer, uint8_t cnt){
    I2C_MasterClearStatus();
    I2C_MasterWriteBuf(address,buffer,cnt,I2C_MODE_COMPLETE_XFER);
}