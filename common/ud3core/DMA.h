/**
 * @file DMA.h
 * @brief DMA channel configuration and initialization for PWM and feedback control
 *
 * Configures DMA channels for automated data transfer between peripherals and memory.
 * Handles feedback filtering chain, PWM parameter updates, and current limit control.
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

#ifndef DMA_H
#define DMA_H

/* ============================================================================
 * Feedback Capture to RAM DMA
 * Transfers feedback capture value from peripheral to RAM buffer
 * ============================================================================ */

/** @brief Bytes transferred per DMA burst for feedback capture */
#define FBC_to_ram_DMA_BYTES_PER_BURST 2

/** @brief DMA requests per burst for feedback capture */
#define FBC_to_ram_DMA_REQUEST_PER_BURST 1

/** @brief Source base address (peripheral registers) */
#define FBC_to_ram_DMA_SRC_BASE (CYDEV_PERIPH_BASE)

/** @brief Destination base address (SRAM) */
#define FBC_to_ram_DMA_DST_BASE (CYDEV_SRAM_BASE)

/* ============================================================================
 * RAM to Filter DMA
 * Transfers data from RAM buffer to hardware filter input
 * ============================================================================ */

/** @brief Bytes transferred per DMA burst for filter input */
#define ram_to_filter_DMA_BYTES_PER_BURST 2

/** @brief DMA requests per burst for filter input */
#define ram_to_filter_DMA_REQUEST_PER_BURST 1

/** @brief Source base address (SRAM) */
#define ram_to_filter_DMA_SRC_BASE (CYDEV_SRAM_BASE)

/** @brief Destination base address (filter peripheral) */
#define ram_to_filter_DMA_DST_BASE (CYDEV_PERIPH_BASE)

/* ============================================================================
 * Filter to Frame RAM DMA
 * Transfers filtered output from hardware filter to RAM
 * ============================================================================ */

/** @brief Bytes transferred per DMA burst for filter output */
#define filter_to_fram_DMA_BYTES_PER_BURST 2

/** @brief DMA requests per burst for filter output */
#define filter_to_fram_DMA_REQUEST_PER_BURST 1

/** @brief Source base address (filter peripheral) */
#define filter_to_fram_DMA_SRC_BASE (CYDEV_PERIPH_BASE)

/** @brief Destination base address (SRAM) */
#define filter_to_fram_DMA_DST_BASE (CYDEV_SRAM_BASE)

/* ============================================================================
 * Frame RAM to PWM A DMA
 * Transfers filtered feedback value to PWM A compare register
 * ============================================================================ */

/** @brief Bytes transferred per DMA burst for PWM A update */
#define fram_to_PWMA_DMA_BYTES_PER_BURST 2

/** @brief DMA requests per burst for PWM A update */
#define fram_to_PWMA_DMA_REQUEST_PER_BURST 1

/** @brief Source base address (SRAM) */
#define fram_to_PWMA_DMA_SRC_BASE (CYDEV_SRAM_BASE)

/** @brief Destination base address (PWM peripheral) */
#define fram_to_PWMA_DMA_DST_BASE (CYDEV_PERIPH_BASE)

/* ============================================================================
 * PWM A Initialization DMA
 * Initializes PWM A and B period/compare registers at startup
 * ============================================================================ */

/** @brief Bytes transferred per DMA burst for PWM initialization (8 bytes total) */
#define PWMA_init_DMA_BYTES_PER_BURST 8

/** @brief DMA requests per burst for PWM initialization */
#define PWMA_init_DMA_REQUEST_PER_BURST 1

/** @brief Source base address (SRAM parameter structure) */
#define PWMA_init_DMA_SRC_BASE (CYDEV_SRAM_BASE)

/** @brief Destination base address (PWM peripherals) */
#define PWMA_init_DMA_DST_BASE (CYDEV_PERIPH_BASE)

/* ============================================================================
 * Phase Shift Burst Initialization DMA
 * Initializes PWM B for phase shift burst mode
 * ============================================================================ */

/** @brief Bytes transferred per DMA burst for PSB initialization */
#define PSBINIT_DMA_BYTES_PER_BURST 4

/** @brief DMA requests per burst for PSB initialization */
#define PSBINIT_DMA_REQUEST_PER_BURST 1

/** @brief Source base address (SRAM parameters) */
#define PSBINIT_DMA_SRC_BASE (CYDEV_SRAM_BASE)

/** @brief Destination base address (PWM B peripheral) */
#define PSBINIT_DMA_DST_BASE (CYDEV_PERIPH_BASE)

/* ============================================================================
 * PWM B Phase Shift Burst DMA
 * Updates PWM B compare value for phase shift burst operation
 * ============================================================================ */

/** @brief Bytes transferred per DMA burst for PWM B PSB update */
#define PWMB_PSB_DMA_BYTES_PER_BURST 2

/** @brief DMA requests per burst for PWM B PSB update */
#define PWMB_PSB_DMA_REQUEST_PER_BURST 1

/** @brief Source base address (SRAM PSB value) */
#define PWMB_PSB_DMA_SRC_BASE (CYDEV_SRAM_BASE)

/** @brief Destination base address (PWM B compare register) */
#define PWMB_PSB_DMA_DST_BASE (CYDEV_PERIPH_BASE)

/* ============================================================================
 * QCW Current Limit DMA
 * Updates DAC for QCW mode current limiting
 * ============================================================================ */

/** @brief Bytes transferred per DMA burst for QCW current limit DAC */
#define QCW_CL_DMA_BYTES_PER_BURST 2

/** @brief DMA requests per burst for QCW current limit */
#define QCW_CL_DMA_REQUEST_PER_BURST 1

/** @brief Source base address (SRAM DAC value array) */
#define QCW_CL_DMA_SRC_BASE (CYDEV_SRAM_BASE)

/** @brief Destination base address (DAC peripheral) */
#define QCW_CL_DMA_DST_BASE (CYDEV_PERIPH_BASE)

/* ============================================================================
 * Transient Mode Current Limit DMA
 * Updates DAC for transient (TR1) mode current limiting
 * ============================================================================ */

/** @brief Bytes transferred per DMA burst for TR1 current limit DAC */
#define TR1_CL_DMA_BYTES_PER_BURST 2

/** @brief DMA requests per burst for TR1 current limit */
#define TR1_CL_DMA_REQUEST_PER_BURST 1

/** @brief Source base address (SRAM DAC value array) */
#define TR1_CL_DMA_SRC_BASE (CYDEV_SRAM_BASE)

/** @brief Destination base address (DAC peripheral) */
#define TR1_CL_DMA_DST_BASE (CYDEV_PERIPH_BASE)

/**
 * @brief Initialize all DMA channels for PWM and feedback control
 *
 * Configures and enables DMA channels for:
 * - Feedback filtering chain (capture -> filter -> PWM)
 * - PWM initialization (startup parameters)
 * - Phase shift burst control
 * - Current limit DAC updates (QCW and TR1 modes)
 *
 * Must be called during system initialization before starting PWM operation.
 */
void initialize_DMA(void);
#endif
