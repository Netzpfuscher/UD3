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

#if !defined(tsk_sid_TASK_H)
#define tsk_sid_TASK_H
    
#define N_SIDCHANNEL 3

/*
 * Add user task definitions, types, includes and other things in the below
 * merge region to customize the task.
 */
/* `#START USER_TYPES_AND_DEFINES` */
#include <device.h>
    
    /* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
    
#include "cli_basic.h"
#include "TTerm.h"
#include "SignalGenerator.h"

/* `#END` */

void tsk_sid_Start(void);

void tsk_sid_reset_skip();

//WARNING: only 16 bits available for this. Keep that in mind when adding flags
#define SID_FRAME_FLAG_CH3OFF   0x100
#define SID_FRAME_FLAG_GATE     0x80
#define SID_FRAME_FLAG_SYNC     0x40
#define SID_FRAME_FLAG_RING     0x20
#define SID_FRAME_FLAG_TEST     0x10
#define SID_FRAME_FLAG_TRIANGLE 0x08
#define SID_FRAME_FLAG_SAWTOOTH 0x04
#define SID_FRAME_FLAG_SQUARE   0x02
#define SID_FRAME_FLAG_NOISE    0x01   //noice
    
typedef enum {ADSR_IDLE, ADSR_ATTACK, ADSR_DECAY, ADSR_SUSTAIN, ADSR_RELEASE} ADSRState_t;

//contains one set of sid register options
typedef struct{
    uint16_t flags[N_SIDCHANNEL];
    
    uint16_t attack[N_SIDCHANNEL];
    uint16_t decay[N_SIDCHANNEL];
    uint16_t sustain[N_SIDCHANNEL];
    uint16_t release[N_SIDCHANNEL];
    
    uint32_t frequency_dHz[N_SIDCHANNEL];
    uint32_t pulsewidth[N_SIDCHANNEL];
    
    //TODO add filter stuff?
    
    //uint16_t master_pw; //TODO: whats this used for?
    uint32_t next_frame;
} SIDFrame_t;
    
//contains one set of sid register options
typedef struct{
    ADSRState_t adsrState;
    
    uint32_t flags;
    
    uint16_t attack;
    uint16_t decay;
    uint16_t sustainVolume;
    uint16_t release;
    
    uint32_t frequency_dHz;
    uint32_t pulsewidth;
    
    uint32_t currentEnvelopeFactor;
    uint32_t currentEnvelopeStartValue;
    uint32_t currentEnvelopeVolume;
} SIDChannelData_t;
    
//contains one set of sid register options
typedef struct{
    //channel specific filters
    uint32_t channelVolume[N_SIDCHANNEL];
    uint32_t hpvEnabled[N_SIDCHANNEL];
    
    //global filter parameters
    uint32_t flags;
    
    uint32_t noiseVolume;
    uint32_t hpvEnabledGlobally;
} SIDFilterData_t;


    
extern xQueueHandle qSID;
extern SIDFilterData_t SID_filterData;


/*
 * Add user function prototypes in the below merge region to add user
 * functionality to the task definition.
 */
/* `#START USER_TASK_PROTOS` */

/* `#END` */

/* ------------------------------------------------------------------------ */
#endif
/* [] END OF FILE */
