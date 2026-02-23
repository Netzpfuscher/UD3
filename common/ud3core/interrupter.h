/**
 * @file interrupter.h
 * @brief Main interrupter control and pulse generation system
 *
 * Controls the Tesla coil firing pulses through a 16-bit PWM clocked at 1MHz
 * (1µs resolution). Manages transient mode, MIDI/SID synthesis modes, burst
 * operation, and external interrupter input. Integrates with signal generator,
 * duty compressor, and DMA for automated pulse parameter updates.
 *
 * The interrupter enables the ZCDtoPWM hardware which generates precisely
 * timed pulses for the coil driver.
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
#ifndef INTERRUPTER_H
#define INTERRUPTER_H

#include <stdint.h>
#include <device.h>
#include "cli_basic.h"
#include "timers.h"

/** @brief Interrupter clock frequency in Hz (1MHz = 1µs resolution) */
#define INTERRUPTER_CLK_FREQ 1000000

/* Interrupter control register bit flags */

/** @brief Kill all interrupter output (disable) */
#define INT_KILL_ALL    0b0000

/** @brief Enable interrupter output */
#define INT_ENA         0b0001

/** @brief Auto-reload mode (continuous pulsing) */
#define INT_AUTO_RELOAD 0b0010

/** @brief Enable external interrupter input */
#define INT_EXT_ENA     0b0100

/** @brief Invert external interrupter input polarity */
#define INT_EXT_INV     0b1000

/* DMA Configuration for interrupter parameter updates */

/** @brief Bytes transferred per DMA burst for interrupter updates (8 bytes total) */
#define int1_dma_BYTES_PER_BURST 8

/** @brief DMA requests per burst for interrupter updates */
#define int1_dma_REQUEST_PER_BURST 1

/** @brief Source base address (SRAM - int1_prd and int1_cmp variables) */
#define int1_dma_SRC_BASE (CYDEV_SRAM_BASE)

/** @brief Destination base address (PWM peripheral registers) */
#define int1_dma_DST_BASE (CYDEV_PERIPH_BASE)

/** @brief Maximum volume value (INT16_MAX for signal generator) */
#define MAX_VOL INT16_MAX

/** @brief Minimum volume value */
#define MIN_VOL 0

/**
 * @brief DMA mode selection for interrupter
 */
enum interrupter_DMA{
	INTR_DMA_TR,   /**< Transient/classic mode DMA */
	INTR_DMA_DDS   /**< Direct Digital Synthesis (MIDI/SID) mode DMA */
};

/**
 * @brief Interrupter operating mode
 */
enum interrupter_mode{
	INTR_MODE_OFF=0,    /**< Interrupter disabled */
	INTR_MODE_TR,       /**< Transient/classic mode active */
	INTR_MODE_BLOCKED   /**< Interrupter blocked (fault condition) */
};

/**
 * @brief Burst mode state
 */
enum interrupter_burst{
	BURST_ON,   /**< Burst on-time (pulsing) */
	BURST_OFF   /**< Burst off-time (silent) */
};

/**
 * @brief Modulation mode (pulse width vs current)
 */
enum interrupter_modulation{
	INTR_MOD_PW=0,   /**< Modulate pulse width */
	INTR_MOD_CUR=1   /**< Modulate current limit */
};

/**
 * @brief Interrupter state and configuration structure
 */
typedef struct
{
	uint16_t pw;                          /**< Current pulse width in microseconds */
	uint16_t prd;                         /**< Current period in microseconds */
	enum interrupter_mode mode;           /**< Operating mode (off/TR/blocked) */
	enum interrupter_burst burst_state;   /**< Burst state (on/off) */
	enum interrupter_modulation mod;      /**< Modulation mode (PW/current) */
	enum interrupter_DMA dma_mode;        /**< DMA mode (TR/DDS) */
	TimerHandle_t xBurst_Timer;           /**< FreeRTOS timer handle for burst timing */
} interrupter_params;

extern interrupter_params interrupter;

/** @brief Interrupter period register value (simulator/debug access only) */
extern uint16_t int1_prd;

/** @brief Interrupter compare register value (simulator/debug access only) */
extern uint16_t int1_cmp;

/** @brief Interrupter DMA channel handle (simulator/debug access only) */
extern uint8_t int1_dma_Chan;

/* ============================================================================
 * Core Functions
 * ============================================================================ */

/**
 * @brief One-time initialization of interrupter hardware and DMA
 *
 * Initializes signal generator, duty compressor, PWM hardware, and DMA channels.
 * Sets safe initial values for pulse parameters. Must be called once at startup.
 */
void initialize_interrupter(void);

/**
 * @brief Configure interrupter based on current parameter settings
 *
 * Updates minimum period based on max PRF, resets to safe state.
 * Called when EEPROM is loaded or interrupter settings change.
 */
void configure_interrupter();

/**
 * @brief Update interrupter for transient/classic mode
 *
 * Recalculates frequency and updates signal generator with current TR parameters
 * (pulse width, period, burst on/off times). Only active when synth mode is SYNTH_TR.
 */
void interrupter_updateTR();

/**
 * @brief Ramp control for smooth parameter transitions
 *
 * Handles ramping of pulse parameters to avoid abrupt changes.
 */
void ramp_control(void);

