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
#include "tsk_sid.h"
#include "tsk_midi.h"
#include "tsk_uart.h"
#include "tsk_cli.h"
#include "tsk_fault.h"
#include "tsk_overlay.h"
#include "tsk_eth_common.h"
#include "alarmevent.h"
#include "clock.h"
#include "version.h"
#include "SignalGenerator.h"
#include "SidProcessor.h"

#include "helper/printf.h"
#include "helper/debug.h"

#include "TTerm.h"
#include "cli_basic.h"

#include "helper/nvm.h"
#include "helper/buffer.h"
#include "NoteMapper.h"
#include "VMS.h"
#include "VMSWrapper.h"

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
struct _time min_time;

uint32_t uart_bytes_rx=0;
uint32_t uart_bytes_tx=0;

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

typedef struct {
    uint32_t id;
    VMS_Block_t block;
} VMS_WIRE_DATA_t;

typedef struct {
    uint32_t id;
    MAPTABLE_HEADER_t header;
} MAP_HEADER_WIRE_DATA_t;

typedef struct {
    //uint8_t id;
    MAPTABLE_ENTRY_t entry;
} MAP_ENTRY_WIRE_DATA_t;


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
    uart_bytes_tx++;
    UART_PutChar(byte);
}

uint32_t min_time_ms(void){
  return (xTaskGetTickCount() * portTICK_RATE_MS);
}

void min_reset(uint8_t port){
    //USBMIDI_1_callbackLocalMidiEvent(0, (uint8_t*)kill_msg);   
    alarm_push(ALM_PRIO_WARN, "COM: MIN reset",ALM_NO_VALUE);
}

void min_tx_start(uint8_t port){
	#ifdef SIMULATOR
	UART_start_frame();
	#endif
}
void min_tx_finished(uint8_t port){
	#ifdef SIMULATOR
	UART_end_frame();
	#endif
}
void time_cb(uint32_t remote_time){
    min_time.remote = remote_time;
    min_time.diff_raw = min_time.remote-l_time;
    min_time.diff = average(&sample,min_time.diff_raw);
    if(min_time.diff>1000 ||min_time.diff<-1000){
        clock_set(min_time.remote);
        min_time.resync++;
        //clock_reset_inc();
    }else{
        clock_trim(min_time.diff);
    }   
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
                SidProcessor_resetSid();
            }
            break;
        case SYNTH_CMD_SID:
            param.synth=SYNTH_SID;
            callback_SynthFunction(0,0,0);
            break;
        case SYNTH_CMD_MIDI:
            param.synth=SYNTH_MIDI;
            callback_SynthFunction(0,0,0);
            break;
        case SYNTH_CMD_OFF:
            param.synth=SYNTH_OFF;
            callback_SynthFunction(0,0,0);
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
            alarm_push(ALM_PRIO_INFO, "COM: Unknown command", command);
            break;      
    }
}

struct __event_response {
    uint8_t id;
    uint8_t struct_version;
    uint32_t unique_id[2];
    char udname[16];   
};

typedef struct __event_response event_resonse;


struct __os_info {
    uint8_t struct_version;
    uint32_t free_heap;
};

typedef struct __os_info os_info;

void min_event(uint8_t command, uint8_t *min_payload, uint8_t len_payload){
    event_resonse response;
    switch(command){
        case EVENT_GET_INFO:
            response.id = EVENT_GET_INFO;
            response.struct_version = EVENT_STRUCT_VERSION;
            CyGetUniqueId(response.unique_id);
            strncpy(response.udname, configuration.ud_name, sizeof(configuration.ud_name));
            min_send_frame(&min_ctx,MIN_ID_EVENT,(uint8_t*)&response,sizeof(response));
            break;
        case EVENT_ETH_INIT_FAIL:
            alarm_push(ALM_PRIO_INFO, "EVENT: Fibernet ETH init fail", ALM_NO_VALUE);
            break;
        case EVENT_ETH_INIT_DONE:
            alarm_push(ALM_PRIO_INFO, "EVENT: Fibernet ETH init success", ALM_NO_VALUE);
            break;
        case EVENT_ETH_LINK_UP:
            alarm_push(ALM_PRIO_INFO, "EVENT: Fibernet link up", ALM_NO_VALUE);
            break;
        case EVENT_ETH_LINK_DOWN:
            alarm_push(ALM_PRIO_INFO, "EVENT: Fibernet link down", ALM_NO_VALUE);
            break;
        case EVENT_ETH_DHCP_SUCCESS:
            alarm_push(ALM_PRIO_INFO, "EVENT: DHCP success", ALM_NO_VALUE);
            break;
        case EVENT_ETH_DHCP_FAIL:
            alarm_push(ALM_PRIO_INFO, "EVENT: DHCP fail", ALM_NO_VALUE);
            break;
        case EVENT_FS_CARD_CONNECTED:
            alarm_push(ALM_PRIO_INFO, "EVENT: Fibernet SD connected", ALM_NO_VALUE);
            break;
        case EVENT_FS_CARD_REMOVED:
            alarm_push(ALM_PRIO_INFO, "EVENT: Fibernet SD disconnected", ALM_NO_VALUE);
            break;
        default:
           alarm_push(ALM_PRIO_INFO, "COM: Unknown event", command);
           break;
    }
}

