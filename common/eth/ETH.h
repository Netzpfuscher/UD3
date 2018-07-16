#if !defined(ETH_H)
	#define ETH_H
/* ------------------------------------------------------------------------ */
/**
 * \defgroup e2forlife_w5500 W5500 Device Driver for PSoC
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
 * \file w5500.h
 * \author Chuck Erhardt (chuck@e2forlife.com)
 * 
 * Driver configuration, function prototypes, and chip register defines for
 * accessing the iW5500.
 */
/* ======================================================================== */

#include <cytypes.h>


/* Calculated memory size parameter to allocate more socket mem when using fewer sockets */
#define ETH_SOCKET_MEM         ( 2 )
/* Chip Configuration. Set to the maximum memory available for socket data */
#define ETH_CHIP_MEM           ( 16 )
#define ETH_CHIP_SOCKETS       ( 8 )
/* Chip Configuration (Calculated). This contains the calculated MAX sockets based on available memory */
#define ETH_MAX_SOCKETS        ( ETH_CHIP_MEM / ETH_SOCKET_MEM )
	
#define ETH_SOCKET_BAD(x)      ( (x >= ETH_MAX_SOCKETS)||(ETH_socketStatus[x] == ETH_SOCKET_AVAILALE) )


/* ------------------------------------------------------------------------ */
#define ETH_SOCKET_OPEN           ( 1 )
#define ETH_SOCKET_AVAILALE       ( 0 )

/* ------------------------------------------------------------------------ */
/* Macros & Function Defines */
/*
 * W5500 Register Map
 */
#define ETH_REG_MODE            ( 0x0000 )
#define ETH_REG_GAR             ( 0x0001 )
#define ETH_REG_SUBR            ( 0x0005 )
#define ETH_REG_SHAR            ( 0x0009 )
#define ETH_REG_SIPR            ( 0x000F )
#define ETH_REG_INTLEVEL        ( 0x0013 )
#define ETH_REG_IR              ( 0x0015 )
#define ETH_REG_IMR             ( 0x0016 )
#define ETH_REG_SIR             ( 0x0017 )
#define ETH_REG_SIMR            ( 0x0018 )
#define ETH_REG_RTR             ( 0x0019 )
#define ETH_REG_RCR             ( 0x001B )
#define ETH_REG_PTIMER          ( 0x001C )
#define ETH_REG_PMAGIC          ( 0x001D )
#define ETH_REG_PHAR            ( 0x001E )
#define ETH_REG_PSID            ( 0x0024 )
#define ETH_REG_PMRU            ( 0x0026 )
#define ETH_REG_UIPR            ( 0x0028 )
#define ETH_REG_UPORTR          ( 0x002C )
#define ETH_REG_PHYCFGR         ( 0x002E )
#define ETH_REG_VERSIONR        ( 0x0039 )

/* Socket Registers */
#define ETH_SREG_MR             ( 0x0000 )
#define ETH_SREG_CR             ( 0x0001 )
#define ETH_SREG_IR             ( 0x0002 )
#define ETH_SREG_SR             ( 0x0003 )
#define ETH_SREG_PORT           ( 0x0004 )
#define ETH_SREG_DHAR           ( 0x0006 )
#define ETH_SREG_DIPR           ( 0x000C )
#define ETH_SREG_DPORT          ( 0x0010 )
#define ETH_SREG_MSSR           ( 0x0012 )
#define ETH_SREG_TOS            ( 0x0015 )
#define ETH_SREG_TTL            ( 0x0016 )
#define ETH_SREG_RXBUF_SIZE     ( 0x001E )
#define ETH_SREG_TXBUF_SIZE     ( 0x001F )
#define ETH_SREG_TX_FSR         ( 0x0020 )
#define ETH_SREG_TX_RD          ( 0x0022 )
#define ETH_SREG_TX_WR          ( 0x0024 )
#define ETH_SREG_RX_RSR         ( 0x0026 ) 
#define ETH_SREG_RX_RD          ( 0x0028 )
#define ETH_SREG_RX_WR          ( 0x002A )
#define ETH_SREG_IMR            ( 0x002C )
#define ETH_FRAG                ( 0x002D )
#define ETH_KPALVTR             ( 0x002F )

