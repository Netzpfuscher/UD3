#ifndef USBMIDI_1_H
#define USBMIDI_1_H

#include <stdint.h>

extern uint8_t USBMIDI_1_initVar;

#define USBMIDI_1_5V_OPERATION 5
#define USBMIDI_1_Start(p1, p2)
#define USBMIDI_1_CDC_Init()
#define USBMIDI_1_MIDI_Init()
#define USBMIDI_1_MIDI_OUT_Service()

uint8_t USBMIDI_1_IsConfigurationChanged();
uint8_t USBMIDI_1_GetConfiguration();
void USBMIDI_1_PutData(uint8_t * buffer, uint16_t count);
uint8_t USBMIDI_1_CDCIsReady();

uint8_t USBMIDI_1_DataIsReady();
uint16_t USBMIDI_1_GetAll(uint8_t * buffer);


#endif