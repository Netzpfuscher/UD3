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


//Synth constants
#define SID_NOISE_WEIGHT 0x03
    
    
//FreeRTOS
    
#define ACTIVATE_TASK_INFO 0
#define HEAP_SIZE 50    //kb
    
    
//MIN
#define NUM_MIN_CON 4
#define STREAMBUFFER_RX_SIZE    256     //bytes
#define STREAMBUFFER_TX_SIZE    1024    //bytes  
    
//Alarm-Event
#define AE_QUEUE_SIZE 50
    
    
#endif