/*
 * W5500 Commands
 */
#define ETH_CR_OPEN            ( 0x01 )
#define ETH_CR_LISTEN          ( 0x02 )
#define ETH_CR_CONNECT         ( 0x04 )
#define ETH_CR_DISCON          ( 0x08 )
#define ETH_CR_CLOSE           ( 0x10 )
#define ETH_CR_SEND            ( 0x20 )
#define ETH_CR_SEND_MAC        ( 0x21 )
#define ETH_CR_SEND_KEEP       ( 0x22 )
#define ETH_CR_RECV            ( 0x40 )

/*
 * W5500 Interrupt Flags
 */
#define ETH_IR_CON              ( 0x01 )
#define ETH_IR_DISCON           ( 0x02 )
#define ETH_IR_RECV             ( 0x04 )
#define ETH_IR_TIMEOUT          ( 0x08 )
#define ETH_IR_SEND_OK          ( 0x10 )

/*
 * Socket Status
 */
#define ETH_SR_CLOSED         ( 0x00 )
#define ETH_SR_INIT           ( 0x13 )
#define ETH_SR_LISTEN         ( 0x14 )
#define ETH_SR_ESTABLISHED    ( 0x17 )
#define ETH_SR_CLOSE_WAIT     ( 0x1C )
#define ETH_SR_UDP            ( 0x22 )
#define ETH_SR_MACRAW         ( 0x42 )
#define ETH_SR_FRESH_CLOSED   ( 0x01 )

/*
 * timeout used to prevent command acceptance deadlock
 */
#define ETH_CMD_TIMEOUT         ( 125 )

/* Block select regions of the W5500 */
#define ETH_BLOCK_COMMON        (0x00)
#define ETH_BLOCK_S0_REG        (0x08)
#define ETH_BLOCK_S0_TXB        (0x10)
#define ETH_BLOCK_S0_RXB        (0x18)
#define ETH_BLOCK_S1_REG        (0x28)
#define ETH_BLOCK_S1_TXB        (0x30)
#define ETH_BLOCK_S1_RXB        (0x38)
#define ETH_BLOCK_S2_REG        (0x48)
#define ETH_BLOCK_S2_TXB        (0x50)
#define ETH_BLOCK_S2_RXB        (0x58)
#define ETH_BLOCK_S3_REG        (0x68)
#define ETH_BLOCK_S3_TXB        (0x70)
#define ETH_BLOCK_S3_RXB        (0x78)
#define ETH_BLOCK_S4_REG        (0x88)
#define ETH_BLOCK_S4_TXB        (0x90)
#define ETH_BLOCK_S4_RXB        (0x98)
#define ETH_BLOCK_S5_REG        (0xA8)
#define ETH_BLOCK_S5_TXB        (0xB0)
#define ETH_BLOCK_S5_RXB        (0xB8)
#define ETH_BLOCK_S6_REG        (0xC8)
#define ETH_BLOCK_S6_TXB        (0xD0)
#define ETH_BLOCK_S6_RXB        (0xD8)
#define ETH_BLOCK_S7_REG        (0xE8)
#define ETH_BLOCK_S7_TXB        (0xF0)
#define ETH_BLOCK_S7_RXB        (0xF8)

/* 
 * MACRO functions to calculate the socket region block selects
 * from the socket number.  These are helpful when working with
 * the socket based commands.
 */
#define ETH_SOCKET_BASE(s)                     (( (s*4) + 1 )<<3)
#define ETH_TX_BASE(s)                         (( (s*4) + 2 )<<3)
#define ETH_RX_BASE(s)                         (( (s*4) + 3 )<<3)

