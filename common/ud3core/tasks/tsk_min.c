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

#include "tsk_min.h"
#include "tsk_midi.h"
#include "tsk_uart.h"
#include "tsk_cli.h"
#include "tsk_fault.h"
#include "tsk_overlay.h"
#include "tsk_eth_common.h"
#include "alarmevent.h"
#include "clock.h"
#include "version.h"

#include "helper/printf.h"
#include "helper/debug.h"

#include "TTerm.h"
#include "cli_basic.h"

xTaskHandle tsk_min_TaskHandle;
uint8 tsk_min_initVar = 0u;

SemaphoreHandle_t min_Semaphore;


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
#include "helper/printf.h" 

struct min_context min_ctx;
struct _time time;

uint32_t uart_bytes_received=0;
uint32_t uart_bytes_transmitted=0;

/* `#END` */
/* ------------------------------------------------------------------------ */
/*
 * User defined task-local code that is used to process commands, control
 * operations, and/or generrally do stuff to make the taks do something
 * meaningful.
 */
/* `#START USER_TASK_LOCAL_CODE` */

#define LOCAL_UART_BUFFER_SIZE  250     //bytes
#define FLOW_RETRANSMIT_TICKS 50

#define ITEMS			32								//Stärke des Lowpass-Filters

typedef struct {
	uint8_t i;
	int32_t total;
	int32_t last;
	int32_t samples[ITEMS];
} average_buff;

average_buff sample;


/*******************************************************************************************
Makes avaraging of given values
In	= buffer (avarage_buff) for storing samples
IN	= new_sample (int)
OUT = avaraged value (int) 
********************************************************************************************/
int average (average_buff *buffer, int new_sample){
	buffer->total -= buffer->samples[buffer->i];
	buffer->total += new_sample;
	buffer->samples[buffer->i] = new_sample;
	buffer->i = (buffer->i+1) % ITEMS;
	buffer->last = buffer->total / ITEMS;
	return buffer->last;
}


uint16_t min_tx_space(uint8_t port){
    return (UART_TX_BUFFER_SIZE - UART_GetTxBufferSize());
}

uint32_t min_rx_space(uint8_t port){
    return (UART_RX_BUFFER_SIZE - UART_GetRxBufferSize());
}

