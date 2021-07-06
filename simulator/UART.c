#include "UART.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define BUFFER_FAIL     0
#define BUFFER_SUCCESS  1

#define BUFFER_SIZE 4096

struct Buffer {
  uint8_t data[BUFFER_SIZE];
  uint32_t read; // zeigt auf das Feld mit dem ältesten Inhalt
  uint32_t write; // zeigt immer auf leeres Feld
  uint32_t len;
} buffer = {{}, 0, 0, 0};

//
// Stellt 1 Byte in den Ringbuffer
//
// Returns:
//     BUFFER_FAIL       der Ringbuffer ist voll. Es kann kein weiteres Byte gespeichert werden
//     BUFFER_SUCCESS    das Byte wurde gespeichert
//
uint8_t BufferIn(uint8_t byte)
{
  //if (buffer.write >= BUFFER_SIZE)
  //  buffer.write = 0; // erhöht sicherheit

  if ( ( buffer.write + 1 == buffer.read ) ||
       ( buffer.read == 0 && buffer.write + 1 == BUFFER_SIZE ) )
    return BUFFER_FAIL; // voll

  buffer.data[buffer.write] = byte;

  buffer.write++;
  buffer.len++;
  if (buffer.write >= BUFFER_SIZE)
    buffer.write = 0;


  return BUFFER_SUCCESS;
}

//
// Holt 1 Byte aus dem Ringbuffer, sofern mindestens eines abholbereit ist
//
// Returns:
//     BUFFER_FAIL       der Ringbuffer ist leer. Es kann kein Byte geliefert werden.
//     BUFFER_SUCCESS    1 Byte wurde geliefert
//    
uint8_t BufferOut(uint8_t *pByte)
{
  if (buffer.read == buffer.write)
    return BUFFER_FAIL;

  *pByte = buffer.data[buffer.read];

  buffer.read++;
  buffer.len--;
  if (buffer.read >= BUFFER_SIZE)
    buffer.read = 0;

  return BUFFER_SUCCESS;
}

uint32_t frame_len = 0;

typedef struct __frame__
{
    uint16_t len;
    uint8_t data[ 400 ];
}frm;

frm frame;

uint8_t r_buffer[16000];

#define MIN_PORT 1337


QueueHandle_t xframes=NULL;

void tsk_udp(void *pvParameters) {
	
	xframes = xQueueCreate( 256, sizeof(frm) );
	
	int sock, rc;

	const int y = 1;
	
	struct sockaddr_in client_address;
	int client_address_len = 0;

	struct sockaddr_in cliAddr, servAddr;
	
	sock = socket (PF_INET, SOCK_DGRAM, 0);
	
	frm to_send;
	
	if (sock < 0) {
		console_print("Socket error\n");
	}
	
	/* Lokalen Server Port bind(en) */
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	servAddr.sin_port = htons (MIN_PORT);
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
	rc = bind ( sock, (struct sockaddr *) &servAddr,
			  sizeof (servAddr));
	if (rc < 0) {
		console_print("Bind error\n");
	}
	
	while(1){
	
		int len = recvfrom(sock, r_buffer, sizeof(r_buffer), 0,
							   (struct sockaddr *)&client_address,
							   &client_address_len);
		if(len!=-1){
			r_buffer[len] = '\0';
			//console_print("len: %i received: '%s' from client %s\n",len,  r_buffer,
			//		inet_ntoa(client_address.sin_addr));
			for(uint32_t i=0;i<len;i++){
				BufferIn(r_buffer[i]);
			}				
		}
		
		if(xQueueReceive( xframes,&to_send, 1 )){
			//console_print("Frame: %u\n", to_send.len);
			sendto(sock, to_send.data, to_send.len, 0, (struct sockaddr *)&client_address,sizeof(client_address));
		}
		
		
	}
	
}
xTaskHandle tsk_udp_TaskHandle;

void UART_Start(){
	
	xTaskCreate(tsk_udp, "UDP-Svc", 1024, NULL, 3, &tsk_udp_TaskHandle);
	
	frame.len = 0;
	
}



uint32_t UART_GetTxBufferSize(){
	return buffer.len;
}
uint32_t UART_GetRxBufferSize(){
	return buffer.len;
}
void UART_PutChar(uint8_t byte){
	frame.data[frame.len] = byte;
	frame.len++;
}
uint8_t UART_GetByte(){
	uint8_t c;
	if(BufferOut(&c) == BUFFER_SUCCESS){
		return c;
	}else{
		return 0;
	}
}


void UART_start_frame(){
	frame.len = 0;
	
}

void UART_end_frame(){
	
	xQueueSend( xframes, &frame, ( TickType_t ) 0 );
	
	
	//console_print("len: %u", frame_len);
}