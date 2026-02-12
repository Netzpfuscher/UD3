/**
 * @file SignalGenerator.h
 * @brief Polyphonic pulse generator with voice management and modulation
 *
 * The SignalGenerator module is the heart of UD3's audio synthesis system, providing:
 * - 6-voice polyphonic synthesis (MIDI, SID, VMS audio engines)
 * - TR (transient) mode for manual pulse generation
 * - QCW mode pulse scheduling (long pulses with ramped envelopes)
 * - Hypervoice support (multiple sub-pulses per period for harmonic content)
 * - Thread-safe ring buffer for pulse queuing (128 entries)
 * - Real-time limiting based on duty cycle, current, and safety interlocks
 * - Burst mode for rhythmic patterns (on/off timing control)
 *
 * Architecture:
 * - SigGen_task() runs at 8kHz (MIDI_ISR_Hz), generates pulses from voice states
 * - Pulse buffer queues pulses for hardware consumption via ISR timer
 * - Volume scaling: INT16_MAX (internal) → DAC counts [min_tr_cl_dac_val, max_tr_cl_dac_val]
 * - Hypervoice divides note period into multiple sub-pulses with phase offsets
 * - Master volume control applies global gain to all voices
 *
 * Copyright (c) 2021 Jens Kerrinnes
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

#if !defined(SignalGenerator_H)
#define SignalGenerator_H
    
    
    #include <stdint.h>
    #include "config.h"
    #include "FreeRTOS.h"
    #include "TTerm.h"
    #include "cli_basic.h"
    
    /**
     * @brief Synthesis modes for signal generator operation
     */
    enum SYNTH{
        SYNTH_OFF=0,        /**< No synthesis, outputs disabled */
        SYNTH_MIDI=1,       /**< MIDI polyphonic mode (VMS engine, transient pulses) */
        SYNTH_SID=2,        /**< SID chip emulation mode (6581/8580, transient pulses) */
        SYNTH_TR=3,         /**< Manual transient mode (direct pulse control) */
        SYNTH_MIDI_QCW=4,   /**< MIDI with QCW long pulses (ramped envelopes) */
        SYNTH_SID_QCW=5,    /**< SID with QCW long pulses (ramped envelopes) */
    };

    #include "RingBuffer/include/RingBuffer.h"

    /** @brief SignalGenerator version for compatibility checking */
    #define SIGGEN_VERSION 1

    /**
     * @brief Maximum achievable on-time in microseconds
     *
     * Limited to 10900µs to prevent overflow in calculations (max safe value 65535).
     * Actual maximum may be further limited by duty cycle, current limits, and feedback.
     */
    #define SIGGEN_MAXOT 10900

    /** @brief Number of output channels (currently single coil support) */
    #define SIGGEN_OUTPUTCOUNT 1
    /** @brief Number of polyphonic voices (6-voice synthesis) */
    #define SIGGEN_VOICECOUNT 6

    /** @brief Size of pulse ring buffer (power of 2 for efficient masking) */
    #define SIGGEN_PULSEBUFFER_SIZE 128
    /** @brief Minimum allowed on-time in microseconds (hardware limitation) */
    #define SIGGEN_MIN_OT 10    //TODO evaluate this... maybe make it bigger
    /** @brief Minimum allowed period in microseconds (frequency limit) */
    #define SIGGEN_MIN_PERIOD 1000    //TODO evaluate this...

    /** @brief Parameter index for right auxiliary output mode */
    #define SIGGEN_PARAM_AUX_R_MODE 0
    /** @brief Default right aux mode: audio output */
    #define SIGGEN_DEFAULT_PARAM_AUX_R_MODE AUXMODE_AUDIO_OUT
    /** @brief Parameter index for left auxiliary output mode */
    #define SIGGEN_PARAM_AUX_L_MODE 1
    /** @brief Default left aux mode: audio output */
    #define SIGGEN_DEFAULT_PARAM_AUX_L_MODE AUXMODE_AUDIO_OUT

    /**
     * @brief Per-voice state for polyphonic synthesis
     *
     * Tracks timing, parameters, and hypervoice state for one synthesis voice.
     * Modified by audio engines (MIDI/SID/VMS) and SigGen_task(), read by pulse ISR.
     */
    typedef struct{
        /* ===== Time References ===== */
        int32_t counter;            /**< Phase accumulator in task ticks (8kHz), wraps at period */
        int32_t burstCounter;       /**< Burst mode counter in task ticks */
        int32_t currHPVCounter;     /**< Current hypervoice sub-pulse counter */
        int32_t currHPVDivider;     /**< Hypervoice period divider (period / hpvCount) */
        
        int32_t period;             /**< Note period in task ticks (8kHz), unlimited */
        int32_t limitedPeriod;      /**< Note period after min period limiting */
        
        int32_t noteAge;            /**< Ticks since note-on (for envelope/aging effects) */
        int32_t hpvOffset;          /**< Phase offset for hypervoice sub-pulses */
        int32_t hpvCount;           /**< Number of hypervoice sub-pulses per period */
        int32_t limitedHpvCount;    /**< Hypervoice count after limiting (duty/safety) */
        
        /* ===== Flags ===== */
        int32_t enabled;            /**< Voice enabled (1) or silent (0) */
        int32_t limitedEnabled;     /**< Voice enabled after safety/limit checks */
        int32_t noiseAmplitude;     /**< White noise amplitude (SID noise waveform) */
        int32_t alreadyTriggered;   /**< Flag to prevent double-triggering in same period */
        
        /* ===== Pulse Parameters ===== */
        int32_t frequencyTenths;    /**< Frequency in tenths of Hz (e.g., 4400 = 440.0 Hz) */
        int32_t hpvPhase;           /**< Phase offset for hypervoice sub-pulses (0-255) */
        
        int32_t hpvVolume;          /**< Hypervoice volume (0 to INT16_MAX) */
        int32_t pulseVolume;        /**< Main pulse volume (0 to INT16_MAX) */
        
        int32_t initialPulseVolume; /**< Volume at note-on (for envelope tracking) */
        int32_t initialHpvVolume;   /**< Hypervoice volume at note-on */
        
        int32_t pulseWidth_us;      /**< Requested pulse width in microseconds */
        int32_t limitedPulseWidth_us; /**< Pulse width after safety limiting */
        
        int32_t hpvPulseWidth_us;   /**< Hypervoice pulse width in microseconds */
        int32_t limitedHpvPulseWidth_us; /**< Hypervoice pulse width after limiting */
        
        /* ===== Burst Parameters ===== */
        int32_t burstPeriod;        /**< Burst on+off period in task ticks (0 = disabled) */
        int32_t burstOt;            /**< Burst on-time in task ticks */
    } SigGen_voiceData_t;

    /**
     * @brief Single pulse descriptor for ring buffer queuing
     *
     * Units change depending on context:
     * - Before queuing: period/onTime in microseconds, current in siggen volume (0-INT16_MAX)
     * - In pulse buffer: period/onTime in timer clock counts, current in DAC counts
     *
     * Thread-safe when accessed through ring buffer primitives.
     */
    typedef struct{
        volatile int32_t period;    /**< Pulse period: µs (pre-queue) or timer counts (in buffer) */
        volatile int32_t onTime;    /**< On-time: µs (pre-queue) or timer counts (in buffer) */
        volatile int32_t current;   /**< Volume: 0-INT16_MAX (pre-queue) or DAC counts (in buffer) */
    } volatile SigGen_pulseData_t;

    /**
     * @brief Global task state for signal generator
     *
     * Central data structure containing all voices, pulse buffer, and TR mode state.
     * Accessed by SigGen_task() (8kHz), audio engines, and pulse ISR.
     */
    typedef struct{
        SigGen_voiceData_t voice[SIGGEN_VOICECOUNT]; /**< Array of 6 polyphonic voices */
        
        /* ===== Buffering ===== */
        volatile int32_t bufferLengthInCounts; /**< Target pulse buffer fill level in timer counts */
        volatile RingBuffer_t * pulseBuffer;   /**< Ring buffer for pulse queue (128 entries) */
        volatile uint32_t pulseFlopFlip;       /**< Flip-flop state for pulse alternation logic */
        
        /* ===== TR Mode Burst State ===== */
        uint32_t trBurstCounter; /**< TR burst mode counter in task ticks */
        uint32_t trBurstState;   /**< TR burst state: 0=off period, 1=on period */
        uint32_t trBurstOnTime;  /**< TR burst on-time in task ticks */
        uint32_t trBurstOffTime; /**< TR burst off-time in task ticks */
        
    } volatile SigGen_taskData_t;
    
    /** @brief Per-voice status flags for UI feedback (6-element array) */
    extern uint32_t VoiceFlags[];

    /**
     * @brief Auxiliary output modes for AUX_L/AUX_R channels
     */
    typedef enum {
        AUXMODE_AUDIO_OUT, /**< Output audio signal to auxiliary channels */
        AUXMODE_E_STOP     /**< Emergency stop output mode */
    } AuxMode_t;

    /* ===== Initialization ===== */
    
    /**
     * @brief Initialize signal generator and create FreeRTOS task
     *
     * Sets up:
     * - Pulse ring buffer (128 entries)
     * - Task data structure
     * - FreeRTOS task (8kHz periodic execution)
     * - Hardware timer ISR for pulse output
     */
    void SigGen_init();

    /* ===== Voice Status Query ===== */
    
    /**
     * @brief Check if a voice is currently active
     * @param voice Voice index (0-5)
     * @return 1 if voice enabled and producing output, 0 otherwise
     */
    uint32_t SigGen_isVoiceOn(uint32_t voice);

    /**
     * @brief Get current duty cycle across all voices
     * @return Duty cycle as percentage * 10 (e.g., 125 = 12.5%)
     */
    uint32_t SigGen_getCurrDuty();

    /* ===== Volume Conversion Utilities ===== */
    
    /**
     * @brief Convert on-time (µs) to internal volume (0-INT16_MAX)
     * @param ontime On-time in microseconds
     * @return Volume in range [0, INT16_MAX]
     */
    uint32_t SigGen_otToVolume(uint32_t ontime);
    
    /**
     * @brief Convert internal volume to on-time (µs)
     * @param volume Volume in range [0, INT16_MAX]
     * @return On-time in microseconds
     */
    uint32_t SigGen_volumeToOT(uint32_t volume);
    
    /**
     * @brief Parameter change callback for siggen config
     * @param params Parameter table
     * @param index Index of changed parameter
     * @param handle Terminal handle for error messages
     * @return pdTRUE to accept change, pdFALSE to reject
     */
    uint8_t callback_siggen(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);

    /* ===== Global Controls ===== */
    
    /**
     * @brief Set master volume (global gain applied to all voices)
     * @param newVolume Master volume (0-INT16_MAX, typically 0-100 from UI)
     */
    void SigGen_setMasterVol(uint32_t newVolume);
    
    /**
     * @brief Switch synthesis mode (MIDI/SID/TR/QCW variants)
     * @param newMode New mode from enum SYNTH
     */
    void SigGen_switchSynthMode(uint8_t newMode);
    
    /* ===== TR Mode Voice Control ===== */
    
    /**
     * @brief Set TR (transient) mode voice parameters
     * @param enabled Enable (1) or disable (0) TR voice
     * @param pulseWidth Pulse width in microseconds
     * @param volume Volume (0-INT16_MAX)
     * @param frequencyTenths Frequency in tenths of Hz (e.g., 4400 = 440.0 Hz)
     * @param burstOn_us Burst on-time in microseconds (0 = no burst)
     * @param burstOff_us Burst off-time in microseconds
     */
    void SigGen_setVoiceTR(uint32_t enabled, int32_t pulseWidth, int32_t volume, int32_t frequencyTenths, int32_t burstOn_us, int32_t burstOff_us);
    
    /* ===== SID Mode Voice Control ===== */
    
    /**
     * @brief Set SID voice hypervoice parameters (sub-pulses for harmonics)
     * @param voice Voice index (0-5)
     * @param count Number of hypervoice sub-pulses per period
     * @param pulseWidth Hypervoice pulse width in microseconds
     * @param volume Hypervoice volume (0-INT16_MAX)
     * @param phase Phase offset for sub-pulses (0-255)
     */
    void SigGen_setHyperVoiceSID(uint32_t voice, uint32_t count, uint32_t pulseWidth, uint32_t volume, uint32_t phase);
    
    /**
     * @brief Set SID voice main parameters
     * @param voice Voice index (0-5)
     * @param enabled Enable (1) or disable (0) voice
     * @param pulseWidth Pulse width in microseconds
     * @param volume Volume (0-INT16_MAX)
     * @param frequencyTenths Frequency in tenths of Hz
     * @param noiseAmplitude White noise amplitude (SID noise waveform)
     */
    void SigGen_setVoiceSID(uint32_t voice, uint32_t enabled, int32_t pulseWidth, int32_t volume, int32_t frequencyTenths, int32_t noiseAmplitude);
    
    /* ===== VMS Mode Voice Control ===== */
    
    /**
     * @brief Set VMS voice hypervoice parameters
     * @param voice Voice index (0-5)
     * @param count Number of hypervoice sub-pulses per period
     * @param pulseWidth Hypervoice pulse width in microseconds
     * @param volume Hypervoice volume (0-INT16_MAX)
     * @param phase Phase offset for sub-pulses (0-255)
     */
    void SigGen_setHyperVoiceVMS(uint32_t voice, uint32_t count, uint32_t pulseWidth, uint32_t volume, uint32_t phase);
    
    /**
     * @brief Set VMS voice main parameters
     * @param voice Voice index (0-5)
     * @param enabled Enable (1) or disable (0) voice
     * @param pulseWidth Pulse width in microseconds
     * @param volume Volume (0-INT16_MAX)
     * @param frequencyTenths Frequency in tenths of Hz
     * @param noiseAmplitude White noise amplitude
     * @param burstOn_us Burst on-time in microseconds (0 = no burst)
     * @param burstOff_us Burst off-time in microseconds
     */
    void SigGen_setVoiceVMS(uint32_t voice, uint32_t enabled, int32_t pulseWidth, int32_t volume, int32_t frequencyTenths, int32_t noiseAmplitude, int32_t burstOn_us, int32_t burstOff_us);
    
    /* ===== Safety and Limiting ===== */
    
    /**
     * @brief Apply safety limits to all voice parameters
     *
     * Enforces:
     * - Duty cycle limits (max_tr_duty)
     * - Current limits via DAC range [min_tr_cl_dac_val, max_tr_cl_dac_val]
     * - Hypervoice count limiting
     * - Min period/on-time constraints
     *
     * Called by SigGen_task() each iteration.
     */
    void SigGen_limit();
    
    /**
     * @brief Enable or disable signal output
     * @param en Enable (1) or disable (0) output
     */
    void SigGen_setOutputEnabled(uint32_t en);
    
    /**
     * @brief Emergency stop - kill all audio output immediately
     *
     * Disables all voices and flushes pulse buffer.
     * Used for fault handling and user kill command.
     */
    void SigGen_killAudio();

    /* ===== Pulse Queue Interface ===== */
    
    /**
     * @brief Queue a pulse for hardware output
     * @param pulse Pointer to pulse descriptor (period, onTime, current)
     * @return 1 if pulse queued successfully, 0 if buffer full
     *
     * Units:
     * - Input: period/onTime in microseconds, current in siggen volume (0-INT16_MAX)
     * - Internally converted to timer counts and DAC values before queuing
     *
     * Thread-safe (uses ring buffer primitives).
     */
    uint8_t SigGen_queuePulse(SigGen_pulseData_t* pulse);

#endif
