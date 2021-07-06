#ifndef UART_H
#define UART_H

#include <stdint.h>


void UART_Start();

#define UART_TX_BUFFER_SIZE 1024
#define UART_RX_BUFFER_SIZE 1024

uint32_t UART_GetTxBufferSize();
uint32_t UART_GetRxBufferSize();
void UART_PutChar(uint8_t byte);
uint8_t UART_GetByte();

void UART_start_frame();
void UART_end_frame();

#define uart_rx_StartEx(p1)
#define uart_tx_StartEx(p1)
#define UART_Stop()
#define UART_CLK_SetDividerValue(divider_selected)



#endif