#define VMS_WRT_BLOCK      1
#define VMS_WRT_MAP_HEADER 2
#define VMS_WRT_MAP_DATA   3
#define VMS_WRT_FLUSH      4
#define VMS_WIRE_SIZE (sizeof(VMS_WIRE_DATA_t)) 

void min_vms(uint8_t *min_payload, uint8_t len_payload){
    
    uint8_t packet_type = *min_payload;
    min_payload++;
    len_payload--;
    
    
    static MAP_HEADER_WIRE_DATA_t* map_header;
    static MAPTABLE_ENTRY_t* map_entries;
    volatile static MAP_ENTRY_WIRE_DATA_t* map_entry;
    static uint8_t n_map_entry=0;
    static uint16_t last_write_index=0;
    
    switch (packet_type){
        case VMS_WRT_BLOCK:
            if((len_payload % VMS_WIRE_SIZE) != 0){
                alarm_push(ALM_PRIO_WARN, "COM: Malformed block write", len_payload);
                return;
            }
            
            //convert wire byte to vms stuff
            VMS_WIRE_DATA_t * data = (VMS_WIRE_DATA_t *) min_payload;
            nvm_write_buffer(MAPMEM_SIZE + (sizeof(VMS_Block_t) * (data->id)), (uint8_t*) &(data->block), sizeof(VMS_Block_t));
            TERM_printDebug(min_handle[1], "got block: %d target=%d\r\n", data->id, data->block.target);
            
            break;
        case VMS_WRT_MAP_HEADER:
            //header will be needed again later, allocate ram
            if(map_header != NULL) vPortFree(map_header);
            map_header = pvPortMalloc(sizeof(MAP_HEADER_WIRE_DATA_t));
            memcpy(map_header, min_payload, sizeof(MAP_HEADER_WIRE_DATA_t));
            
            if (map_header->header.listEntries != 0) {
                map_entries = pvPortMalloc(sizeof(MAPTABLE_ENTRY_t) * map_header->header.listEntries);
                
                TERM_printDebug(min_handle[1], "got header for %s with count %d\r\n", map_header->header.name, map_header->header.listEntries);
            } else {
                nvm_write_buffer(last_write_index, (uint8_t*)&(map_header->header), sizeof(MAPTABLE_HEADER_t));
                last_write_index += sizeof(MAPTABLE_HEADER_t);
                vPortFree(map_header);
                map_header = NULL;
            }
            n_map_entry = 0;
            break;
        case VMS_WRT_MAP_DATA:
            if(map_header != NULL || map_entries != NULL){
                
                //copy the data 
                map_entry = (MAP_ENTRY_WIRE_DATA_t*) min_payload;
                memcpy(&map_entries[n_map_entry], &(map_entry->entry), sizeof(MAPTABLE_ENTRY_t));
                n_map_entry++;
                
                TERM_printDebug(min_handle[1], "got mapentry %d of %d: id=%d notes=%d-%d freq=%d flags=%02x ot=%d startBlock=%d\r\n", n_map_entry, map_header->header.listEntries, 0, map_entry->entry.startNote, map_entry->entry.endNote, map_entry->entry.data.noteFreq, map_entry->entry.data.flags, map_entry->entry.data.targetOT, map_entry->entry.data.startblockID);
                
                if(n_map_entry == map_header->header.listEntries){
                    TERM_printDebug(min_handle[1], "write map: %s\r\n", map_header->header.name);
                    
                    nvm_write_buffer(last_write_index, (uint8_t*)&(map_header->header), sizeof(MAPTABLE_HEADER_t));
                    last_write_index += sizeof(MAPTABLE_HEADER_t);
                    nvm_write_buffer(last_write_index, (uint8_t*)map_entries, sizeof(MAPTABLE_ENTRY_t) * map_header->header.listEntries);
                    last_write_index += sizeof(MAPTABLE_ENTRY_t) * map_header->header.listEntries;
                    
                    vPortFree(map_entries);
                    vPortFree(map_header);
                    map_entries = NULL;
                    map_header = NULL;
                }
            }else{
                alarm_push(ALM_PRIO_WARN, "COM: Malformed map write", len_payload);
            }
            break;
        case VMS_WRT_FLUSH:
            last_write_index=0;
            nvm_flush();
            if(map_entries != NULL) vPortFree(map_entries);
            if(map_header != NULL) vPortFree(map_header);
            map_header = NULL;
            for(uint8_t i=0;i<NUM_MIN_CON;i++){
                if(socket_info[i].socket==SOCKET_CONNECTED){
                    TERMINAL_HANDLE* handle = min_handle[i];
                    ttprintf("\r\n VMS-Block write finished: %u\r\n", nvm_get_blk_cnt((VMS_Block_t*) NVM_blockMem));
                    
                    ttprintfEcho("\r\n\r\n%s@%s>", handle->currUserName, TERM_DEVICE_NAME);
                }
            }
            break;
        default:
            break;
    }
}

