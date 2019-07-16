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

#include "tsk_min.h"
#include "tsk_midi.h"
#include "tsk_uart.h"
#include "tsk_cli.h"
#include "tsk_fault.h"
#include "tsk_eth_common.h"
#include "alarmevent.h"

#include "helper/printf.h"

xTaskHandle tsk_min_TaskHandle;
uint8 tsk_min_initVar = 0u;


/* ------------------------------------------------------------------------ */
/*
 * Place user included headers, defines and task global data in the
 * below merge region section.
 */
/* `#START USER_INCLUDE SECTION` */
#include "cli_common.h"
#include "tsk_priority.h"

#include "min_id.h"
#include "telemetry.h"
#include "stream_buffer.h" 
#include "ntlibc.h" 


struct min_context min_ctx;

eth_info *pEth_info[2];
void send_config_to_esp();

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */

#define LOCAL_UART_BUFFER_SIZE  127     //bytes
#define FLOW_RETRANSMIT_TICKS 50


uint16_t min_tx_space(uint8_t port){
    return (UART_2_TX_BUFFER_SIZE - UART_2_GetTxBufferSize());
}

uint32_t min_rx_space(uint8_t port){
    return (UART_2_RX_BUFFER_SIZE - UART_2_GetRxBufferSize());
}

void min_tx_byte(uint8_t port, uint8_t byte){
    UART_2_PutChar(byte);
}

uint32_t min_time_ms(void){
  return (xTaskGetTickCount() * portTICK_RATE_MS);
}

void min_reset(uint8_t port){
    kill_accu();
    USBMIDI_1_callbackLocalMidiEvent(0, (uint8_t*)kill_msg);   
    alarm_push(ALM_PRIO_WARN,warn_min_reset,ALM_NO_VALUE);
}

void min_tx_start(uint8_t port){
}
void min_tx_finished(uint8_t port){
}

uint8_t flow_ctl=1;
static const uint8_t min_stop = 'x';
static const uint8_t min_start = 'o';


struct _socket_info socket_info[NUM_ETH_CON];

void process_command(uint8_t *min_payload, uint8_t len_payload){
    len_payload--;
    uint8_t interface=0;
    switch(*min_payload++){
        case COMMAND_IP:
            interface=*min_payload;
            if(interface>ETH_INFO_WIFI) break;
            min_payload++;
            len_payload--;
            if(len_payload>=sizeof(pEth_info[interface]->ip)-1 || pEth_info[interface]==NULL){
                ntlibc_strcpy(pEth_info[interface]->ip,"overflow");
                break;
            }
            memcpy(pEth_info[interface]->ip,min_payload,len_payload);
            min_payload[len_payload]='\0';
            break;
        case COMMAND_GW:
            interface=*min_payload;
            if(interface>ETH_INFO_WIFI) break;
            min_payload++;
            len_payload--;
            if(len_payload>=sizeof(pEth_info[interface]->gw)-1|| pEth_info[interface]==NULL){
                ntlibc_strcpy(pEth_info[interface]->gw,"overflow");
                break;
            }
            memcpy(pEth_info[interface]->gw,min_payload,len_payload);
            min_payload[len_payload]='\0';
            break;
        case COMMAND_INFO:
            interface=*min_payload;
            if(interface>ETH_INFO_WIFI) break;
            min_payload++;
            len_payload--;
            if(len_payload>=sizeof(pEth_info[interface]->info)-1|| pEth_info[interface]==NULL){
                ntlibc_strcpy(pEth_info[interface]->info,"overflow");
                break;
            }
            memcpy(pEth_info[interface]->info,min_payload,len_payload);
            min_payload[len_payload]='\0';
            break;
        case COMMAND_ETH_STATE:
            interface=*min_payload;
            if(interface>ETH_INFO_WIFI) break;
            min_payload++;
            len_payload--;
            pEth_info[interface]->eth_state = *min_payload;
            break;
        case COMMAND_GET_CONFIG:
            send_config_to_esp();
            break;
  }
}

