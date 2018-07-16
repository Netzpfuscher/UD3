/* ------------------------------------------------------------------------ */
/**
 * \addtogroup e2forlife_w5500
 * @{
 */
/**
 * Copyright (c) 2015, E2ForLife.com
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of E2ForLife.com nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \file tcp.c
 * \author Chuck Erhardt (chuck@e2forlife.com)
 * 
 * Implementation of the TCP protocol using the internal WizNET protocol stack
 * in the iEthernet devices.
 */
/* ======================================================================== */
#include <cytypes.h>
#include <cylib.h>
#include <string.h>

#include "ETH.h"

extern uint8 ETH_socketStatus[ETH_MAX_SOCKETS];

/* ------------------------------------------------------------------------ */
/**
 * \brief Read the status of the socket and check to see if a connection is established
 * \param sock (uint8) the socket number to which status shoudl be checked
 * \returns 0 Socket is not yet established
 * \returns 0xFF Socket is not open
 * \returns 0x80 Socket Timeout
 * \returns 0x01 the socket connection is established
 */
cystatus ETH_TcpConnected( uint8 sock )
{
	uint8 status;
	
	if (ETH_SOCKET_BAD(sock)) return CYRET_BAD_PARAM;
	
	ETH_Send(ETH_SREG_SR,ETH_SOCKET_BASE(sock),0,&status, 1);
	if (status == ETH_SR_ESTABLISHED) {
		return CYRET_SUCCESS;
	}
	else {
		ETH_Send(ETH_SREG_IR, ETH_SOCKET_BASE(sock),0,&status, 1);
		if ( (status & ETH_IR_TIMEOUT) != 0) {
			status = 0xFF;
			ETH_Send(ETH_SREG_IR, ETH_SOCKET_BASE(sock),1,&status,1);
			return CYRET_TIMEOUT;
		}
	}
	return CYRET_STARTED;
}
/* ------------------------------------------------------------------------ */
/**
 * \brief Open a socket and set protocol to TCP mode
 * \param port (uint16) the port number on which to open the socket
 * \param remote_ip (uint32) The remote IP address to connect
 * \param remote_port (uint16) the port on the remote server to connect
 * \returns 0xFF Unable to open the socket
 * \returns uint8 socket number allocated for this socket
 *
 *
 */
uint8 ETH_TcpOpenClient( uint16 port, uint32 remote_ip, uint16 remote_port )
{
	uint8 socket;
	uint32 timeout;
	uint8 ir;
	uint8 rCfg[6];
	
	/* open the socket using the TCP mode */
	socket = ETH_SocketOpen( port, ETH_PROTO_TCP );

	/*
	 * 2.0 Patch: retun immediately upon the detection of a socket that is not open
	 */
	if (ETH_SOCKET_BAD(socket) ) return 0xFF;
	if ( (remote_ip != 0xFFFFFFFF) && (remote_ip != 0) ) {
		/*
		 * a valid socket was opened, so now we can use the socket handle to
		 * open the client connection to the specified server IP.
		 * So, this builds a configuration packet to send to the device
		 * to reduce the operations executed at one time (lower overhead)
		 */
		remote_port = CYSWAP_ENDIAN16(remote_port);
		memcpy((void*)&rCfg[0], (void*)&remote_ip, 4);
		memcpy((void*)&rCfg[4],(void*)&remote_port,2);
		
		/*
		 * Blast out the configuration record all at once to set up the IP and
		 * port for the remote connection.
		 */
		ETH_Send(ETH_SREG_DIPR, ETH_SOCKET_BASE(socket),1,&rCfg[0], 6);

		/*
		 * Execute the connection to the remote server and check for errors
		 */
		if (ETH_ExecuteSocketCommand(socket, ETH_CR_CONNECT) == CYRET_SUCCESS) {
			timeout = 0;
			/* wait for the socket connection to the remote host is established */
			do {
				CyDelay(1);
				++timeout;
				ETH_Send(ETH_SREG_IR, ETH_SOCKET_BASE(socket), 0, &ir, 1);
				if ( (ir & 0x08) != 0 ) {
					/* internal chip timeout occured */
					timeout = 3000;
				}				
			}
			while ( ((ir&0x01) == 0)  && (timeout < 3000) );
		}
		else {
			ETH_SocketClose(socket,0);
			socket = 0xFF;
		}
	}
	return socket;
}
/* ------------------------------------------------------------------------ */
/**
 * \brief Open a TCP server socket using a specified port
 * \param port The port number to assign to the socket
 * \returns uint8 socket number for open server socket
 * \returns 0xFF Socket error, cannot open socket.
 *
 * _TcpOpenServer opens a socket with the TCP protocol on the port specified
 * by the parameter (port).  After opening the socket, _TcpOpenServer will
 * issue a LISTEN command to the W5500 to initiate a server. The server will
 * connect when a valid SYN packet is received.
 * \sa _SocketOpen
 */
