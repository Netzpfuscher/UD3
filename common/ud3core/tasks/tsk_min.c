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

/**
 * @file tsk_min.c
 * @brief MIN protocol task implementation
 *
 * See tsk_min.h for function documentation.
 */

#include "cyapicallbacks.h"
#include <cytypes.h>

#include "tsk_min.h"
#include "tsk_sid.h"
#include "tsk_midi.h"
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


average_buff sample = {0};

/**
 * @brief Computes moving average for time synchronization
 * @param buffer Pointer to average buffer structure
 * @param new_sample New sample value to add to the filter
 * @return Averaged value (32-sample moving average)
 */
int average (average_buff *buffer, int new_sample){
	buffer->total -= buffer->samples[buffer->i];
	buffer->samples[buffer->i] = new_sample;
	buffer->total += new_sample;
	buffer->i = (buffer->i+1) % ITEMS;
	buffer->last = buffer->total / ITEMS;
	return buffer->last;
}

/**
 * @brief MIN protocol callback - returns available TX buffer space
 * @param port Port number (unused, UART is fixed)
 * @return Number of bytes available in UART TX buffer
 */
uint16_t min_tx_space(uint8_t port){
    return (UART_TX_BUFFER_SIZE - UART_GetTxBufferSize());
}

/**
 * @brief MIN protocol callback - returns available RX buffer space
 * @param port Port number (unused, UART is fixed)
 * @return Number of bytes available in UART RX buffer
 */
uint32_t min_rx_space(uint8_t port){
    return (UART_RX_BUFFER_SIZE - UART_GetRxBufferSize());
}

/**
 * @brief MIN protocol callback - transmit single byte
 * @param port Port number (unused, UART is fixed)
 * @param byte Byte to transmit over UART
 */
void min_tx_byte(uint8_t port, uint8_t byte){
    uart_bytes_tx++;
    UART_PutChar(byte);
}

/**
 * @brief MIN protocol callback - get current system time
 * @return Current time in milliseconds since system start
 */
uint32_t min_time_ms(void){
  return (xTaskGetTickCount() * portTICK_RATE_MS);
}

/**
 * @brief MIN protocol callback - called when protocol reset is detected
 * @param port Port number (unused)
 */
void min_reset(uint8_t port){
    //USBMIDI_1_callbackLocalMidiEvent(0, (uint8_t*)kill_msg);   
    alarm_push(ALM_PRIO_WARN, "COM: MIN reset",ALM_NO_VALUE);
}

/**
 * @brief MIN protocol callback - called before starting frame transmission
 * @param port Port number (unused)
 * @note Simulator-only: marks frame boundary for debugging
 */
void min_tx_start(uint8_t port){
	#ifdef SIMULATOR
	UART_start_frame();
	#endif
}

/**
 * @brief MIN protocol callback - called after finishing frame transmission
 * @param port Port number (unused)
 * @note Simulator-only: marks frame boundary for debugging
 */
void min_tx_finished(uint8_t port){
	#ifdef SIMULATOR
	UART_end_frame();
	#endif
}

/**
 * @brief Time synchronization callback - syncs local clock with remote time
 * @param remote_time Remote system time in milliseconds
 * @note Uses moving average to smooth jitter. Hard reset if drift >1 second.
 */
void time_cb(uint32_t remote_time){
    min_time.remote = remote_time;
    min_time.diff_raw = min_time.remote-l_time;
    min_time.diff = average(&sample,min_time.diff_raw);
    if(min_time.diff>1000 ||min_time.diff<-1000){
        // Large drift - force resync
        clock_set(min_time.remote);
        min_time.resync++;
    }else{
        // Small drift - trim incrementally
        clock_trim(min_time.diff);
    }   
}


uint8_t flow_ctl=1;
static const uint8_t min_stop = 'x';
static const uint8_t min_start = 'o';


struct _socket_info socket_info[NUM_MIN_CON];

/**
 * @brief Process synthesizer control commands (SID/MIDI/OFF)
 * @param min_payload Pointer to command payload (first byte is command)
 * @param len_payload Length of payload in bytes
 */
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

/**
 * @brief Queue a command frame with string argument
 * @param ctx MIN protocol context
 * @param cmd Command byte
 * @param str String argument (truncated to 39 bytes if longer)
 */
void send_command(struct min_context *ctx, uint8_t cmd, char *str){
    uint8_t len=0;
    uint8_t buf[40];
    buf[0] = cmd;
    len=strlen(str);
    if(len>sizeof(buf)-1)len = sizeof(buf)-1;
    memcpy(&buf[1],str,len);
    min_queue_frame(ctx,MIN_ID_COMMAND,buf,len+1);
}

// Feature transmission counter - counts down from total features to 0
uint8_t transmit_features=0;

/**
 * @brief Process incoming command messages
 * @param command Command byte
 * @param min_payload Pointer to command payload
 * @param len_payload Length of payload in bytes
 */
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

/**
 * @brief Process incoming event messages (fibernet status, device info, etc)
 * @param command Event type
 * @param min_payload Pointer to event payload
 * @param len_payload Length of payload in bytes
 */
