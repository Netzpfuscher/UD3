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

#ifndef DMA_H
#define DMA_H

/* DMA Configuration for FBC_to_ram_DMA */
#define FBC_to_ram_DMA_BYTES_PER_BURST 2
#define FBC_to_ram_DMA_REQUEST_PER_BURST 1
#define FBC_to_ram_DMA_SRC_BASE (CYDEV_PERIPH_BASE)
#define FBC_to_ram_DMA_DST_BASE (CYDEV_SRAM_BASE)

/* DMA Configuration for ram_to_filter_DMA */
#define ram_to_filter_DMA_BYTES_PER_BURST 2
#define ram_to_filter_DMA_REQUEST_PER_BURST 1
#define ram_to_filter_DMA_SRC_BASE (CYDEV_SRAM_BASE)
#define ram_to_filter_DMA_DST_BASE (CYDEV_PERIPH_BASE)

/* DMA Configuration for filter_to_fram_DMA */
#define filter_to_fram_DMA_BYTES_PER_BURST 2
#define filter_to_fram_DMA_REQUEST_PER_BURST 1
#define filter_to_fram_DMA_SRC_BASE (CYDEV_PERIPH_BASE)
#define filter_to_fram_DMA_DST_BASE (CYDEV_SRAM_BASE)

/* DMA Configuration for fram_to_PWMA_DMA */
#define fram_to_PWMA_DMA_BYTES_PER_BURST 2
#define fram_to_PWMA_DMA_REQUEST_PER_BURST 1
#define fram_to_PWMA_DMA_SRC_BASE (CYDEV_SRAM_BASE)
#define fram_to_PWMA_DMA_DST_BASE (CYDEV_PERIPH_BASE)

/* DMA Configuration for PWMA_init_DMA */
#define PWMA_init_DMA_BYTES_PER_BURST 8
#define PWMA_init_DMA_REQUEST_PER_BURST 1
#define PWMA_init_DMA_SRC_BASE (CYDEV_SRAM_BASE)
#define PWMA_init_DMA_DST_BASE (CYDEV_PERIPH_BASE)

/* DMA Configuration for PSBINIT_DMA */
#define PSBINIT_DMA_BYTES_PER_BURST 4
#define PSBINIT_DMA_REQUEST_PER_BURST 1
#define PSBINIT_DMA_SRC_BASE (CYDEV_SRAM_BASE)
#define PSBINIT_DMA_DST_BASE (CYDEV_PERIPH_BASE)

/* DMA Configuration for PWMB_PSB_DMA */
#define PWMB_PSB_DMA_BYTES_PER_BURST 2
#define PWMB_PSB_DMA_REQUEST_PER_BURST 1
#define PWMB_PSB_DMA_SRC_BASE (CYDEV_SRAM_BASE)
#define PWMB_PSB_DMA_DST_BASE (CYDEV_PERIPH_BASE)

/* DMA Configuration for QCW_CL_DMA */
#define QCW_CL_DMA_BYTES_PER_BURST 2
#define QCW_CL_DMA_REQUEST_PER_BURST 1
#define QCW_CL_DMA_SRC_BASE (CYDEV_SRAM_BASE)
#define QCW_CL_DMA_DST_BASE (CYDEV_PERIPH_BASE)

/* DMA Configuration for TR1_CL_DMA */
#define TR1_CL_DMA_BYTES_PER_BURST 2
#define TR1_CL_DMA_REQUEST_PER_BURST 1
#define TR1_CL_DMA_SRC_BASE (CYDEV_SRAM_BASE)
#define TR1_CL_DMA_DST_BASE (CYDEV_PERIPH_BASE)

void initialize_DMA(void);
#endif
