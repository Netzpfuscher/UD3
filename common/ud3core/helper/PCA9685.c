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

#include "PCA9685.h"
#include "i2c_helper.h"
#include "FreeRTOS.h"

uint32_t i2c_bytes_rx=0;
uint32_t i2c_bytes_tx=0;


#define MODE1 0x00			//Mode  register  1
#define MODE2 0x01			//Mode  register  2
#define SUBADR1 0x02		//I2C-bus subaddress 1
#define SUBADR2 0x03		//I2C-bus subaddress 2
#define SUBADR3 0x04		//I2C-bus subaddress 3
#define ALLCALLADR 0x05     //LED All Call I2C-bus address
#define LED0 0x6			//LED0 start register
#define LED0_ON_L 0x6		//LED0 output and brightness control byte 0
#define LED0_ON_H 0x7		//LED0 output and brightness control byte 1
#define LED0_OFF_L 0x8		//LED0 output and brightness control byte 2
#define LED0_OFF_H 0x9		//LED0 output and brightness control byte 3
#define LED_MULTIPLYER 4	// For the other 15 channels
#define ALLLED_ON_L 0xFA    //load all the LEDn_ON registers, byte 0 (turn 0-7 channels on)
#define ALLLED_ON_H 0xFB	//load all the LEDn_ON registers, byte 1 (turn 8-15 channels on)
#define ALLLED_OFF_L 0xFC	//load all the LEDn_OFF registers, byte 0 (turn 0-7 channels off)
#define ALLLED_OFF_H 0xFD	//load all the LEDn_OFF registers, byte 1 (turn 8-15 channels off)
#define PRE_SCALE 0xFE		//prescaler for output frequency
#define CLOCK_FREQ 25000000.0 //25MHz default osc clock

PCA9685* PCA9685_new(uint8_t address){
    PCA9685* ptr;
    ptr = pvPortMalloc(sizeof(PCA9685)); 
    if(ptr != NULL){
        ptr->address = address;
        PCA9685_init(ptr);
        return ptr;
    }
    return NULL;
}

void PCA9685_free(PCA9685* ptr){
   vPortFree(ptr);
}


void PCA9685_setPWM_i(PCA9685* ptr, uint8_t led, int on_value, int off_value) {
    if(ptr==NULL) return;
    ptr->buffer[0] = LED0_ON_L + LED_MULTIPLYER * led;
    ptr->buffer[1] = on_value & 0xFF;
    ptr->buffer[2] = on_value >> 8;
    ptr->buffer[3] = off_value & 0xFF;
    ptr->buffer[4] = off_value >> 8;
    i2c_bytes_tx+=5;
    I2C_Write_Blk(ptr->address,ptr->buffer,5);
}

void PCA9685_setPWM(PCA9685* ptr, uint8_t led, uint16_t value) {
    if(ptr==NULL) return;
    if(value>4095) value = 4095;
    if(led>15) led = 15;
	PCA9685_setPWM_i(ptr, led, 0, value);
}

void PCA9685_reset(PCA9685* ptr) {
    if(ptr==NULL) return;
    i2c_bytes_tx+=2;
	I2C_Write(ptr->address,MODE1, 0x00); //Normal mode
	I2C_Write(ptr->address,MODE2, 0x04); //totem pole

}

void PCA9685_setPWMFreq(PCA9685* ptr, int freq) {
    if(ptr==NULL) return;
    i2c_bytes_tx+=4;
	uint8_t prescale_val = (CLOCK_FREQ / 4096 / freq)  - 1;
	I2C_Write(ptr->address,MODE1, 0x10); //sleep
	I2C_Write(ptr->address,PRE_SCALE, prescale_val); // multiplyer for PWM frequency
	I2C_Write(ptr->address,MODE1, 0x80); //restart
	I2C_Write(ptr->address,MODE2, 0x04); //totem pole (default)
}

void PCA9685_init(PCA9685* ptr) {
    if(ptr==NULL) return;
	PCA9685_reset(ptr);
	PCA9685_setPWMFreq(ptr, 1000);
    i2c_bytes_tx+=1;
    I2C_Write(ptr->address,MODE1, 0x21); //0010 0001 : AI ENABLED
}