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

/**
 * @file tsk_sid.h
 * @brief SID chip emulation task
 *
 * Emulates Commodore 64 SID (Sound Interface Device) chip for music playback.
 * Supports 3 voices with ADSR envelopes and multiple waveforms.
 */

#if !defined(tsk_sid_TASK_H)
#define tsk_sid_TASK_H

#define N_SIDCHANNEL 3 //!< Number of SID voices (3 like original SID chip)

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

/**
 * @brief Start the SID emulation task
 */
void tsk_sid_Start(void);

/**
 * @brief Reset the SID frame skip counter
 */
void tsk_sid_reset_skip();

/** @name SID Frame Flags
 * Control flags for SID waveform generation (16-bit maximum)
 * @{
 */
#define SID_FRAME_FLAG_CH3OFF 0x100   //!< Disable channel 3
#define SID_FRAME_FLAG_GATE 0x80      //!< Gate signal (start/stop note)
#define SID_FRAME_FLAG_SYNC 0x40      //!< Oscillator sync
#define SID_FRAME_FLAG_RING 0x20      //!< Ring modulation
#define SID_FRAME_FLAG_TEST 0x10      //!< Test bit (halt oscillator)
#define SID_FRAME_FLAG_TRIANGLE 0x08  //!< Triangle waveform
#define SID_FRAME_FLAG_SAWTOOTH 0x04  //!< Sawtooth waveform
#define SID_FRAME_FLAG_SQUARE 0x02    //!< Square waveform
#define SID_FRAME_FLAG_NOISE 0x01     //!< Noise waveform
/** @} */

/**
 * @brief ADSR envelope state machine states
 */
typedef enum { ADSR_IDLE, ADSR_ATTACK, ADSR_DECAY, ADSR_SUSTAIN, ADSR_RELEASE } ADSRState_t;

/**
 * @brief SID frame data structure
 *
 * Contains all register data for one frame of SID output.
 * Sent via queue to SID task for processing.
 */
typedef struct {
	uint16_t flags[N_SIDCHANNEL];          //!< Control flags per channel
	uint16_t attack[N_SIDCHANNEL];         //!< Attack time per channel
	uint16_t decay[N_SIDCHANNEL];          //!< Decay time per channel
	uint16_t sustain[N_SIDCHANNEL];        //!< Sustain level per channel
	uint16_t release[N_SIDCHANNEL];        //!< Release time per channel
	uint32_t frequency_dHz[N_SIDCHANNEL];  //!< Frequency in decihertz per channel
	uint32_t pulsewidth[N_SIDCHANNEL];     //!< Pulse width per channel
	uint32_t next_frame;                   //!< Timestamp for next frame
} SIDFrame_t;

/**
 * @brief SID channel runtime data
 *
 * Tracks ADSR envelope state and current parameters for one SID voice.
 */
typedef struct {
	ADSRState_t adsrState;             //!< Current ADSR state
	uint32_t flags;                    //!< Waveform and control flags
	uint16_t attack;                   //!< Attack time
	uint16_t decay;                    //!< Decay time
	uint16_t sustainVolume;            //!< Sustain level
	uint16_t release;                  //!< Release time
	uint32_t frequency_dHz;            //!< Frequency in decihertz
	uint32_t pulsewidth;               //!< Pulse width
	uint32_t currentEnvelopeFactor;    //!< Envelope interpolation factor
	uint32_t currentEnvelopeStartValue; //!< Envelope start value for current phase
	uint32_t currentEnvelopeVolume;    //!< Current envelope volume
} SIDChannelData_t;

/**
 * @brief SID filter configuration data
 *
 * Controls per-channel and global filtering parameters.
 */
typedef struct {
	uint32_t channelVolume[N_SIDCHANNEL]; //!< Volume per channel
	uint32_t hpvEnabled[N_SIDCHANNEL];    //!< High-pass filter enabled per channel
	uint32_t flags;                       //!< Global filter flags
	uint32_t noiseVolume;                 //!< Noise channel volume
	uint32_t hpvEnabledGlobally;          //!< Global high-pass filter enable
} SIDFilterData_t;

extern xQueueHandle qSID;                 //!< Queue for incoming SID frames
extern SIDFilterData_t SID_filterData;    //!< Global SID filter configuration


/*
 * Add user function prototypes in the below merge region to add user
 * functionality to the task definition.
 */
/* `#START USER_TASK_PROTOS` */

/* `#END` */

/* ------------------------------------------------------------------------ */
#endif
/* [] END OF FILE */
