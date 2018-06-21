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

#include "DMA.h"
#include "ZCDtoPWM.h"
#include <device.h>

void initialize_DMA(void) {
	/* Variable declarations for FBC_to_ram_DMA */
	/* Move these variable declarations to the top of the function */
	uint8 FBC_to_ram_DMA_Chan;
	uint8 FBC_to_ram_DMA_TD[1];
	FBC_to_ram_DMA_Chan = FBC_to_ram_DMA_DmaInitialize(FBC_to_ram_DMA_BYTES_PER_BURST, FBC_to_ram_DMA_REQUEST_PER_BURST,
													   HI16(FBC_to_ram_DMA_SRC_BASE), HI16(FBC_to_ram_DMA_DST_BASE));
	FBC_to_ram_DMA_TD[0] = CyDmaTdAllocate();
	CyDmaTdSetConfiguration(FBC_to_ram_DMA_TD[0], 2, FBC_to_ram_DMA_TD[0], FBC_to_ram_DMA__TD_TERMOUT_EN);
	CyDmaTdSetAddress(FBC_to_ram_DMA_TD[0], LO16((uint32)FB_capture_CAPTURE_LSB_PTR), LO16((uint32)&fb_filter_in));
	CyDmaChSetInitialTd(FBC_to_ram_DMA_Chan, FBC_to_ram_DMA_TD[0]);
	CyDmaChEnable(FBC_to_ram_DMA_Chan, 1);

	/* Variable declarations for ram_to_filter_DMA */
	/* Move these variable declarations to the top of the function */
	uint8 ram_to_filter_DMA_Chan;
	uint8 ram_to_filter_DMA_TD[1];
	ram_to_filter_DMA_Chan = ram_to_filter_DMA_DmaInitialize(ram_to_filter_DMA_BYTES_PER_BURST, ram_to_filter_DMA_REQUEST_PER_BURST,
															 HI16(ram_to_filter_DMA_SRC_BASE), HI16(ram_to_filter_DMA_DST_BASE));
	ram_to_filter_DMA_TD[0] = CyDmaTdAllocate();
	CyDmaTdSetConfiguration(ram_to_filter_DMA_TD[0], 2, ram_to_filter_DMA_TD[0], ram_to_filter_DMA__TD_TERMOUT_EN);
	CyDmaTdSetAddress(ram_to_filter_DMA_TD[0], LO16((uint32)&fb_filter_in), LO16((uint32)FB_Filter_STAGEA_PTR));
	CyDmaChSetInitialTd(ram_to_filter_DMA_Chan, ram_to_filter_DMA_TD[0]);
	CyDmaChEnable(ram_to_filter_DMA_Chan, 1);

	/* Variable declarations for filter_to_fram_DMA */
	/* Move these variable declarations to the top of the function */
	uint8 filter_to_fram_DMA_Chan;
	uint8 filter_to_fram_DMA_TD[1];
	filter_to_fram_DMA_Chan = filter_to_fram_DMA_DmaInitialize(filter_to_fram_DMA_BYTES_PER_BURST, filter_to_fram_DMA_REQUEST_PER_BURST,
															   HI16(filter_to_fram_DMA_SRC_BASE), HI16(filter_to_fram_DMA_DST_BASE));
	filter_to_fram_DMA_TD[0] = CyDmaTdAllocate();
	CyDmaTdSetConfiguration(filter_to_fram_DMA_TD[0], 2, filter_to_fram_DMA_TD[0], filter_to_fram_DMA__TD_TERMOUT_EN);
	CyDmaTdSetAddress(filter_to_fram_DMA_TD[0], LO16((uint32)FB_Filter_HOLDA_PTR), LO16((uint32)&fb_filter_out));
	CyDmaChSetInitialTd(filter_to_fram_DMA_Chan, filter_to_fram_DMA_TD[0]);
	CyDmaChEnable(filter_to_fram_DMA_Chan, 1);

	/* Variable declarations for fram_to_PWMA_DMA */
	/* Move these variable declarations to the top of the function */
	uint8 fram_to_PWMA_DMA_Chan;
	uint8 fram_to_PWMA_DMA_TD[1];
	fram_to_PWMA_DMA_Chan = fram_to_PWMA_DMA_DmaInitialize(fram_to_PWMA_DMA_BYTES_PER_BURST, fram_to_PWMA_DMA_REQUEST_PER_BURST,
														   HI16(fram_to_PWMA_DMA_SRC_BASE), HI16(fram_to_PWMA_DMA_DST_BASE));
	fram_to_PWMA_DMA_TD[0] = CyDmaTdAllocate();
	CyDmaTdSetConfiguration(fram_to_PWMA_DMA_TD[0], 2, fram_to_PWMA_DMA_TD[0], fram_to_PWMA_DMA__TD_TERMOUT_EN);
	CyDmaTdSetAddress(fram_to_PWMA_DMA_TD[0], LO16((uint32)&fb_filter_out), LO16((uint32)PWMA_COMPARE1_LSB_PTR));
	CyDmaChSetInitialTd(fram_to_PWMA_DMA_Chan, fram_to_PWMA_DMA_TD[0]);
	CyDmaChEnable(fram_to_PWMA_DMA_Chan, 1);

	/* Variable declarations for PWMA_init_DMA */
	/* Move these variable declarations to the top of the function */
	uint8 PWMA_init_DMA_Chan;
	uint8 PWMA_init_DMA_TD[4];
	PWMA_init_DMA_Chan = PWMA_init_DMA_DmaInitialize(PWMA_init_DMA_BYTES_PER_BURST, PWMA_init_DMA_REQUEST_PER_BURST,
													 HI16(PWMA_init_DMA_SRC_BASE), HI16(PWMA_init_DMA_DST_BASE));
	PWMA_init_DMA_TD[0] = CyDmaTdAllocate();
	PWMA_init_DMA_TD[1] = CyDmaTdAllocate();
	PWMA_init_DMA_TD[2] = CyDmaTdAllocate();
	PWMA_init_DMA_TD[3] = CyDmaTdAllocate();
	CyDmaTdSetConfiguration(PWMA_init_DMA_TD[0], 2, PWMA_init_DMA_TD[1], TD_AUTO_EXEC_NEXT);
	CyDmaTdSetConfiguration(PWMA_init_DMA_TD[1], 2, PWMA_init_DMA_TD[2], TD_AUTO_EXEC_NEXT);
	CyDmaTdSetConfiguration(PWMA_init_DMA_TD[2], 2, PWMA_init_DMA_TD[3], TD_AUTO_EXEC_NEXT);
	CyDmaTdSetConfiguration(PWMA_init_DMA_TD[3], 2, PWMA_init_DMA_TD[0], PWMA_init_DMA__TD_TERMOUT_EN);
	CyDmaTdSetAddress(PWMA_init_DMA_TD[0], LO16((uint32)&params.pwma_start_prd), LO16((uint32)PWMA_PERIOD_LSB_PTR));
	CyDmaTdSetAddress(PWMA_init_DMA_TD[1], LO16((uint32)&params.pwma_start_cmp), LO16((uint32)PWMA_COMPARE1_LSB_PTR));
	CyDmaTdSetAddress(PWMA_init_DMA_TD[2], LO16((uint32)&params.pwmb_start_prd), LO16((uint32)PWMB_PERIOD_LSB_PTR));
	CyDmaTdSetAddress(PWMA_init_DMA_TD[3], LO16((uint32)&params.pwmb_start_cmp), LO16((uint32)PWMB_COMPARE1_LSB_PTR));
	CyDmaChSetInitialTd(PWMA_init_DMA_Chan, PWMA_init_DMA_TD[0]);
	CyDmaChEnable(PWMA_init_DMA_Chan, 1);

	/* Variable declarations for PSBINIT_DMA */
	/* Move these variable declarations to the top of the function */
	uint8 PSBINIT_DMA_Chan;
	uint8 PSBINIT_DMA_TD[2];
	PSBINIT_DMA_Chan = PSBINIT_DMA_DmaInitialize(PSBINIT_DMA_BYTES_PER_BURST, PSBINIT_DMA_REQUEST_PER_BURST,
												 HI16(PSBINIT_DMA_SRC_BASE), HI16(PSBINIT_DMA_DST_BASE));
	PSBINIT_DMA_TD[0] = CyDmaTdAllocate();
	PSBINIT_DMA_TD[1] = CyDmaTdAllocate();
	CyDmaTdSetConfiguration(PSBINIT_DMA_TD[0], 2, PSBINIT_DMA_TD[1], TD_AUTO_EXEC_NEXT);
	CyDmaTdSetConfiguration(PSBINIT_DMA_TD[1], 2, PSBINIT_DMA_TD[0], PSBINIT_DMA__TD_TERMOUT_EN);
	CyDmaTdSetAddress(PSBINIT_DMA_TD[0], LO16((uint32)&params.pwm_top), LO16((uint32)PWMB_PERIOD_LSB_PTR));
	CyDmaTdSetAddress(PSBINIT_DMA_TD[1], LO16((uint32)&params.pwmb_psb_val), LO16((uint32)PWMB_COMPARE1_LSB_PTR));
	CyDmaChSetInitialTd(PSBINIT_DMA_Chan, PSBINIT_DMA_TD[0]);
	CyDmaChEnable(PSBINIT_DMA_Chan, 1);

	/* Variable declarations for PWMB_PSB_DMA */
	/* Move these variable declarations to the top of the function */
	uint8 PWMB_PSB_DMA_Chan;
	uint8 PWMB_PSB_DMA_TD[1];
	PWMB_PSB_DMA_Chan = PWMB_PSB_DMA_DmaInitialize(PWMB_PSB_DMA_BYTES_PER_BURST, PWMB_PSB_DMA_REQUEST_PER_BURST,
												   HI16(PWMB_PSB_DMA_SRC_BASE), HI16(PWMB_PSB_DMA_DST_BASE));
	PWMB_PSB_DMA_TD[0] = CyDmaTdAllocate();
	CyDmaTdSetConfiguration(PWMB_PSB_DMA_TD[0], 2, PWMB_PSB_DMA_TD[0], PWMB_PSB_DMA__TD_TERMOUT_EN); // TD_AUTO_EXEC_NEXT);
	CyDmaTdSetAddress(PWMB_PSB_DMA_TD[0], LO16((uint32)&params.pwmb_psb_val), LO16((uint32)PWMB_COMPARE1_LSB_PTR));
	CyDmaChSetInitialTd(PWMB_PSB_DMA_Chan, PWMB_PSB_DMA_TD[0]);
	CyDmaChEnable(PWMB_PSB_DMA_Chan, 1);

	/* Variable declarations for QCW_CL_DMA */
	/* Move these variable declarations to the top of the function */
	uint8 QCW_CL_DMA_Chan;
	uint8 QCW_CL_DMA_TD[2];
	QCW_CL_DMA_Chan = QCW_CL_DMA_DmaInitialize(QCW_CL_DMA_BYTES_PER_BURST, QCW_CL_DMA_REQUEST_PER_BURST,
											   HI16(QCW_CL_DMA_SRC_BASE), HI16(QCW_CL_DMA_DST_BASE));
	QCW_CL_DMA_TD[0] = CyDmaTdAllocate();
	QCW_CL_DMA_TD[1] = CyDmaTdAllocate();
	CyDmaTdSetConfiguration(QCW_CL_DMA_TD[0], 1, QCW_CL_DMA_TD[1], TD_AUTO_EXEC_NEXT);
	CyDmaTdSetConfiguration(QCW_CL_DMA_TD[1], 1, QCW_CL_DMA_TD[0], 0);
	CyDmaTdSetAddress(QCW_CL_DMA_TD[0], LO16((uint32)&ct1_dac_val[2]), LO16((uint32)CT1_dac_Data_PTR));
	CyDmaTdSetAddress(QCW_CL_DMA_TD[1], LO16((uint32)&ct1_dac_val[2]), LO16((uint32)CT1_dac_Data_PTR));
	CyDmaChSetInitialTd(QCW_CL_DMA_Chan, QCW_CL_DMA_TD[0]);
	CyDmaChEnable(QCW_CL_DMA_Chan, 1);

	/* Variable declarations for TR1_CL_DMA */
	/* Move these variable declarations to the top of the function */
	uint8 TR1_CL_DMA_Chan;
	uint8 TR1_CL_DMA_TD[2];
	TR1_CL_DMA_Chan = TR1_CL_DMA_DmaInitialize(TR1_CL_DMA_BYTES_PER_BURST, TR1_CL_DMA_REQUEST_PER_BURST,
											   HI16(TR1_CL_DMA_SRC_BASE), HI16(TR1_CL_DMA_DST_BASE));
	TR1_CL_DMA_TD[0] = CyDmaTdAllocate();
	TR1_CL_DMA_TD[1] = CyDmaTdAllocate();
	CyDmaTdSetConfiguration(TR1_CL_DMA_TD[0], 1, TR1_CL_DMA_TD[1], TD_AUTO_EXEC_NEXT);
	CyDmaTdSetConfiguration(TR1_CL_DMA_TD[1], 1, TR1_CL_DMA_TD[0], 0);
	CyDmaTdSetAddress(TR1_CL_DMA_TD[0], LO16((uint32)&ct1_dac_val[0]), LO16((uint32)CT1_dac_Data_PTR));
	CyDmaTdSetAddress(TR1_CL_DMA_TD[1], LO16((uint32)&ct1_dac_val[0]), LO16((uint32)CT1_dac_Data_PTR));
	CyDmaChSetInitialTd(TR1_CL_DMA_Chan, TR1_CL_DMA_TD[0]);
	CyDmaChEnable(TR1_CL_DMA_Chan, 1);
}
