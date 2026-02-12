/**
 * @file config.h
 * @brief System-wide configuration constants and compile-time parameters
 *
 * Defines timing constants, buffer sizes, task configuration, and hardware
 * parameters for the UD3 Tesla coil controller. These are compile-time
 * constants that affect memory allocation and system behavior.
 */

#if !defined(config_H)
#define config_H

/* ============================================================================
 * Timing Constants
 * ============================================================================ */

/** @brief MIDI processing ISR frequency in Hz */
#define MIDI_ISR_Hz 8000

/** @brief Signal generator clock frequency in Hz */
#define SG_CLOCK_Hz 320000

/** @brief MIDI ISR period in microseconds */
#define MIDI_ISR_US (1e6 / MIDI_ISR_Hz)

/** @brief Signal generator period in nanoseconds */
#define SG_PERIOD_NS (1e9 / SG_CLOCK_Hz)

/** @brief Signal generator half-period count */
#define SG_CLOCK_HALFCOUNT (SG_CLOCK_Hz/2)

/* ============================================================================
 * Ramp Chart Display Constants
 * ============================================================================ */

/** @brief Ramp chart height in pixels */
#define RAMP_CHART_HEIGHT 255

/** @brief Ramp chart width in pixels */
#define RAMP_CHART_WIDTH 400

/** @brief Ramp chart X-axis offset in pixels */
#define RAMP_CHART_OFFSET_X 40

/** @brief Ramp chart Y-axis offset in pixels */
#define RAMP_CHART_OFFSET_Y 20

/** @brief Ramp chart X-axis division spacing in pixels */
#define RAMP_CHART_DIV_X 25

/** @brief Ramp chart Y-axis division spacing in pixels */
#define RAMP_CHART_DIV_Y 25

/* ============================================================================
 * FreeRTOS Configuration
 * ============================================================================ */

/** @brief Enable task runtime statistics collection */
#define ACTIVATE_TASK_INFO 1

#ifndef SIMULATOR
/** @brief FreeRTOS heap size in kilobytes (firmware) */
#define HEAP_SIZE 48    //kb
#else
/** @brief FreeRTOS heap size in kilobytes (simulator) */
#define HEAP_SIZE 512    //kb	
#endif

/* ============================================================================
 * MIN Protocol Configuration
 * ============================================================================ */

/** @brief Maximum number of simultaneous MIN protocol connections */
#define NUM_MIN_CON 4

/** @brief MIN receive stream buffer size in bytes */
#define STREAMBUFFER_RX_SIZE    256     //bytes

/** @brief MIN transmit stream buffer size in bytes */
#define STREAMBUFFER_TX_SIZE    512    //bytes

/* ============================================================================
 * Alarm and Event System
 * ============================================================================ */

/** @brief Length of the alarm and event queue */
#define AE_QUEUE_SIZE       50

/* ============================================================================
 * Analog Task and ADC Configuration
 * ============================================================================ */

/** @brief ADC DMA buffer count (samples per channel) */
#define ADC_BUFFER_CNT      25

/** @brief ADC sample clock frequency in Hz */
#define ADC_SAMPLE_CLK      32000       //Hz

/** @brief New ADC data ready rate in milliseconds */
#define NEW_DATA_RATE_MS    ((1.0/(ADC_SAMPLE_CLK/4)) * ADC_BUFFER_CNT)    //ms

/** @brief Current control PID update frequency in Hz */
#define CURRENT_PID_HZ      ((uint16_t)(1.0 / NEW_DATA_RATE_MS))           //Hz

/** @brief Number of samples for RMS filter */
#define SAMPLES_COUNT       2048

/** @brief Bus voltage divider bottom resistor in ohms */
#define BUSV_R_BOT          5000UL

/** @brief Drive voltage divider top resistor in ohms */
#define DRIVEV_R_TOP        10000UL

/** @brief Drive voltage divider bottom resistor in ohms (UD2.x) */
#define DRIVEV_R_BOT        2200UL

/** @brief Drive voltage divider bottom resistor in ohms (UD3.0 boards) */
#define DRIVEV_R_BOT_V3     1000UL

/* ============================================================================
 * Voice Synthesis and MIDI/SID Processing
 * ============================================================================ */

/** @brief Size of the SID frame buffer (queue depth) */
#define N_QUEUE_SID     64

/** @brief Size of the MIDI event buffer (queue depth) */
#define N_QUEUE_MIDI    64

/** @brief Number of parallel polyphonic voices */
#define N_CHANNEL 4

/* ============================================================================
 * Hardware Pin Configuration
 * ============================================================================ */

/** @brief Relay 1 output polarity (0 = normal, 1 = inverted) */
#define RELAY1_INVERTED 0

/** @brief Relay 2 output polarity (0 = normal, 1 = inverted) */
#define RELAY2_INVERTED 0

/** @brief LED on state (respects invert configuration) */
#define LED_ON  (configuration.ivo_led ? 0 : 1)

/** @brief LED off state (respects invert configuration) */
#define LED_OFF (configuration.ivo_led ? 1 : 0)

/** @brief I/O invert option bit position for UART RX */
#define IVO_UART_RX_BIT  0

/** @brief I/O invert option bit position for UART TX */
#define IVO_UART_TX_BIT  1

/** @brief I/O invert option bit position for LED */
#define IVO_LED_BIT      2


    
#endif