void min_event(uint8_t command, uint8_t *min_payload, uint8_t len_payload){
    event_resonse response;
    switch(command){
        case EVENT_GET_INFO:
            response.id = EVENT_GET_INFO;
            response.struct_version = EVENT_STRUCT_VERSION;
            CyGetUniqueId(response.unique_id);
            strncpy(response.udname, configuration.ud_name, sizeof(response.udname));
            response.udname[sizeof(response.udname)-1] = '\0';
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

/**
 * @brief Process VMS (Voice Memory System) data writes to EEPROM
 * @param min_payload Pointer to VMS command payload
 * @param len_payload Length of payload in bytes
 * 
 * Handles multi-packet VMS transfers:
 * - VMS_WRT_BLOCK: Write individual VMS audio blocks
 * - VMS_WRT_MAP_HEADER: Receive map table header, allocate buffers
 * - VMS_WRT_MAP_DATA: Accumulate map entries, write when complete
 * - VMS_WRT_FLUSH: Finalize EEPROM writes and cleanup buffers
 */
void min_vms(uint8_t *min_payload, uint8_t len_payload){
    
    uint8_t packet_type = *min_payload;
    min_payload++;
    len_payload--;
    
    
    static MAP_HEADER_WIRE_DATA_t* map_header;
    static MAPTABLE_ENTRY_t* map_entries;
    static MAP_ENTRY_WIRE_DATA_t* map_entry;
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
                if(map_entries != NULL) vPortFree(map_entries);
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
            if(map_header != NULL && map_entries != NULL){
                
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
            if(map_entries != NULL) {
                vPortFree(map_entries);
                map_entries = NULL;
            }
            if(map_header != NULL) {
                vPortFree(map_header);
                map_header = NULL;
            }
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

/**
 * @brief Main MIN protocol frame dispatcher
 * @param min_id MIN frame ID (channel 0-9, or protocol ID)
 * @param min_payload Pointer to frame payload
 * @param len_payload Length of payload in bytes
 * @param port Port number (unused, always UART)
 * 
 * Routes incoming MIN frames to appropriate handlers:
 * - 0-9: Terminal/CLI data for socket connections
 * - MIN_ID_MIDI/SID: Audio synthesis data
 * - MIN_ID_WD: Watchdog and time sync
 * - MIN_ID_COMMAND/EVENT: Control messages
 * - MIN_ID_VMS: Voice Memory System uploads
 */
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
            socket_info[*min_payload].info[sizeof(socket_info[0].info)-1] = '\0';
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
            len_payload--;
            min_command(min_payload[0], &min_payload[1], len_payload);
            return;
        case MIN_ID_EVENT:
            len_payload--;
            min_event(min_payload[0], &min_payload[1], len_payload);
            return;
        case MIN_ID_ALARM:
            alarm_push_c(min_payload[0],(char*)&min_payload[5],len_payload-5,min_payload[1] | (min_payload[2] << 8) | (min_payload[3] << 16) | (min_payload[4] << 24));
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

/**
 * @brief Poll UART for received bytes and process MIN protocol
 * 
 * Called regularly from main task loop. Reads all available bytes from
 * UART RX buffer, feeds them to MIN decoder, and polls MIN state machine.
 */
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

/**
 * @brief Send command frame immediately (without queueing)
 * @param ctx MIN protocol context
 * @param cmd Command byte
 * @param str String argument (truncated to 39 bytes if longer)
 * @note Does NOT use semaphore protection - use during initialization only
 */
void send_command_wq(struct min_context *ctx, uint8_t cmd, char *str){
    uint8_t len=0;
    uint8_t buf[40];
    buf[0] = cmd;
    len=strlen(str);
    if(len>sizeof(buf)-1)len = sizeof(buf)-1;
    memcpy(&buf[1],str,len);
    min_send_frame(ctx,MIN_ID_COMMAND,buf,len+1);
}

/**
 * @brief Reset SID flow control to stopped state
 * @note Thread-safe: protected by min_Semaphore
 */
void min_reset_flow(void){
    if(xSemaphoreTake(min_Semaphore, portMAX_DELAY)){
        flow_ctl=0;
        xSemaphoreGive(min_Semaphore);
    }
}

/**
 * @brief Queue a MIN frame for transmission (thread-safe)
 * @param id MIN frame ID
 * @param data Pointer to frame payload
 * @param len Length of payload in bytes
 * @param ticks Timeout in FreeRTOS ticks to wait for semaphore
 * @return pdTRUE on success, pdFAIL if queue full or semaphore timeout
 */
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

/**
 * @brief Send a MIN frame immediately (thread-safe)
 * @param id MIN frame ID
 * @param data Pointer to frame payload
 * @param len Length of payload in bytes
 * @param ticks Timeout in FreeRTOS ticks to wait for semaphore
 * @return pdTRUE on success, pdFAIL on semaphore timeout
 * @note Bypasses queue, sends immediately. Use for time-critical data.
 */
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
/**
 * @brief MIN protocol task - handles UART communication and MIN framing
 * @param pvParameters Task parameters (unused)
 * 
 * Main responsibilities:
 * - Poll UART for incoming data and decode MIN frames
 * - Transmit queued MIN frames from terminal/CLI tasks
 * - Handle SID audio flow control (stop/start messages)
 * - Transmit feature list on socket connection
 * - Maintain MIN protocol state machine
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
    

    // Init MIN Protocol
    if (!min_init_context(&min_ctx, 0)) {
        alarm_push(ALM_PRIO_CRITICAL, "MIN CRC LUT corrupt", 0);
    }
    
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
/**
 * @brief Initialize and start the MIN protocol task
 * 
 * Called during system startup. Initializes UART hardware, creates binary
 * semaphore for thread-safe access, and creates the MIN service task.
 */
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