uint8 ETH_TcpOpenServer(uint16 port)
{
	uint8 socket;
	uint8 status;
	
	/* open the socket using the TCP mode */
	socket = ETH_SocketOpen( port, ETH_PROTO_TCP );

	/*
	 * 2.0 Patch: retun immediately upon the detection of a socket that is not open
	 */
	if ( ETH_SOCKET_BAD(socket)) return 0xFF;
	
	ETH_Send(ETH_SREG_SR,ETH_SOCKET_BASE(socket),0,&status,1);
	if (status != ETH_SR_INIT) {
		/*
		 * Error opening socket
		 */
		ETH_SocketClose(socket,0);
	}
	else if (ETH_ExecuteSocketCommand( socket, ETH_CR_LISTEN) != CYRET_SUCCESS) {
		ETH_SocketClose( socket, 0);
		socket = 0xFF;
	}
	
	return socket;
}
/* ------------------------------------------------------------------------ */
/**
 * \brief suspend operation while waiting for a connection to be established
 * \param socket (uint8) Socket handle for an open socket.
 *
 * _TcpWaitForConnection will poll the W5500 and wait until a connection has
 * been established.  Note that this is a BLOCKING call and will deadlock your
 * control loop if your application waits for long periods between connections.
 * Use _TcpConnected for non-blocking scans.
 * \sa _TcpConnected
 */ 
cystatus ETH_TcpWaitForConnection( uint8 socket )
{
	uint8 status;

	/*
	 * If the socket is invalid or not yet open, return a non-connect result
	 * to prevent calling functions and waiting for the timeout for sockets
	 * that are not yet open
	 */
	if (ETH_SOCKET_BAD(socket)) return CYRET_BAD_PARAM;
	/*
	 * Wait for the connectino to be established, or a timeout on the connection
	 * delay to occur.
	 */
	do {
		CyDelay(10);
		ETH_Send(ETH_SREG_SR,ETH_SOCKET_BASE(socket),0,&status, 1);
	}
	while ( status == ETH_SR_LISTEN );
		
	return CYRET_SUCCESS;
}

cystatus ETH_TcpPollSocket( uint8 socket)
{
	uint8 status;

	/*
	 * If the socket is invalid or not yet open, return a non-connect result
	 * to prevent calling functions and waiting for the timeout for sockets
	 * that are not yet open
	 */
    
	if (ETH_SOCKET_BAD(socket) ) return 0;
    
	/*
	 * Wait for the connectino to be established, or a timeout on the connection
	 * delay to occur.
	 */
	
	ETH_Send(ETH_SREG_SR,ETH_SOCKET_BASE(socket),0,&status, 1);

	return status;
}

cystatus ETH_TcpPollConnection( uint8* socket, uint16_t port )
{
	uint8 status;

	/*
	 * If the socket is invalid or not yet open, return a non-connect result
	 * to prevent calling functions and waiting for the timeout for sockets
	 * that are not yet open
	 */
    
	if (ETH_SOCKET_BAD(*socket)){
        *socket = ETH_TcpOpenServer(port);
        return CYRET_BAD_PARAM;
    }
    
	/*
	 * Wait for the connectino to be established, or a timeout on the connection
	 * delay to occur.
	 */
	
	ETH_Send(ETH_SREG_SR,ETH_SOCKET_BASE(*socket),0,&status, 1);
	switch (status){
        case ETH_SR_CLOSED:
            *socket = ETH_TcpOpenServer(port);
            return 0;
        break;
        case ETH_SR_CLOSE_WAIT:
            ETH_SocketDisconnect(*socket);
            return ETH_SR_FRESH_CLOSED;
        break;
        case ETH_SR_ESTABLISHED:
            return ETH_SR_ESTABLISHED;
        break;
    }
		
	return 0xFF;
}
/* ------------------------------------------------------------------------ */
/**
 * \brief Generic transmission of a data block using TCP
 * \param socket the socket that will be used to send the data
 * \param *buffer the array of data to be sent using TCP
 * \param len the length of data to send from the buffer
 * \param flags control flags for controlling options for transmission
 *
 * ETH_TcpSend transmits a block of generic data using TCP through a socket.
 * the connection must have been previously established in order for the
 * the function to operate properly, otherwise, no data will be transmitted.
 */
