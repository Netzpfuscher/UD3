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

#include "cyapicallbacks.h"
#include <cytypes.h>

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "tsk_priority.h"
#include "cli_common.h"
#include <stdlib.h>
#include "telemetry.h"

xTaskHandle tsk_i2c_TaskHandle;
uint8 tsk_i2c_initVar = 0u;

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

uint8_t buffer[5];

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


void PCA9685_setPWM_i(uint8_t led, int on_value, int off_value) {
    buffer[0] = LED0_ON_L + LED_MULTIPLYER * led;
    buffer[1] = on_value & 0xFF;
    buffer[2] = on_value >> 8;
    buffer[3] = off_value & 0xFF;
    buffer[4] = off_value >> 8;
    I2C_Write_Blk(0x40,buffer,5);
}

void PCA9685_setPWM(uint8_t led, uint16_t value) {
    if(value>4095) value = 4095;
    if(led>15) led = 15;
	PCA9685_setPWM_i(led, 0, value);
}

void PCA9685_reset() {

		I2C_Write(0x40,MODE1, 0x00); //Normal mode
		I2C_Write(0x40,MODE2, 0x04); //totem pole

}

void PCA9685_setPWMFreq(int freq) {

		uint8_t prescale_val = (CLOCK_FREQ / 4096 / freq)  - 1;
		I2C_Write(0x40,MODE1, 0x10); //sleep
		I2C_Write(0x40,PRE_SCALE, prescale_val); // multiplyer for PWM frequency
		I2C_Write(0x40,MODE1, 0x80); //restart
		I2C_Write(0x40,MODE2, 0x04); //totem pole (default)
}

void PCA9685_init() {
	PCA9685_reset();
	PCA9685_setPWMFreq(1000);
    I2C_Write(0x40,MODE1, 0x21); //0010 0001 : AI ENABLED
}

/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */


/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * This is the main procedure that comprises the task.  Place the code required
 * to preform the desired function within the merge regions of the task procedure
 * to add functionality to the task.
 */

uint8_t CMD_i2c(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    uint8_t byte;
    uint8_t status;
    uint8_t cnt=0;
    //PCA9685_init();
    vTaskDelay(10);
    
    uint16_t val = atoi(args[0]);
    

    ttprintf("Mode 1: %x\r\n",I2C_Read(0x40,MODE1));

       
    vTaskDelay(50);
 
    /*
    I2C_MasterClearStatus();
    do{
       
        status = I2C_MasterSendStart(cnt,I2C_READ_XFER_MODE);
        
        if(status==0) ttprintf("Found device = %x\r\n",cnt);
        if(status==0) byte = I2C_MasterReadByte(I2C_NAK_DATA);
        status = I2C_MasterSendStop();
        cnt++;
        if(cnt>127){
            cnt = 0;
            ttprintf("Finished scan...\r\n",cnt);
            break;
        }
        
        
    }while(Term_check_break(handle,20));
    */
    return TERM_CMD_EXIT_SUCCESS;
}



void tsk_i2c_TaskProc(void *pvParameters) {
	/*
	 * Add and initialize local variables that are allocated on the Task stack
	 * the the section below.
	 */
	/* `#START TASK_VARIABLES` */

	/* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */
    I2C_Start();
    
    PCA9685_init();
    
    uint16_t cnt=0;
    uint8_t tog=1;
    
	/* `#END` */

	for (;;) {

        for(uint8_t i = 0;i<16;i++){
            PCA9685_setPWM(i,cnt);
        }
        
        if(tog){
            cnt+=100;
        }else{
            cnt-=100;
        }
        if(cnt>4095) tog=0;
        if(cnt==0) tog=1;
        vTaskDelay(20);
	}
}
/* ------------------------------------------------------------------------ */
void tsk_i2c_Start(void) {
	/*
	 * Insert task global memeory initialization here. Since the OS does not
	 * initialize ANY global memory, execute the initialization here to make
	 * sure that your task data is properly 
	 */
	/* `#START TASK_GLOBAL_INIT` */
    
    

	/* `#END` */

	if (tsk_i2c_initVar != 1) {

		/*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_i2c_TaskProc, "I2C-Svc", STACK_I2C, NULL, PRIO_I2C, &tsk_i2c_TaskHandle);
		tsk_i2c_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