void process_synth(uint8_t *min_payload, uint8_t len_payload){
    len_payload--;
    switch(*min_payload++){
        case SYNTH_CMD_FLUSH:
            if(qSID!=NULL){
                xQueueReset(qSID);
            }
            break;
        case SYNTH_CMD_SID:
            param.synth=SYNTH_SID;
            switch_synth(SYNTH_SID);
            break;
        case SYNTH_CMD_MIDI:
            param.synth=SYNTH_MIDI;
            switch_synth(SYNTH_MIDI);
            break;
        case SYNTH_CMD_OFF:
            param.synth=SYNTH_OFF;
            switch_synth(SYNTH_OFF);
            break;
  }
}

void min_application_handler(uint8_t min_id, uint8_t *min_payload, uint8_t len_payload, uint8_t port)
{
    if(min_id<10){
        if(min_id>NUM_ETH_CON) return;
        if(socket_info[min_id].socket==SOCKET_DISCONNECTED) return;
        xStreamBufferSend(eth_port[min_id].rx,min_payload, len_payload,1);
        return;
    }else if(min_id>=20 && min_id <30){
        switch(param.synth){
            case SYNTH_OFF:
       
                break;
            case SYNTH_MIDI:
                process_midi(min_payload,len_payload);     
                break;
            case SYNTH_SID:
                process_sid(min_payload, len_payload);
                break;
        }
        return;
    }
    switch(min_id){
        case MIN_ID_WD:
            if(*min_payload>NUM_ETH_CON) return;
                WD_reset();
            break;
        case MIN_ID_SOCKET:
            if(*min_payload>NUM_ETH_CON) return;
            socket_info[*min_payload].socket = *(min_payload+1);
            strncpy(socket_info[*min_payload].info,(char*)min_payload+2,sizeof(socket_info[0].info));
            if(socket_info[*min_payload].socket==SOCKET_CONNECTED){
                command_cls("",&eth_port[*min_payload]);
                send_string(":>", &eth_port[*min_payload]);
            }else{
                eth_port[*min_payload].term_mode = PORT_TERM_VT100;    
                stop_overlay_task(&eth_port[*min_payload]);   
            }
            break;
        case MIN_ID_COMMAND:
            process_command(min_payload,len_payload);
            break;
        case MIN_ID_SYNTH:
            process_synth(min_payload,len_payload);
            break;
            

    }
}


void poll_UART(uint8_t* ptr){
        
    uint16_t bytes = UART_2_GetRxBufferSize();
    uint16_t bytes_cnt=0;
    if(bytes){
        rx_blink_Write(1);
        if (bytes > LOCAL_UART_BUFFER_SIZE) bytes = LOCAL_UART_BUFFER_SIZE;
            while(bytes_cnt < bytes){
                ptr[bytes_cnt] = UART_2_GetByte();
                bytes_cnt++;
            }
    }
    min_poll(&min_ctx, ptr, bytes_cnt);
    
}

uint8_t assemble_command(uint8_t cmd, char *str, uint8_t *buf){
    uint8_t len=0;
    *buf = cmd;
    buf++;
    len=strlen(str);
    memcpy(buf,str,len);
    return len+1;
}

void min_reset_flow(void){
    flow_ctl=0;
}

void send_config_to_esp(){
    uint8_t len;
    uint8_t buf[40];
    len = assemble_command(COMMAND_IP,configuration.ip_addr,buf);
    min_send_frame(&min_ctx,MIN_ID_COMMAND,buf,len);
    len = assemble_command(COMMAND_GW,configuration.ip_gw,buf);
    min_send_frame(&min_ctx,MIN_ID_COMMAND,buf,len);
    len = assemble_command(COMMAND_MAC,configuration.ip_mac,buf);
    min_send_frame(&min_ctx,MIN_ID_COMMAND,buf,len);
    len = assemble_command(COMMAND_SSID,configuration.ssid,buf);
    min_send_frame(&min_ctx,MIN_ID_COMMAND,buf,len);
    len = assemble_command(COMMAND_PASSWD,configuration.passwd,buf);
    min_send_frame(&min_ctx,MIN_ID_COMMAND,buf,len);
}


/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * This is the main procedure that comprises the task.  Place the code required
 * to preform the desired function within the merge regions of the task procedure
 * to add functionality to the task.
 */
