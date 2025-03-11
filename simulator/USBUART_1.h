#ifndef USBUART_1_H
#define USBUART_1_H

#include <stdint.h>

extern uint8_t USBUART_1_initVar;

#define USBUART_1_5V_OPERATION 5
#define USBUART_1_Start(p1, p2)
#define USBUART_1_CDC_Init()

uint8_t USBUART_1_IsConfigurationChanged();
uint8_t USBUART_1_GetConfiguration();
void USBUART_1_PutData(uint8_t * buffer, uint16_t count);
uint8_t USBUART_1_CDCIsReady();

uint8_t USBUART_1_DataIsReady();
uint16_t USBUART_1_GetAll(uint8_t * buffer);


#endif
