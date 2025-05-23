// Feature test macro to enable various functions related to pseudo terminals
#define _XOPEN_SOURCE 600
#include <sys/select.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>
#include "UART.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "console.h"
#include <errno.h>
#include <termios.h>
#include <unistd.h>


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


typedef struct __sck__
{
    int sock;
    int rc;
	struct sockaddr_in cliAddr;
	int cliAddrLen;
	struct sockaddr_in servAddr;

	int pseudo_terminal;
}sck;

#define TTY_SYMLINK "/tmp/ud3sim-uart"

int setup_pseudo_serial() {
	// Set up pseudo terminal
	int const pseudo_port = posix_openpt(O_RDWR | O_NONBLOCK);
	grantpt(pseudo_port);
	unlockpt(pseudo_port);
	// Create symlink at fixed path for convienience (ptsname will vary between runs)
	remove(TTY_SYMLINK);
	symlink(ptsname(pseudo_port), TTY_SYMLINK);
	// I think you're supposed to set a bunch of flags on the PTY here (baudrate, XON/XOFF, and so on), but it seems to
	// work fine with the defaults.
	return pseudo_port;
}

void tsk_udp_rx(void *pvParameters) {
	sck* ip = pvParameters;
	
	ip->rc = -1;
	
	const int y = 1;
	
	ip->sock = socket (PF_INET, SOCK_DGRAM, 0);
	
		
	if (ip->sock < 0) {
		console_print("Socket error\n");
	}
	
	
	ip->pseudo_terminal = setup_pseudo_serial();
	/* Lokalen Server Port bind(en) */
	ip->servAddr.sin_family = AF_INET;
	ip->servAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	ip->servAddr.sin_port = htons (MIN_PORT);
	setsockopt(ip->sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
	ip->rc = bind ( ip->sock, (struct sockaddr *) &ip->servAddr,
			  sizeof (ip->servAddr));
	if (ip->rc < 0) {
		console_print("Bind error\n");
	}
	

	while(1){
		vTaskDelay(1);
		struct pollfd polls[2] = {
			{ip->pseudo_terminal, POLLIN, 0},
			{ip->sock, POLLIN, 0},
		};
		int rc = poll(polls, 2, -1);
		if (rc <= 0) {
			continue;
		}
		int len = -1;
		if (polls[0].revents & POLLIN) {
			len = read(ip->pseudo_terminal, r_buffer, sizeof(r_buffer));
		} else if (polls[1].revents & POLLIN) {
			len = recvfrom(ip->sock, r_buffer, sizeof(r_buffer), 0,
			                       (struct sockaddr *)&ip->cliAddr,
			                       &ip->cliAddrLen);
		}
		//console_print("str %s\n",strerror(errno));
		if(len!=-1){
			r_buffer[len] = '\0';
			//console_print("len: %i received: '%s' from client %s\n",len,  r_buffer,
			//		inet_ntoa(ip->cliAddr.sin_addr));
			for(uint32_t i=0;i<len;i++){
				BufferIn(r_buffer[i]);
			}
		}
		
	}
	
}

void tsk_udp_tx(void *pvParameters) {
	
	sck* ip = pvParameters;
	
	frm to_send;
	
	xframes = xQueueCreate( 256, sizeof(frm) );
	

	while (ip->rc == -1) {
		vTaskDelay(10);
	}
	
	while(1){
	
		
		if(xQueueReceive( xframes,&to_send, 1 )){
			sendto(ip->sock, to_send.data, to_send.len, 0, (struct sockaddr *)&ip->cliAddr,sizeof(ip->cliAddr));
			write(ip->pseudo_terminal, to_send.data, to_send.len);
		}
		
		
	}
	
}


xTaskHandle tsk_udp_rx_TaskHandle;
xTaskHandle tsk_udp_tx_TaskHandle;


int tsk_started = 0;


sck sim_socket;

void UART_Start(){
	
	
	if(!tsk_started){
		
		xTaskCreate(tsk_udp_rx, "UDP-RX", 1024, &sim_socket, 3, &tsk_udp_rx_TaskHandle);
		xTaskCreate(tsk_udp_tx, "UDP-TX", 1024, &sim_socket, 3, &tsk_udp_tx_TaskHandle);
	
		frame.len = 0;
		tsk_started=1;
	}
	
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
