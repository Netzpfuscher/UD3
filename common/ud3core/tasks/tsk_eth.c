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
#include <stdarg.h>

#include "tsk_eth.h"
#include "tsk_cli.h"

xTaskHandle tsk_eth_TaskHandle;
uint8 tsk_eth_initVar = 0u;


/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */
#include "cli_common.h"
#include "tsk_midi.h"
#include "tsk_cli.h"
#include "tsk_priority.h"
#include "telemetry.h"
#include "ETH.h"



/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */


#define STREAMBUFFER_RX_SIZE    256     //bytes
#define STREAMBUFFER_TX_SIZE    1024    //bytes

#define LOCAL_ETH_BUFFER_SIZE 256


#define PORT_TELNET     23
#define PORT_MIDI       123


void process_midi(uint8_t* ptr, uint16_t len) {
	uint8_t c;
    static uint8_t midi_count = 0;
    static uint8_t midiMsg[3];
    
	while (len) {
		c = *ptr;
        len--;
        ptr++;
		if (c & 0x80) {
			midi_count = 1;
			midiMsg[0] = c;

			goto end;
		} else if (!midi_count) {
            goto end;
		}
		switch (midi_count) {
		case 1:
			midiMsg[1] = c;
			midi_count = 2;
			break;
		case 2:
			midiMsg[2] = c;
			midi_count = 0;
			if (midiMsg[0] == 0xF0) {
				if (midiMsg[1] == 0x0F) {
					watchdog_reset_Control = 1;
					watchdog_reset_Control = 0;
					goto end;
				}
			}
			USBMIDI_1_callbackLocalMidiEvent(0, midiMsg);
			break;
		}
	end:;
	}
}

void process_sid(uint8_t* ptr, uint16_t len) {
    static uint8_t SID_frame_marker=0;
    static uint8_t SID_register=0;
    static uint16_t dutycycle=0;
    static struct sid_f SID_frame;
    static uint8_t start_frame=0;
    if(len){
        uint32_t temp;
	    while(len){
            
            if(start_frame){
                switch(SID_register){
                    case 0:
                        SID_frame.freq[0]=(SID_frame.freq[0] & 0xff00) + *ptr;
                        break;
                    case 1:
                        SID_frame.freq[0] = (SID_frame.freq[0] & 0xff) + ((uint16_t)*ptr << 8);
                        temp = ((uint32_t)SID_frame.freq[0]<<7)/2179;
                        SID_frame.freq[0]=temp;
                        SID_frame.half[0]= (150000<<14) / (temp<<14);
                        break;
                    case 2:
    		            SID_frame.pw[0] = (SID_frame.pw[0] & 0xff00) + *ptr;
    		            break;      
                    case 3:
    		            SID_frame.pw[0] = (SID_frame.pw[0] & 0xff) + ((uint16_t)*ptr << 8);
    		            break;
            		case 4:
                        SID_frame.gate[0] = *ptr & 0x01;
                        SID_frame.wave[0] = *ptr & 0x80;
            			break;
                    case 7:
                        SID_frame.freq[1]=(SID_frame.freq[1] & 0xff00) + *ptr;
                        break;
                    case 8:
                        SID_frame.freq[1] = (SID_frame.freq[1] & 0xff) + ((uint16_t)*ptr << 8);
                        temp = ((uint32_t)SID_frame.freq[1]<<7)/2179;
                        SID_frame.freq[1]=temp;
                        SID_frame.half[1]= (150000<<14) / (temp<<14);
                        break;
                    case 9:
    		            SID_frame.pw[1] = (SID_frame.pw[1] & 0xff00) + *ptr;
    		            break;      
                    case 10:
    		            SID_frame.pw[1] = (SID_frame.pw[1] & 0xff) + ((uint16_t)*ptr << 8);
    		            break;
            		case 11:
                        SID_frame.gate[1] = *ptr & 0x01;
                        SID_frame.wave[1] = *ptr & 0x80;
            			break;
                    case 14:
                        SID_frame.freq[2]=(SID_frame.freq[2] & 0xff00) + *ptr;
                        break;
                    case 15:
                        SID_frame.freq[2] = (SID_frame.freq[2] & 0xff) + ((uint16_t)*ptr << 8);
                        temp = ((uint32_t)SID_frame.freq[2]<<7)/2179;
                        SID_frame.freq[2]=temp;
                        SID_frame.half[2]= (150000<<14) / (temp<<14);
                        break;
                    case 16:
    		            SID_frame.pw[2] = (SID_frame.pw[2] & 0xff00) + *ptr;
    		            break;      
                    case 17:
    		            SID_frame.pw[2] = (SID_frame.pw[2] & 0xff) + ((uint16_t)*ptr << 8);
    		            break;
            		case 18:
                        SID_frame.gate[2] = *ptr & 0x01;
                        SID_frame.wave[2] = *ptr & 0x80;
            			break;
                }
                SID_register++;
                if(SID_register==25){
                    for(uint8_t i=0;i<3;i++){
                        if (SID_frame.gate[i]){
                            if(SID_frame.wave[i]){
                                dutycycle+= ((uint32)127*(uint32)param.pw)/(127000ul/(uint32)4000); //Noise
                            }else{
                                dutycycle+= ((uint32)127*(uint32)param.pw)/(127000ul/(uint32)SID_frame.freq[i]); //Normal wave
                            }
                        }
                    }
                    telemetry.duty = dutycycle;
                    if(dutycycle>configuration.max_tr_duty){
                        SID_frame.master_pw = (param.pw * configuration.max_tr_duty) / dutycycle;
                    }else{
                        SID_frame.master_pw = param.pw;
                    }  
                    interrupter.pw = SID_frame.master_pw;
                    xQueueSend(qSID,&SID_frame,0);
                    SID_register=0;
                    start_frame=0;
                }
            }
            
            if(*ptr==0xFF){
                SID_frame_marker++;
            }else{
                SID_frame_marker=0;
            }
            if(SID_frame_marker==4){
                dutycycle=0;
                SID_register=0;
                start_frame=1;
            }
            
            len--;
            ptr++;
            
        }
        
    }
}