void tsk_min_TaskProc(void *pvParameters) {
	/*
	 * Add and initialize local variables that are allocated on the Task stack
	 * the the section below.
	 */
	/* `#START TASK_VARIABLES` */
    
    uint8_t buffer[LOCAL_UART_BUFFER_SIZE];
    uint8_t buffer_u[LOCAL_UART_BUFFER_SIZE];

    uint8_t bytes_cnt=0;
    
    
    pEth_info[ETH_INFO_ETH] = pvPortMalloc(sizeof(eth_info));
    pEth_info[ETH_INFO_WIFI] = pvPortMalloc(sizeof(eth_info));

	/* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */
    

    //Init MIN Protocol
    min_init_context(&min_ctx, 0);
    
    for(uint8_t i=0;i<NUM_ETH_CON;i++){
        socket_info[i].socket=SOCKET_DISCONNECTED;   
        socket_info[i].old_state=SOCKET_DISCONNECTED;
        socket_info[i].info[0] = '\0';
    }
    
    if(configuration.eth_hw==ETH_HW_ESP32){
        send_config_to_esp();
    }
        
	/* `#END` */
    uint8_t i=0;
    uint16_t bytes_waiting=0;
    
    uint32_t next_sid_flow = 0;
    alarm_push(ALM_PRIO_INFO,warn_task_min, ALM_NO_VALUE);
	for (;;) {

        bytes_waiting=UART_2_GetRxBufferSize();
        
        for(i=0;i<NUM_ETH_CON;i++){
        
    		/* `#START TASK_LOOP_CODE` */
             
            poll_UART(buffer_u);
            if(socket_info[i].socket==SOCKET_DISCONNECTED) goto end;   
            
            
            if(param.synth==SYNTH_SID){
                if(uxQueueSpacesAvailable(qSID) < 30 && flow_ctl){
                    min_queue_frame(&min_ctx, MIN_ID_MIDI, (uint8_t*)&min_stop,1);
                    flow_ctl=0;
                }else if(uxQueueSpacesAvailable(qSID) > 45 && !flow_ctl){ //
                    min_queue_frame(&min_ctx, MIN_ID_MIDI, (uint8_t*)&min_start,1);
                    flow_ctl=1;
                }else if(uxQueueSpacesAvailable(qSID) > 59){
                    if(xTaskGetTickCount()>next_sid_flow){
                        next_sid_flow = xTaskGetTickCount() + FLOW_RETRANSMIT_TICKS;
                        min_send_frame(&min_ctx, MIN_ID_MIDI, (uint8_t*)&min_start,1);
                        flow_ctl=1;
                    }
                }
            }
            uint16_t eth_bytes=xStreamBufferBytesAvailable(eth_port[i].tx);
            if(eth_bytes){
                bytes_waiting+=eth_bytes;
                bytes_cnt = xStreamBufferReceive(eth_port[i].tx, buffer, LOCAL_UART_BUFFER_SIZE, 0);
                if(bytes_cnt){
                    uint8_t res=0;
                    res = min_queue_frame(&min_ctx,i,buffer,bytes_cnt);
                    while(!res){
                        poll_UART(buffer_u);
                        vTaskDelay(1);   
                        res = min_queue_frame(&min_ctx,i,buffer,bytes_cnt);
                    }
                }
            }
            end:;
        }
        if(bytes_waiting==0){
            vTaskDelay(1);
        }
		/* `#END` */
	}
}
/* ------------------------------------------------------------------------ */
void tsk_min_Start(void) {
	/*
	 * Insert task global memeory initialization here. Since the OS does not
	 * initialize ANY global memory, execute the initialization here to make
	 * sure that your task data is properly 
	 */
	/* `#START TASK_GLOBAL_INIT` */

	/* `#END` */
    UART_2_Start();

	if (tsk_min_initVar != 1) {
		/*
	 	* Create the task and then leave. When FreeRTOS starts up the scheduler
	 	* will call the task procedure and start execution of the task.
	 	*/
		xTaskCreate(tsk_min_TaskProc, "MIN-Svc", STACK_MIN, NULL, PRIO_UART, &tsk_min_TaskHandle);
		tsk_min_initVar = 1;
	}
}
/* ------------------------------------------------------------------------ */
/* ======================================================================== */
/* [] END OF FILE */