/**
 * @brief Fire a single pulse with specified parameters (scaled volume)
 * @param pw Pulse width in microseconds
 * @param vol Volume (0 to INT16_MAX, scaled to current limit DAC value)
 *
 * Generates a single transient-mode pulse. Enforces safety limits (max_tr_pw).
 * Volume is scaled to DAC value based on current limit configuration.
 * Does not fire if fault condition exists or external interrupter is active.
 */
void interrupter_oneshot(uint32_t pw, uint32_t vol);

/**
 * @brief Fire a single pulse with raw DAC value (no scaling)
 * @param pw_us Pulse width in microseconds
 * @param dacValue_counts Raw DAC value in counts for current limit
 *
 * Generates a single pulse with direct DAC control. Used for precise
 * current limit control without scaling. Enforces max_tr_pw limit.
 */
void interrupter_oneshotRaw(uint32_t pw_us, uint32_t dacValue_counts);

/**
 * @brief Update interrupter for external input mode
 *
 * Configures hardware to accept external gate/trigger signals. Sets current
 * limit to maximum and pulse width to configured max. External signal controls
 * timing. Polarity can be normal or inverted based on configuration.
 */
void interrupter_update_ext();

/**
 * @brief Set pulse width for specified channel
 * @param ch Channel number (0-3)
 * @param pw Pulse width in microseconds
 *
 * Updates pulse width for DMA-driven multi-channel operation.
 */
void interrupter_set_pw(uint8_t ch, uint16_t pw);

/**
 * @brief Set pulse width and volume for specified channel
 * @param ch Channel number (0-3)
 * @param pw Pulse width in microseconds
 * @param vol Volume/current (0 to INT16_MAX)
 *
 * Updates both pulse width and current limit for DMA-driven multi-channel operation.
 */
void interrupter_set_pw_vol(uint8_t ch, uint16_t pw, uint32_t vol);

/**
 * @brief Switch DMA mode between TR and DDS (MIDI/SID)
 * @param mode DMA mode to activate (INTR_DMA_TR or INTR_DMA_DDS)
 *
 * Reconfigures DMA channels for either transient mode or synthesizer mode operation.
 */
void interrupter_DMA_mode(enum interrupter_DMA mode);

/**
 * @brief Initialize interrupter to safe default state
 *
 * Sets large period, minimal pulse width, and kills output. Used during
 * configuration changes or fault recovery.
 */
void interrupter_init_safe();

/* ============================================================================
 * Parameter Callbacks
 * ============================================================================ */

/**
 * @brief Callback when external interrupter setting changes
 * @param params Parameter array
 * @param index Parameter index
 * @param handle Terminal handle
 * @return pdPASS
 *
 * Activates/deactivates external interrupter mode and pushes warning alarm.
 */
uint8_t callback_ext_interrupter(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);

/**
 * @brief Callback when burst mode parameters change
 * @param params Parameter array
 * @param index Parameter index
 * @param handle Terminal handle
 * @return pdPASS
 *
 * Updates interrupter with new burst on/off times.
 */
uint8_t callback_BurstFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);

/**
 * @brief Callback when pulse width parameter changes
 * @param params Parameter array
 * @param index Parameter index
 * @param handle Terminal handle
 * @return pdPASS
 *
 * Updates hardware or signal generator based on current synth mode (TR/MIDI/external).
 */
uint8_t callback_PWFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);

/**
 * @brief Callback when volume parameter changes
 * @param params Parameter array
 * @param index Parameter index
 * @param handle Terminal handle
 * @return pdPASS
 *
 * Updates signal generator master volume.
 */
uint8_t callback_VolFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);

/**
 * @brief Callback when modulation mode changes
 * @param params Parameter array
 * @param index Parameter index
 * @param handle Terminal handle
 * @return pdPASS
 *
 * Note: Modulation mode changes no longer supported in current implementation.
 */
uint8_t callback_interrupter_mod(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);

/* ============================================================================
 * Safety Functions
 * ============================================================================ */

/**
 * @brief Emergency stop - disable all interrupter output
 *
 * Sets interlock flag, kills audio synthesis, zeros pulse width, and updates
 * hardware to safe state. Called on critical faults or user kill command.
 */
void interrupter_kill(void);

/**
 * @brief Release interlock to allow interrupter operation
 *
 * Clears interlock flag. Does not automatically restart operation - user must
 * explicitly start desired mode after unkill.
 */
void interrupter_unkill(void);

/* ============================================================================
 * CLI Commands
 * ============================================================================ */

/**
 * @brief Synthesizer monitor command - displays real-time synth status
 * @param handle Terminal handle
 * @param argCount Argument count
 * @param args Command arguments
 * @return TERM_CMD_EXIT_SUCCESS
 *
 * Shows voice status for MIDI or SID synthesizer, compressor state, and
 * note mappings. Updates display every 1 second until Ctrl+C pressed.
 */
uint8_t CMD_SynthMon(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);

/**
 * @brief Callback when synthesizer filter settings change
 * @param params Parameter array
 * @param index Parameter index
 * @param handle Terminal handle
 * @return Status code
 *
 * Updates synthesizer filter configuration.
 */
uint8_t callback_synthFilter(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);

/**
 * @brief Callback when synthesizer mode changes
 * @param params Parameter array
 * @param index Parameter index
 * @param handle Terminal handle
 * @return pdTRUE
 *
 * Resets MIDI and SID processors, switches signal generator to new synth mode.
 */
uint8_t callback_SynthFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);

#endif
