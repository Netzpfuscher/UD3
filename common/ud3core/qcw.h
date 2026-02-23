/**
 * @file qcw.h
 * @brief QCW (Quasi-Continuous Wave) mode control for long-pulse Tesla coil operation
 *
 * QCW mode generates long, modulated pulses (100µs - 10ms+) with ramped current
 * envelopes for dramatic visual effects. Key features:
 * - Current ramping with configurable slope and holdoff
 * - Frequency modulation overlay (audio-rate modulation on DC envelope)
 * - Manual ramp editing via point/line commands
 * - Automatic pulse repetition with configurable period
 * - Safety limits (max pulse width, duty cycle)
 *
 * QCW differs from TR mode:
 * - TR: Short pulses (1-100µs) at audio rate for musical output
 * - QCW: Long pulses (100µs-10ms) with slow current ramp for CW-like operation
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


#if !defined(qcw_H)
#define qcw_H
    
#include <device.h>
#include "cli_common.h"
#include "TTerm.h"
#include "helper/teslaterm.h"

/** @brief Maximum QCW ramp array size in samples (250µs per sample = 100ms total) */
#define QCW_RAMP_SAMPLES (400)
    
/**
 * @brief QCW current ramp parameters and playback state
 *
 * Stores current envelope waveform and playback position. Ramp array is updated
 * at MIDI ISR rate (8kHz, 125µs per sample). Modified by qcw_regenerate_ramp()
 * or manual editing commands.
 */
typedef struct
{
	uint8_t changed;            /**< Flag: ramp needs regeneration before next pulse */
	uint16_t index;             /**< Current playback position (incremented each MIDI ISR) */
	uint16_t stop_index;        /**< End position (calculated from qcw_pw parameter) */
	uint8_t data[QCW_RAMP_SAMPLES]; /**< Current envelope values (0-255, 0=min current, 255=max) */
} ramp_params;

extern ramp_params volatile ramp;


/* ========== Core QCW Control Functions ========== */

/**
 * @brief Start a QCW pulse (enables hardware and initializes playback)
 *
 * Returns early if duty cycle exceeds max_qcw_duty. Atomically enables QCW
 * output and sets initial phase shift. Called by qcw command or MIDI handler.
 */
void qcw_start();

/**
 * @brief Update current limit modulation for current ramp sample
 * @param val Modulation value (0-255, scales phase shift within safe range)
 *
 * Called from MIDI ISR at 8kHz rate during QCW pulse. Linearizes modulation
 * value based on feedback filter output to maintain safe operating range.
 */
void qcw_modulate(uint16_t val);

/**
 * @brief Stop QCW pulse immediately (disables hardware and resets modulation)
 *
 * Clears QCW enable flag and resets phase shift to zero. Safe to call at any time.
 */
void qcw_stop();

/**
 * @brief Regenerate ramp array from current parameters
 *
 * Generates current envelope from qcw_offset, qcw_ramp, qcw_max, qcw_holdoff,
 * qcw_vol, and qcw_freq parameters. Applies frequency modulation overlay if
 * qcw_vol > 0. Only updates if ramp.changed flag is set.
 */
void qcw_regenerate_ramp();

/**
 * @brief MIDI ISR callback - advance ramp playback by one sample
 *
 * Increments ramp.index and calls qcw_modulate() with current sample value.
 * Stops pulse when reaching stop_index. Must be called at MIDI ISR rate (8kHz).
 */
void qcw_handle();

/**
 * @brief Synthesizer integration handler (reserved for future use)
 *
 * Placeholder for QCW integration with MIDI/SID synthesizers.
 */
void qcw_handle_synth();

/* ========== MIDI Integration ========== */

/**
 * @brief Trigger QCW pulse from MIDI command with frequency modulation
 * @param volume Volume (unused in current implementation)
 * @param frequencyTenths Modulation frequency in tenths of Hz (e.g., 4400 = 440.0 Hz)
 *
 * Sets qcw_freq parameter and starts pulse. Used for MIDI-controlled QCW mode.
 * Only starts new pulse if previous pulse has finished.
 */
void qcw_cmd_midi_pulse(int32_t volume, int32_t frequencyTenths);

/* ========== Ramp Editing Functions ========== */

/**
 * @brief Visualize ramp waveform in Teslaterm graphical chart
 * @param chart Chart configuration (dimensions, offsets, divisions)
 * @param handle Terminal handle for output
 *
 * Draws ramp envelope in green, max_qcw_pw limit in red, qcw_pw setting in blue.
 * Only works with Teslaterm (not VT100 terminal).
 */
void qcw_ramp_visualize(CHART *chart, TERMINAL_HANDLE * handle);

/**
 * @brief Draw line in ramp array using Bresenham's algorithm
 * @param x0 Start X coordinate (sample index, 0-399)
 * @param y0 Start Y value (current, 0-255)
 * @param x1 End X coordinate (sample index, 0-399)
 * @param y1 End Y value (current, 0-255)
 *
 * Allows manual ramp shaping. Coordinates clipped to array bounds.
 */
void qcw_ramp_line(uint16_t x0,uint8_t y0,uint16_t x1, uint8_t y1);

/**
 * @brief Set single point in ramp array
 * @param x Sample index (0-399)
 * @param y Current value (0-255)
 *
 * Bounds-checked write to ramp data array.
 */
void qcw_ramp_point(uint16_t x,uint8_t y);
    
/* ========== CLI Commands and Callbacks ========== */

/**
 * @brief CLI command for manual ramp editing and visualization
 * @param handle Terminal handle for output
 * @param argCount Number of arguments
 * @param args Argument array
 * @return TERM_CMD_EXIT_SUCCESS
 *
 * Usage:
 * - `ramp point <x> <y>` - Set single point
 * - `ramp line <x0> <y0> <x1> <y1>` - Draw line
 * - `ramp clear` - Zero entire ramp array
 * - `ramp draw` - Visualize in Teslaterm chart
 *
 * Only available when is_qcw configuration flag is set.
 */
uint8_t CMD_ramp(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);

/**
 * @brief Callback when QCW ramp parameters change
 * @param params Parameter array (unused)
 * @param index Parameter index (unused)
 * @param handle Terminal handle (unused)
 * @return pdPASS
 *
 * Sets ramp.changed flag and regenerates ramp if QCW is not actively running.
 * Triggered by changes to: qcw_offset, qcw_ramp, qcw_max, qcw_holdoff, qcw_vol, qcw_freq.
 */
uint8_t callback_rampFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);

/**
 * @brief Delete QCW auto-repeat timer
 * @return pdPASS if timer deleted successfully, pdFAIL otherwise
 *
 * Stops automatic QCW pulse repetition. Safe to call even if timer doesn't exist.
 */
/**
 * @brief Delete QCW auto-repeat timer
 * @return pdPASS if timer deleted successfully, pdFAIL otherwise
 *
 * Stops automatic QCW pulse repetition. Safe to call even if timer doesn't exist.
 */
BaseType_t QCW_delete_timer(void);

/* Note: CMD_qcw() is declared in cli_common.c */
    
#endif
