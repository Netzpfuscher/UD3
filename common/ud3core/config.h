#if !defined(config_H)
#define config_H

//Timing constants
#define MIDI_ISR_Hz 8000
#define SG_CLOCK_Hz 320000


#define MIDI_ISR_US (1e6 / MIDI_ISR_Hz)
#define SG_PERIOD_NS (1e9 / SG_CLOCK_Hz)
#define SG_CLOCK_HALFCOUNT (SG_CLOCK_Hz/2)

//Ramp constants
#define RAMP_CHART_HEIGHT 255
#define RAMP_CHART_WIDTH 400
#define RAMP_CHART_OFFSET_X 40
#define RAMP_CHART_OFFSET_Y 20
#define RAMP_CHART_DIV_X 25
#define RAMP_CHART_DIV_Y 25


//FreeRTOS
#define ACTIVATE_TASK_INFO 1

#ifndef SIMULATOR
#define HEAP_SIZE 48    //kb
#else
#define HEAP_SIZE 512    //kb	
#endif
    
//MIN
#define NUM_MIN_CON 4
#define STREAMBUFFER_RX_SIZE    256     //bytes
#define STREAMBUFFER_TX_SIZE    512    //bytes  
    
//Alarm-Event
#define AE_QUEUE_SIZE       50          //Length of the alarm and event queue
    
//Analog-Task
#define ADC_BUFFER_CNT      25          //ADC DMA buffer
#define ADC_SAMPLE_CLK      32000       //Hz
#define NEW_DATA_RATE_MS    ((1.0/(ADC_SAMPLE_CLK/4)) * ADC_BUFFER_CNT)    //ms
#define CURRENT_PID_HZ      ((uint16_t)(1.0 / NEW_DATA_RATE_MS))           //Hz
#define SAMPLES_COUNT       2048             //How many samples for RMS filter
#define BUSV_R_BOT          5000UL           //Bus voltage bottom resistor
#define DRIVEV_R_TOP        10000UL          //Drive voltage top resistor
#define DRIVEV_R_BOT        2200UL           //Drive voltage bottom resistor
#define DRIVEV_R_BOT_V3     1000UL           //Drive voltage bottom resistor (for UD3.0)
    
//Synthesizer
#define N_QUEUE_SID     64          //Size of the SID frame buffer
#define N_QUEUE_MIDI    64         //Size of the midi event buffer
#define N_CHANNEL 4                 //Number of parallel voices

#define RELAY1_INVERTED 0
#define RELAY2_INVERTED 0

#define LED_ON  (configuration.ivo_led ? 0 : 1)
#define LED_OFF (configuration.ivo_led ? 1 : 0)

#define IVO_UART_RX_BIT  0
#define IVO_UART_TX_BIT  1
#define IVO_LED_BIT      2


    
#endif