uint8_t stop = 'x';
uint8_t start = 'o';

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * This is the main procedure that comprises the task.  Place the code required
 * to preform the desired function within the merge regions of the task procedure
 * to add functionality to the task.
 */


void tsk_eth_TaskProc(void *pvParameters) {
	/*
	 * Add and initialize local variables that are allocated on the Task stack
	 * the the section below.
	 */
	/* `#START TASK_VARIABLES` */
    
    uint8_t buffer[LOCAL_ETH_BUFFER_SIZE];

	/* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */
    

    
    

    //uint8_t cli_socket =0xFF;
    uint8_t cli_socket[NUM_ETH_CON];
    uint8_t cli_socket_state[NUM_ETH_CON];
    uint8_t cli_socket_state_old[NUM_ETH_CON];
    
    uint8_t synth_socket[NUM_ETH_CON];
    uint8_t synth_socket_state[NUM_ETH_CON];
    
    uint8_t flow_ctl[NUM_ETH_CON];
    uint16_t bytes_waiting[NUM_ETH_CON];
    
    for(int i =0;i<NUM_ETH_CON;i++){
        synth_socket[i]=0xFF;
        cli_socket[i]=0xFF;
        flow_ctl[i]=1;
        xETH_rx[i] = xStreamBufferCreate(STREAMBUFFER_RX_SIZE,1);
        xETH_tx[i] = xStreamBufferCreate(STREAMBUFFER_TX_SIZE,256);
    }
    
    
    uint16_t len;
    
    SPIM0_Start();
    uint8_t ret;
    if(strcmp(configuration.ip_addr,"NULL")){
        //ret= ETH_StartEx(configuration.ip_gw,configuration.ip_subnet,"58:E9:40:31:CB:9B",configuration.ip_addr);
        ret= ETH_StartEx(configuration.ip_gw,configuration.ip_subnet,configuration.ip_mac,configuration.ip_addr);
        
    }else{
        ret=CYRET_TIMEOUT;
    }
	  
    if(ret==CYRET_TIMEOUT){
        for(uint8_t i=0;i<NUM_ETH_CON;i++){
            if(ETH_Terminal_TaskHandle[i]!=NULL){
                vTaskDelete(ETH_Terminal_TaskHandle[i]);
                ETH_Terminal_TaskHandle[i]=NULL;
            }  
        }
        vTaskDelete(tsk_eth_TaskHandle);    
    }

	/* `#END` */
    /* `#START TASK_LOOP_CODE` */
   
    
    
	for (;;) {
        
        
        for(uint8_t i=0;i<NUM_ETH_CON;i++){
            if (cli_socket[i] != 0xFF) {
    			cli_socket_state[i] = ETH_TcpPollSocket(cli_socket[i]);
                
    			if (cli_socket_state[i] == ETH_SR_ESTABLISHED) {
                    if(cli_socket_state[i] != cli_socket_state_old[i]){
                        command_cls("",&eth_port[i]);
                        send_string(":>", &eth_port[i]);
                    }
    				/*
    				 * Check for data received from the telnet port.
    				 */
    				len = ETH_TcpReceive(cli_socket[i],buffer,LOCAL_ETH_BUFFER_SIZE,0);
                    if(len){
    				    xStreamBufferSend(xETH_rx[i], buffer, len, 0);
                    }
    				/*
    				 * check for data waiting to be sent over the telnet port
    				 */
    				len = xStreamBufferReceive(xETH_tx[i], buffer, LOCAL_ETH_BUFFER_SIZE, 1);
                    if(len){
    				    ETH_TcpSend(cli_socket[i],buffer,len,0);
                    }
    			}
    			else if (cli_socket_state[i] != ETH_SR_LISTEN) {
                    ETH_SocketClose(cli_socket[i],1);
                    eth_port[i].term_mode = PORT_TERM_VT100;    
                    stop_overlay_task(&eth_port[i]);
    				cli_socket[i] = 0xFF;
    			}
                cli_socket_state_old[i] = cli_socket_state[i];
    		}
    		else {
    			cli_socket[i] = ETH_TcpOpenServer(PORT_TELNET);
    		}			
           
            
            if (synth_socket[i] != 0xFF) {
    			synth_socket_state[i] = ETH_TcpPollSocket(synth_socket[i]);
    			if (synth_socket_state[i] == ETH_SR_ESTABLISHED) {

                    bytes_waiting[i] = ETH_RxDataReady(synth_socket[i]);
                    
                    if(!i){
                        telemetry.num_bytes = bytes_waiting[i];
                    }else{
                        telemetry.num_bytes += bytes_waiting[i];
                    }
                    
                    switch(param.synth){
                        case SYNTH_OFF:
                            if(bytes_waiting>0){
                                len = ETH_TcpReceive(synth_socket[i],buffer,LOCAL_ETH_BUFFER_SIZE,0);
                            }
                            break;
                        case SYNTH_MIDI:
                            len = ETH_TcpReceive(synth_socket[i],buffer,LOCAL_ETH_BUFFER_SIZE,0);
            				if(len){
                                process_midi(buffer,len);
                            }
                            break;
                        case SYNTH_SID:
                            if(bytes_waiting[i]>1800){
                                if(flow_ctl[i]){
                                    ETH_TcpSend(synth_socket[i],&stop,1,0);
                                    flow_ctl[i]=0;
                                }
                            }else{
                                 if(!flow_ctl[i]){
                                    flow_ctl[i]=1;
                                    ETH_TcpSend(synth_socket[i],&start,1,0);
                                }
                            }
                            if(uxQueueSpacesAvailable(qSID) > 15){
            				    len = ETH_TcpReceive(synth_socket[i],buffer,LOCAL_ETH_BUFFER_SIZE,0);
            				    if(len){
                                    process_sid(buffer,len);
                                }
                            }
                            break;
                    }
           
    			}
    			else if (synth_socket_state[i] != ETH_SR_LISTEN) {
    				ETH_SocketClose(synth_socket[i],1);
    				synth_socket[i] = 0xFF;
    			}
    		}
    		else {
    			synth_socket[i] = ETH_TcpOpenServer(PORT_MIDI);
    		}
        }
		vTaskDelay(2 /portTICK_RATE_MS);


		/* `#END` */
	}
}
/* ------------------------------------------------------------------------ */
void tsk_eth_Start(void) {
	/*
	 * Insert task global memeory initialization here. Since the OS does not
	 * initialize ANY global memory, execute the initialization here to make
	 * sure that your task data is properly 
	 */
	/* `#START TASK_GLOBAL_INIT` */

	/* `#END` */

	if (tsk_eth_initVar != 1) {

		/*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_eth_TaskProc, "ETH-Svc", STACK_ETH, NULL, PRIO_ETH, &tsk_eth_TaskHandle);
		tsk_eth_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */
