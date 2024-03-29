/*******************************************************************************
* File Name: project.h
* 
* PSoC Creator  4.4
*
* Description:
* It contains references to all generated header files and should not be modified.
* This file is automatically generated by PSoC Creator.
*
********************************************************************************
* Copyright (c) 2007-2020 Cypress Semiconductor.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
********************************************************************************/

#include "sim_hw.h"
#include "UART.h"
#include "cyfitter.h"

/*
#include "cyfitter_cfg.h"
#include "cydevice.h"
#include "cydevice_trm.h"
#include "CyPWMCLK.h"
#include "Cysamp_clk.h"
#include "CyINTCLK.h"
#include "cyfitter.h"
#include "cydisabledsheets.h"
#include "GD2A_aliases.h"
#include "GD2A.h"
#include "therm2_aliases.h"
#include "therm2.h"
#include "fram_to_PWMA_DMA_dma.h"
#include "ADC_data_ready.h"
#include "ADC_DMA_dma.h"
#include "UVLO_aliases.h"
#include "UVLO.h"
#include "TP6_aliases.h"
#include "TP6.h"
#include "LED1_aliases.h"
#include "LED1.h"
#include "Therm_Mux.h"
#include "Fan_aliases.h"
#include "Fan.h"
#include "ADC.h"
#include "filter_to_fram_DMA_dma.h"
#include "interrupter1.h"
#include "therm1_aliases.h"
#include "therm1.h"
#include "GD1A_aliases.h"
#include "GD1A.h"
#include "PWMB.h"
#include "ZCDB_aliases.h"
#include "ZCDB.h"
#include "LED2_aliases.h"
#include "LED2.h"
#include "ram_to_filter_DMA_dma.h"
#include "FB_Filter.h"
#include "FB_Filter_PVT.h"
#include "FBC_to_ram_DMA_dma.h"
#include "CT1_dac.h"
#include "LED4_aliases.h"
#include "LED4.h"
#include "ZCD_compB.h"
#include "interrupter1_control.h"
#include "DEBUG_DA_aliases.h"
#include "DEBUG_DA.h"
#include "int1_dma_dma.h"
#include "FB_capture.h"
#include "EEPROM_1.h"
#include "LED3_aliases.h"
#include "LED3.h"
#include "Relay2_aliases.h"
#include "Relay2.h"
#include "CT1_comp.h"
#include "PWMB_PSB_DMA_dma.h"
#include "Relay1_aliases.h"
#include "Relay1.h"
#include "GD2B_aliases.h"
#include "GD2B.h"
#include "QCW_enable.h"
#include "FB_glitch_detect.h"
#include "ZCDref.h"
#include "ADC_peak.h"
#include "QCW_CL_DMA_dma.h"
#include "TR1_CL_DMA_dma.h"
#include "system_fault.h"
#include "MUX_DMA_dma.h"
#include "IDAC_therm.h"
#include "Ibus_aliases.h"
#include "Ibus.h"
#include "Vbus_aliases.h"
#include "Vbus.h"
#include "ADC_therm.h"
#include "CTout_aliases.h"
#include "CTout.h"
#include "PWMA_init_DMA_dma.h"
#include "PWMA.h"
#include "GD1B_aliases.h"
#include "GD1B.h"
#include "Vin_aliases.h"
#include "Vin.h"
#include "ZCD_counter.h"
#include "ZCDA_aliases.h"
#include "ZCDA.h"
#include "ZCD_compA.h"
#include "PSBINIT_DMA_dma.h"
#include "UVLO_status.h"
#include "OCD.h"
#include "UART.h"
#include "USBMIDI_1.h"
#include "USBMIDI_1_audio.h"
#include "USBMIDI_1_cdc.h"
#include "USBMIDI_1_hid.h"
#include "USBMIDI_1_midi.h"
#include "USBMIDI_1_pvt.h"
#include "USBMIDI_1_cydmac.h"
#include "USBMIDI_1_msc.h"
#include "Comp_1.h"
#include "DEBUG_DAC.h"
#include "Sample_Hold_1.h"
#include "Amux_Ctrl.h"
#include "CT_MUX.h"
#include "CT_SEC_aliases.h"
#include "CT_SEC.h"
#include "uart_rx.h"
#include "uart_tx.h"
#include "SG_Timer.h"
#include "isr_midi.h"
#include "Bootloadable.h"
#include "UART_CLK.h"
#include "no_fb_reg.h"
#include "Vdriver_aliases.h"
#include "Vdriver.h"
#include "Disp.h"
#include "Disp_fonts.h"
#include "ws2812_aliases.h"
#include "ws2812.h"
#include "IVO_UART.h"
#include "Synth_Clock.h"
#include "Relay3_aliases.h"
#include "Relay3.h"
#include "Relay4_aliases.h"
#include "Relay4.h"
#include "Opamp_1.h"
#include "ADC_DMA_peak_dma.h"
#include "ADC_peak_ready.h"
#include "Ch1_DMA_dma.h"
#include "Ch2_DMA_dma.h"
#include "Ext_Interrupter_aliases.h"
#include "Ext_Interrupter.h"
#include "DDS32_1.h"
#include "DDS32_2.h"
#include "Ch3_DMA_dma.h"
#include "Ch4_DMA_dma.h"
#include "SDA_1_aliases.h"
#include "SDA_1.h"
#include "SCL_1_aliases.h"
#include "SCL_1.h"
#include "I2C.h"
#include "I2C_PVT.h"
#include "OnTimeCounter.h"
#include "ADC_IRQ.h"
#include "ADC_theACLK.h"
#include "ADC_Bypass_aliases.h"
#include "ADC_Bypass.h"
#include "ADC_peak_IRQ.h"
#include "ADC_peak_theACLK.h"
#include "ADC_peak_Bypass_aliases.h"
#include "ADC_peak_Bypass.h"
#include "ADC_therm_AMux.h"
#include "ADC_therm_Ext_CP_Clk.h"
#include "ADC_therm_theACLK.h"
#include "USBMIDI_1_Dm_aliases.h"
#include "USBMIDI_1_Dm.h"
#include "USBMIDI_1_Dp_aliases.h"
#include "USBMIDI_1_Dp.h"
#include "Disp_fisr.h"
#include "Disp_Clock.h"
#include "Disp_StringSel.h"
#include "Disp_cisr.h"
#include "core_cm3_psoc5.h"
#include "CyDmac.h"
#include "CyFlash.h"
#include "CyLib.h"
#include "cypins.h"
#include "cyPm.h"
#include "CySpc.h"
#include "cytypes.h"
#include "cy_em_eeprom.h"
*/
/*[]*/