/* Socket Configuration FLags */
#define ETH_FLG_SKT_UDP_MULTICAST_ENABLE       ( 0x80 )
#define ETH_FLG_SKT_MACRAW_MAC_FILT_ENABLE     ( 0x80 )
#define ETH_FLG_SKT_BLOCK_BROADCAST            ( 0x40 )
#define ETH_FLG_SKT_ND_ACK                     ( 0x20 )
#define ETH_FLG_SKT_UDP_IGMP_V1                ( 0x20 )
#define ETH_FLG_SKT_MACRAW_MULTICAST_BLOCK     ( 0x20 )
#define ETH_FLG_SKT_UDP_BLOCK_UNICAST          ( 0x10 )
#define ETH_FLG_SKT_MACRAW_IPV6_BLOCKING       ( 0x10 )
/* Socket protocol configurations */
#define ETH_PROTO_CLOSED                       ( 0x00 )
#define ETH_PROTO_TCP                          ( 0x01 )
#define ETH_PROTO_UDP                          ( 0x02 )
#define ETH_PROTO_MACRAW                       ( 0x04 )

#define ETH_TXRX_FLG_WAIT                      ( 0x01 )

//#define ETH_IPADDRESS(x1,x2,x3,x4)   ( (uint32)(x1&0x000000FF) + (uint32)((x2<<8)&0x0000FF00) + (uint32)((x3<<16)&0x00FF0000) + ((uint32)(x4<<24)&0xFF000000 ))
	
/* ------------------------------------------------------------------------ */
void ETH_Send(uint16 offset, uint8 block_select, uint8 write, uint8 *buffer, uint16 len);

cystatus ETH_Start( void );
cystatus ETH_StartEx( const char *gateway, const char *subnet, const char *mac, const char *ip );
cystatus ETH_Init( uint8* gateway, uint8* subnet, uint8 *mac, uint8* ip );
void ETH_GetMac(uint8* mac);
uint32 ETH_GetIp( void );
uint16 ETH_GetTxLength(uint8 socket, uint16 len, uint8 flags);
cystatus ETH_WriteTxData(uint8 socket, uint8 *buffer, uint16 tx_length, uint8 flags);

uint16 ETH_RxDataReady( uint8 socket );
uint16 ETH_TxBufferFree( uint8 socket );
uint8 ETH_SocketOpen( uint16 port, uint8 flags );
cystatus ETH_SocketClose( uint8 sock, uint8 discon );
cystatus ETH_SocketDisconnect( uint8 sock );
cystatus ETH_SocketSendComplete( uint8 socket );
cystatus ETH_ExecuteSocketCommand(uint8 socket, uint8 cmd );

cystatus ETH_TcpConnected( uint8 sock );
uint8 ETH_TcpOpenClient( uint16 port, uint32 remote_ip, uint16 remote_port );
uint8 ETH_TcpOpenServer(uint16 port);
cystatus ETH_TcpWaitForConnection( uint8 socket );
uint16 ETH_TcpSend( uint8 socket, uint8* buffer, uint16 len, uint8 flags);
void ETH_TcpPrint(uint8 socket, const char *string );
uint16 ETH_TcpReceive(uint8 socket, uint8* buffer, uint16 len, uint8 flags);
char ETH_TcpGetChar( uint8 socket );
int ETH_TcpGetLine( uint8 socket, char *buffer );
cystatus ETH_TcpPollConnection( uint8* socket, uint16_t port );
cystatus ETH_TcpPollSocket( uint8 socket);

uint8 ETH_UdpOpen( uint16 port );
uint16 ETH_UdpSend(uint8 socket, uint32 ip, uint16 port, uint8 *buffer, uint16 len, uint8 flags);
uint16 ETH_UdpReceive(uint8 socket, uint8 *header, uint8 *buffer, uint16 len, uint8 flags);


uint32 ETH_ParseIP( const char* ipString );
cystatus ETH_ParseMAC(const char *macString, uint8 *mac);
void ETH_StringMAC(uint8 *mac, char *macString);
void ETH_StringIP( uint32 ip, char *ipString );
int ETH_Base64Encode(const void* data_buf, int dataLength, char* result, int resultSize);
int ETH_Base64Decode (char *in, int inLen, uint8 *out, int *outLen);
uint32 ETH_IPADDRESS(uint8 x1, uint8 x2, uint8 x3, uint8 x4 );

#endif
/** @} */
/* [] END OF FILE */