void min_tx_byte(uint8_t port, uint8_t byte){
    uart_bytes_transmitted++;
    UART_PutChar(byte);
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
void time_cb(uint32_t remote_time){
    time.remote = remote_time;
    time.diff_raw = time.remote-l_time;
    time.diff = average(&sample,time.diff_raw);
    if(time.diff>1000 ||time.diff<-1000){
        clock_set(time.remote);
        time.resync++;   
        //clock_reset_inc();
    }else{
        clock_trim(time.diff);
    }   
}

void min_debug_prt(const char *fmt, ...){
    if(debug_port == NULL) return;
    char buf[100];
    va_list args;
    va_start (args, fmt );
    uint8_t len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end (args);
    send_buffer(buf,len,debug_port);
}

uint8_t flow_ctl=1;
static const uint8_t min_stop = 'x';
static const uint8_t min_start = 'o';


struct _socket_info socket_info[NUM_MIN_CON];

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

void send_command(struct min_context *ctx, uint8_t cmd, char *str){
    uint8_t len=0;
    uint8_t buf[40];
    buf[0] = cmd;
    len=strlen(str);
    if(len>sizeof(buf)-1)len = sizeof(buf)-1;
    memcpy(&buf[1],str,len);
    min_queue_frame(ctx,MIN_ID_COMMAND,buf,len+1);
}
uint8_t transmit_features=0;

void min_command(uint8_t command, uint8_t *min_payload, uint8_t len_payload){
    switch(command){
        case CMD_LINK:
            sysfault.link_state = *min_payload ? pdTRUE : pdFALSE;
            break;
        default:
            alarm_push(ALM_PRIO_INFO, warn_min_command, command);
            break;      
    }
}

struct __event_response {
    uint8 id;
    uint8 struct_version;
    uint32 unique_id[2];
    char udname[16];   
};
typedef struct __event_response event_resonse;

void min_event(uint8_t command, uint8_t *min_payload, uint8_t len_payload){
    if(command==EVENT_GET_INFO){
        event_resonse response;
        response.id = EVENT_GET_INFO;
        response.struct_version = 1;
        CyGetUniqueId(response.unique_id);
        strncpy(response.udname, configuration.ud_name, sizeof(configuration.ud_name));
        min_send_frame(&min_ctx,MIN_ID_EVENT,(uint8_t*)&response,sizeof(response));
    }else{
        alarm_push(ALM_PRIO_INFO, warn_min_command, command);
    }    
}

void min_application_handler(uint8_t min_id, uint8_t *min_payload, uint8_t len_payload, uint8_t port)
{
    if(min_id==debug_id && debug_port!=NULL){
        xSemaphoreTake(debug_port->term_block, 100);
        send_buffer(min_payload,len_payload,debug_port);
        xSemaphoreGive(debug_port->term_block);
    }
    
    switch(min_id){
        case 0 ... 9:
            if(min_id>(NUM_MIN_CON-1)) return;
            if(socket_info[min_id].socket==SOCKET_DISCONNECTED) return;
            xStreamBufferSend(min_port[min_id].rx,min_payload, len_payload,1);
            return;
        case MIN_ID_MIDI:
            switch(param.synth){
                case SYNTH_OFF:
       
                    break;
                case SYNTH_MIDI:
                    process_midi(min_payload,len_payload);     
                    break;
                case SYNTH_SID:
                    process_sid(min_payload, len_payload);
                    break;
                case SYNTH_MIDI_QCW:
                    process_midi(min_payload,len_payload);     
                    break;
                case SYNTH_SID_QCW:
                    process_sid(min_payload, len_payload);
                    break;
            }
            return;
        case MIN_ID_SID:
            process_min_sid(min_payload, len_payload);
            return;
        case MIN_ID_WD:
                if(len_payload==4){
                	time.remote  = ((uint32_t)min_payload[0]<<24);
			        time.remote |= ((uint32_t)min_payload[1]<<16);
			        time.remote |= ((uint32_t)min_payload[2]<<8);
			        time.remote |= (uint32_t)min_payload[3];
                    time_cb(time.remote);
                }
                WD_reset();
            return;
        case MIN_ID_SOCKET:
            if(*min_payload>(NUM_MIN_CON-1)) return;
            socket_info[*min_payload].socket = *(min_payload+1);
            strncpy(socket_info[*min_payload].info,(char*)min_payload+2,sizeof(socket_info[0].info));
            if(socket_info[*min_payload].socket==SOCKET_CONNECTED){
                //command_cls("",&min_port[*min_payload]); // <-------------need to port
                //send_string(":>", &min_port[*min_payload]);  //<-------------need to port
                if(!transmit_features){
                    transmit_features=sizeof(version)/sizeof(char*);
                }
            }else{
                min_port[*min_payload].term_mode = PORT_TERM_VT100;    
                stop_overlay_task(min_handle[*min_payload]);   
            }
            return;
        case MIN_ID_SYNTH:
            process_synth(min_payload,len_payload);
            return;
        case MIN_ID_COMMAND:
            if(len_payload<1) return;
            min_command(min_payload[0], &min_payload[1],--len_payload);
            return;
        case MIN_ID_EVENT:
            min_event(min_payload[0], &min_payload[1],--len_payload);
            return;
        case MIN_ID_ALARM:
            alarm_push_c(min_payload[0],(char*)&min_payload[5],len_payload-5,min_payload[1] | (min_payload[2] << 2) | (min_payload[3] << 16) | (min_payload[4] << 24));
            return;
        default:
            break; 
    }
    if(min_id!=debug_id){
        alarm_push(ALM_PRIO_INFO, warn_min_id, min_id);
    }
}


void poll_UART(){
    uint16_t bytes = UART_GetRxBufferSize();
    if(bytes){
        rx_blink_Write(1);
        uart_bytes_received+=bytes;
        while(bytes){
            min_rx_byte(&min_ctx, UART_GetByte());
            bytes--;
        }
    }
    min_poll(&min_ctx, NULL, 0);
    
}

uint8_t assemble_command(uint8_t cmd, char *str, uint8_t *buf){
    uint8_t len=0;
    *buf = cmd;
    buf++;
    len=strlen(str);
    memcpy(buf,str,len);
    return len+1;
}

void send_command_wq(struct min_context *ctx, uint8_t cmd, char *str){
    uint8_t len=0;
    uint8_t buf[40];
    buf[0] = cmd;
    len=strlen(str);
    if(len>sizeof(buf)-1)len = sizeof(buf)-1;
    memcpy(&buf[1],str,len);
    min_send_frame(ctx,MIN_ID_COMMAND,buf,len+1);
}

void min_reset_flow(void){
    flow_ctl=0;
}

uint8_t min_queue(uint8_t id, uint8_t *data, uint8_t len, TickType_t ticks){
    if(min_queue_has_space_for_frame(&min_ctx,len)== false) return pdFAIL;
    
    if(xSemaphoreTake(min_Semaphore,ticks)){
        uint8_t ret = min_queue_frame(&min_ctx,id,data,len);      
        xSemaphoreGive(min_Semaphore);
        return ret;   
    }else{
        return pdFAIL;
    }      
}

uint8_t min_send(uint8_t id, uint8_t *data, uint8_t len, TickType_t ticks){
    if(xSemaphoreTake(min_Semaphore,ticks)){
        min_send_frame(&min_ctx,id,data,len);      
        xSemaphoreGive(min_Semaphore);
        return pdTRUE;   
    }else{
        return pdFAIL;
    }      
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
    uint8_t bytes_cnt=0;
    
    
   

	/* `#END` */

	/*
	 * Add the task initialzation code in the below merge region to be included
	 * in the task.
	 */
	/* `#START TASK_INIT_CODE` */
    

    //Init MIN Protocol
    min_init_context(&min_ctx, 0);
    
    for(uint8_t i=0;i<NUM_MIN_CON;i++){
        socket_info[i].socket=SOCKET_DISCONNECTED;   
        socket_info[i].old_state=SOCKET_DISCONNECTED;
        socket_info[i].info[0] = '\0';
    }
    
        
	/* `#END` */
    uint8_t i=0;
    uint16_t bytes_waiting=0;
    
    uint32_t next_sid_flow = 0;
    alarm_push(ALM_PRIO_INFO,warn_task_min, ALM_NO_VALUE);
    
    xSemaphoreGive(min_Semaphore);
    
    send_command_wq(&min_ctx,CMD_HELLO_WORLD, configuration.ud_name);
    
	for (;;) {
        
        if(xSemaphoreTake(min_Semaphore,100)){
            bytes_waiting=UART_GetRxBufferSize();
            
            if(transmit_features){
                uint8_t temp=(sizeof(version)/sizeof(char*))-transmit_features;
                min_queue_frame(&min_ctx, MIN_ID_FEATURE, (uint8_t*)version[temp],strlen(version[temp]));  
                transmit_features--;
            }
        
        
        
            for(i=0;i<NUM_MIN_CON;i++){
            
        		/* `#START TASK_LOOP_CODE` */
                 
                poll_UART();
                if(socket_info[i].socket==SOCKET_DISCONNECTED) goto end;   
                
                
                if(param.synth==SYNTH_SID || param.synth==SYNTH_SID_QCW){
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

                uint16_t eth_bytes=xStreamBufferBytesAvailable(min_port[i].tx);
                if(eth_bytes){
                    bytes_waiting+=eth_bytes;
                    if(eth_bytes>LOCAL_UART_BUFFER_SIZE) eth_bytes = LOCAL_UART_BUFFER_SIZE;
                    if(min_queue_has_space_for_frame(&min_ctx,eth_bytes)){
                        bytes_cnt = xStreamBufferReceive(min_port[i].tx, buffer, eth_bytes, 0);
                        if(bytes_cnt){
                            min_queue_frame(&min_ctx,i,buffer,bytes_cnt);
                        }
                    }
                }
                end:;
            }
            xSemaphoreGive(min_Semaphore);
            if(bytes_waiting==0){
                vTaskDelay(1);
            }
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
    UART_Start();
    
    min_Semaphore = xSemaphoreCreateBinary();
    
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