void min_application_handler(uint8_t min_id, uint8_t *min_payload, uint8_t len_payload, uint8_t port)
{
    if(min_id==debug_id && debug_port!=NULL){
        xSemaphoreTake(((port_str*)debug_port)->term_block, 100);
        (*debug_port->print)(debug_port->port, NULL, min_payload, len_payload);
        xSemaphoreGive(((port_str*)debug_port)->term_block);
    }
    
    switch(min_id){
        case 0 ... 9:
            if(min_id>(NUM_MIN_CON-1)) return;
            if(socket_info[min_id].socket==SOCKET_DISCONNECTED) return;
            xStreamBufferSend(min_port[min_id].rx,min_payload, len_payload,1);
            return;
        case MIN_ID_MIDI:
            process_midi(min_payload,len_payload);     
            return;
        case MIN_ID_SID:
            process_min_sid(min_payload, len_payload);
            return;
        case MIN_ID_WD:
                if(len_payload==4){
                    int32_t ind = 0;
                    min_time.remote = buffer_get_uint32(min_payload, &ind);
                    time_cb(min_time.remote);
                }
                WD_reset();
            return;
        case MIN_ID_SOCKET:
            if(*min_payload>(NUM_MIN_CON-1)) return;
            socket_info[*min_payload].socket = *(min_payload+1);
            strncpy(socket_info[*min_payload].info,(char*)min_payload+2,sizeof(socket_info[0].info));
            if(socket_info[*min_payload].socket==SOCKET_CONNECTED){
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
        case MIN_ID_DEBUG:
       
            return;
        case MIN_ID_VMS:
            min_vms(min_payload, len_payload);
            return;
        default:
            break; 
    }
    if(min_id!=debug_id){
        alarm_push(ALM_PRIO_INFO, "COM: Unknown MIN ID", min_id);
    }
}


void poll_UART(){
    uint16_t bytes = UART_GetRxBufferSize();
    if(bytes){
        LED_com_Write(LED_ON);
        uart_bytes_rx+=bytes;
        while(bytes){
            min_rx_byte(&min_ctx, UART_GetByte());
            bytes--;
        }
    }
    min_poll(&min_ctx, NULL, 0);
    
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
    alarm_push(ALM_PRIO_INFO, "TASK: MIN started", ALM_NO_VALUE);
    
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
        
            for(i=0;i<NUM_MIN_CON;i++){
                 
                poll_UART();
                if(socket_info[i].socket!=SOCKET_DISCONNECTED){   
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
                }
                
            }
            xSemaphoreGive(min_Semaphore);
            if(bytes_waiting==0){
                vTaskDelay(2);
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