uint16 ETH_TcpSend( uint8 socket, uint8* buffer, uint16 len, uint8 flags)
{
	uint16 tx_length;
	uint16 max_packet;
	uint8 buf_size;
	uint16 ptr;
	uint8 result;
	
	if (ETH_SOCKET_BAD(socket) ) return 0;
	
	tx_length = ETH_TxBufferFree( socket );
	if ( (tx_length < len ) && ((flags&ETH_TXRX_FLG_WAIT) != 0) ) {
		/* 
		 * there is not enough room in the buffer, but the caller requested
		 * this to block until there was free space. So, check the memory
		 * size to determine if the tx buffer is big enough to handle the
		 * data block without fragmentation.
		 */
		ETH_Send(ETH_SREG_TXBUF_SIZE, ETH_SOCKET_BASE(socket),0,&buf_size,1);
		max_packet = (buf_size == 0)? 0 : (0x400 << (buf_size-1));
		/*
		 * now that we know the max buffer size, if it is smaller than the
		 * requested transmit lenght, we have an error, so return 0
		 */
		if (max_packet < len ) return 0;
		/* otherwise, we will wait for the room in the buffer */
		do {
			tx_length = ETH_TxBufferFree( socket );
		}
		while ( tx_length < len );
	}
	else {
		tx_length = len;
	}
	/*
	 * The length of the Tx data block has now been determined, and can be
	 * copied in to the W5500 buffer memory. First read the pointer, then
	 * write data from the pointer forward, lastly update the pointer and issue
	 * the SEND command.
	 */
	ETH_Send( ETH_SREG_TX_WR, ETH_SOCKET_BASE(socket),0,(uint8*)&ptr,2);
	ptr = CYSWAP_ENDIAN16( ptr );
	ETH_Send( ptr, ETH_TX_BASE(socket),1,buffer,tx_length);
	ptr += tx_length;
	ptr = CYSWAP_ENDIAN16( ptr );
	ETH_Send(ETH_SREG_TX_WR, ETH_SOCKET_BASE(socket),1,(uint8*)&ptr,2);
	
	ETH_ExecuteSocketCommand( socket, ETH_CR_SEND );
	
	if ( (flags & ETH_TXRX_FLG_WAIT) != 0) {
		/*
		 * block until send is complete
		 */
		do {
			CyDelay(1);
			result = ETH_SocketSendComplete(socket);
		}
		while( (result != CYRET_FINISHED) && (result != CYRET_CANCELED) );
	}
	
	return tx_length;
}
/* ------------------------------------------------------------------------ */
/**
 * \brief Send an ASCII String using TCP
 * \param socket the socket to use for sending the data
 * \param *string ASCII-Z String to send using TCP
 * \sa ETH_TcpSend
 * 
 * ETH_TcpPrint is a wrapper for the TcpSend function to simplify the
 * transmission of strings, and prompts over the open connection using TCP.
 */
void ETH_TcpPrint(uint8 socket, const char *string )
{
	uint16 length;
	
	length = strlen(string);
	ETH_TcpSend(socket, (uint8*) string,length, 0);
}
/* ------------------------------------------------------------------------ */
uint16 ETH_TcpReceive(uint8 socket, uint8* buffer, uint16 len, uint8 flags)
{
	uint16 rx_size;
	uint16 bytes;
	uint16 ptr;
	
	bytes = 0;
	/*
	 * when there is a bad socket, just return 0 bys no matter what.
	 */
	if ( ETH_SOCKET_BAD(socket) ) return 0;
	/*
	 * Otherwise, read the number of bytes waiting to be read.  When the byte
	 * count is less than the requested bytes, wait for them to be available
	 * when the wait flag is set, otherwise, just read the waiting data once.
	 */
	do {
		rx_size = ETH_RxDataReady( socket );
	}
	while ( (rx_size < len) && (flags&ETH_TXRX_FLG_WAIT) );
	
	/*
	 * When data is available, begin processing the data
	 */
	if (rx_size > 0) {
		/* 
		 * calculate the number of bytes to receive using the available data
		 * and the requested length of data.
		 */
		bytes = (rx_size > len) ? len : rx_size;
		/* Read the starting memory pointer address, and endian correct */
		ETH_Send( ETH_SREG_RX_RD, ETH_SOCKET_BASE(socket),0,(uint8*)&ptr,2);
		ptr = CYSWAP_ENDIAN16( ptr );
		/* Retrieve the data bytes from the W5500 buffer */
		ETH_Send( ptr, ETH_RX_BASE(socket),0,buffer,bytes);
		/* 
		 * Calculate the new buffer pointer location, endian correct, and
		 * update the pointer register within the W5500 socket registers
		 */
		ptr += bytes;
		ptr = CYSWAP_ENDIAN16( ptr );
		ETH_Send(ETH_SREG_RX_RD, ETH_SOCKET_BASE(socket),1,(uint8*)&ptr,2);
		/*
		 * when all of the available data was read from the message, execute
		 * the receive command
		 */
		ETH_ExecuteSocketCommand( socket, ETH_CR_RECV );
	}	
	return bytes;
}
/* ------------------------------------------------------------------------ */
char ETH_TcpGetChar( uint8 socket )
{
	char ch;
	uint16 len;
	
	do {
		len = ETH_TcpReceive(socket, (uint8*)&ch, 1, 0);
	}
	while (len < 1);
	return ch;
}
/* ------------------------------------------------------------------------ */
int ETH_TcpGetLine( uint8 socket, char *buffer )
{
	char ch;
	int idx;
	
	idx = 0;
	do {
		ch = ETH_TcpGetChar( socket );
		if ((ch != '\r') && (ch!='\n') ) {
			if ( (ch == '\b')||(ch==127) ) {
				buffer[idx] = 0;
				idx = (idx == 0)?0:idx-1;
			}
			else {
				buffer[idx++] = ch;
				buffer[idx] = 0;
			}
		}
	}
	while ( (ch!='\r')&&(ch!='\n'));
	buffer[idx] = 0;
	
	return idx;
}
/* ======================================================================== */
/** @} */
/* [] END OF FILE */
