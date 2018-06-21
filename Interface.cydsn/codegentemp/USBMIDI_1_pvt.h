/***************************************************************************//**
* \file .h
* \version 3.10
*
* \brief
*  This file provides private function prototypes and constants for the 
*  USBFS component. It is not intended to be used in the user project.
*
********************************************************************************
* \copyright
* Copyright 2013-2016, Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_USBFS_USBMIDI_1_pvt_H)
#define CY_USBFS_USBMIDI_1_pvt_H

#include "USBMIDI_1.h"
   
#ifdef USBMIDI_1_ENABLE_AUDIO_CLASS
    #include "USBMIDI_1_audio.h"
#endif /* USBMIDI_1_ENABLE_AUDIO_CLASS */

#ifdef USBMIDI_1_ENABLE_CDC_CLASS
    #include "USBMIDI_1_cdc.h"
#endif /* USBMIDI_1_ENABLE_CDC_CLASS */

#if (USBMIDI_1_ENABLE_MIDI_CLASS)
    #include "USBMIDI_1_midi.h"
#endif /* (USBMIDI_1_ENABLE_MIDI_CLASS) */

#if (USBMIDI_1_ENABLE_MSC_CLASS)
    #include "USBMIDI_1_msc.h"
#endif /* (USBMIDI_1_ENABLE_MSC_CLASS) */

#if (USBMIDI_1_EP_MANAGEMENT_DMA)
    #if (CY_PSOC4)
        #include <CyDMA.h>
    #else
        #include <CyDmac.h>
        #if ((USBMIDI_1_EP_MANAGEMENT_DMA_AUTO) && (USBMIDI_1_EP_DMA_AUTO_OPT == 0u))
            #include "USBMIDI_1_EP_DMA_Done_isr.h"
            #include "USBMIDI_1_EP8_DMA_Done_SR.h"
            #include "USBMIDI_1_EP17_DMA_Done_SR.h"
        #endif /* ((USBMIDI_1_EP_MANAGEMENT_DMA_AUTO) && (USBMIDI_1_EP_DMA_AUTO_OPT == 0u)) */
    #endif /* (CY_PSOC4) */
#endif /* (USBMIDI_1_EP_MANAGEMENT_DMA) */

#if (USBMIDI_1_DMA1_ACTIVE)
    #include "USBMIDI_1_ep1_dma.h"
    #define USBMIDI_1_EP1_DMA_CH     (USBMIDI_1_ep1_dma_CHANNEL)
#endif /* (USBMIDI_1_DMA1_ACTIVE) */

#if (USBMIDI_1_DMA2_ACTIVE)
    #include "USBMIDI_1_ep2_dma.h"
    #define USBMIDI_1_EP2_DMA_CH     (USBMIDI_1_ep2_dma_CHANNEL)
#endif /* (USBMIDI_1_DMA2_ACTIVE) */

#if (USBMIDI_1_DMA3_ACTIVE)
    #include "USBMIDI_1_ep3_dma.h"
    #define USBMIDI_1_EP3_DMA_CH     (USBMIDI_1_ep3_dma_CHANNEL)
#endif /* (USBMIDI_1_DMA3_ACTIVE) */

#if (USBMIDI_1_DMA4_ACTIVE)
    #include "USBMIDI_1_ep4_dma.h"
    #define USBMIDI_1_EP4_DMA_CH     (USBMIDI_1_ep4_dma_CHANNEL)
#endif /* (USBMIDI_1_DMA4_ACTIVE) */

#if (USBMIDI_1_DMA5_ACTIVE)
    #include "USBMIDI_1_ep5_dma.h"
    #define USBMIDI_1_EP5_DMA_CH     (USBMIDI_1_ep5_dma_CHANNEL)
#endif /* (USBMIDI_1_DMA5_ACTIVE) */

#if (USBMIDI_1_DMA6_ACTIVE)
    #include "USBMIDI_1_ep6_dma.h"
    #define USBMIDI_1_EP6_DMA_CH     (USBMIDI_1_ep6_dma_CHANNEL)
#endif /* (USBMIDI_1_DMA6_ACTIVE) */

#if (USBMIDI_1_DMA7_ACTIVE)
    #include "USBMIDI_1_ep7_dma.h"
    #define USBMIDI_1_EP7_DMA_CH     (USBMIDI_1_ep7_dma_CHANNEL)
#endif /* (USBMIDI_1_DMA7_ACTIVE) */

#if (USBMIDI_1_DMA8_ACTIVE)
    #include "USBMIDI_1_ep8_dma.h"
    #define USBMIDI_1_EP8_DMA_CH     (USBMIDI_1_ep8_dma_CHANNEL)
#endif /* (USBMIDI_1_DMA8_ACTIVE) */


/***************************************
*     Private Variables
***************************************/

/* Generated external references for descriptors. */
extern const uint8 CYCODE USBMIDI_1_DEVICE0_DESCR[18u];
extern const uint8 CYCODE USBMIDI_1_DEVICE0_CONFIGURATION0_DESCR[175u];
extern const T_USBMIDI_1_EP_SETTINGS_BLOCK CYCODE USBMIDI_1_DEVICE0_CONFIGURATION0_EP_SETTINGS_TABLE[5u];
extern const uint8 CYCODE USBMIDI_1_DEVICE0_CONFIGURATION0_INTERFACE_CLASS[4u];
extern const T_USBMIDI_1_LUT CYCODE USBMIDI_1_DEVICE0_CONFIGURATION0_TABLE[7u];
extern const T_USBMIDI_1_LUT CYCODE USBMIDI_1_DEVICE0_TABLE[3u];
extern const T_USBMIDI_1_LUT CYCODE USBMIDI_1_TABLE[1u];
extern const uint8 CYCODE USBMIDI_1_SN_STRING_DESCRIPTOR[2];
extern const uint8 CYCODE USBMIDI_1_STRING_DESCRIPTORS[421u];


extern const uint8 CYCODE USBMIDI_1_MSOS_DESCRIPTOR[USBMIDI_1_MSOS_DESCRIPTOR_LENGTH];
extern const uint8 CYCODE USBMIDI_1_MSOS_CONFIGURATION_DESCR[USBMIDI_1_MSOS_CONF_DESCR_LENGTH];
#if defined(USBMIDI_1_ENABLE_IDSN_STRING)
    extern uint8 USBMIDI_1_idSerialNumberStringDescriptor[USBMIDI_1_IDSN_DESCR_LENGTH];
#endif /* (USBMIDI_1_ENABLE_IDSN_STRING) */

extern volatile uint8 USBMIDI_1_interfaceNumber;
extern volatile uint8 USBMIDI_1_interfaceSetting[USBMIDI_1_MAX_INTERFACES_NUMBER];
extern volatile uint8 USBMIDI_1_interfaceSettingLast[USBMIDI_1_MAX_INTERFACES_NUMBER];
extern volatile uint8 USBMIDI_1_deviceAddress;
extern volatile uint8 USBMIDI_1_interfaceStatus[USBMIDI_1_MAX_INTERFACES_NUMBER];
extern const uint8 CYCODE *USBMIDI_1_interfaceClass;

extern volatile T_USBMIDI_1_EP_CTL_BLOCK USBMIDI_1_EP[USBMIDI_1_MAX_EP];
extern volatile T_USBMIDI_1_TD USBMIDI_1_currentTD;

#if (USBMIDI_1_EP_MANAGEMENT_DMA)
    #if (CY_PSOC4)
        extern const uint8 USBMIDI_1_DmaChan[USBMIDI_1_MAX_EP];
    #else
        extern uint8 USBMIDI_1_DmaChan[USBMIDI_1_MAX_EP];
        extern uint8 USBMIDI_1_DmaTd  [USBMIDI_1_MAX_EP];
    #endif /* (CY_PSOC4) */
#endif /* (USBMIDI_1_EP_MANAGEMENT_DMA) */

#if (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO)
#if (CY_PSOC4)
    extern uint8  USBMIDI_1_DmaEpBurstCnt   [USBMIDI_1_MAX_EP];
    extern uint8  USBMIDI_1_DmaEpLastBurstEl[USBMIDI_1_MAX_EP];

    extern uint8  USBMIDI_1_DmaEpBurstCntBackup  [USBMIDI_1_MAX_EP];
    extern uint32 USBMIDI_1_DmaEpBufferAddrBackup[USBMIDI_1_MAX_EP];
    
    extern const uint8 USBMIDI_1_DmaReqOut     [USBMIDI_1_MAX_EP];    
    extern const uint8 USBMIDI_1_DmaBurstEndOut[USBMIDI_1_MAX_EP];
#else
    #if (USBMIDI_1_EP_DMA_AUTO_OPT == 0u)
        extern uint8 USBMIDI_1_DmaNextTd[USBMIDI_1_MAX_EP];
        extern volatile uint16 USBMIDI_1_inLength [USBMIDI_1_MAX_EP];
        extern volatile uint8  USBMIDI_1_inBufFull[USBMIDI_1_MAX_EP];
        extern const uint8 USBMIDI_1_epX_TD_TERMOUT_EN[USBMIDI_1_MAX_EP];
        extern const uint8 *USBMIDI_1_inDataPointer[USBMIDI_1_MAX_EP];
    #endif /* (USBMIDI_1_EP_DMA_AUTO_OPT == 0u) */
#endif /* CY_PSOC4 */
#endif /* (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO) */

extern volatile uint8 USBMIDI_1_ep0Toggle;
extern volatile uint8 USBMIDI_1_lastPacketSize;
extern volatile uint8 USBMIDI_1_ep0Mode;
extern volatile uint8 USBMIDI_1_ep0Count;
extern volatile uint16 USBMIDI_1_transferByteCount;


/***************************************
*     Private Function Prototypes
***************************************/
void  USBMIDI_1_ReInitComponent(void)            ;
void  USBMIDI_1_HandleSetup(void)                ;
void  USBMIDI_1_HandleIN(void)                   ;
void  USBMIDI_1_HandleOUT(void)                  ;
void  USBMIDI_1_LoadEP0(void)                    ;
uint8 USBMIDI_1_InitControlRead(void)            ;
uint8 USBMIDI_1_InitControlWrite(void)           ;
void  USBMIDI_1_ControlReadDataStage(void)       ;
void  USBMIDI_1_ControlReadStatusStage(void)     ;
void  USBMIDI_1_ControlReadPrematureStatus(void) ;
uint8 USBMIDI_1_InitControlWrite(void)           ;
uint8 USBMIDI_1_InitZeroLengthControlTransfer(void) ;
void  USBMIDI_1_ControlWriteDataStage(void)      ;
void  USBMIDI_1_ControlWriteStatusStage(void)    ;
void  USBMIDI_1_ControlWritePrematureStatus(void);
uint8 USBMIDI_1_InitNoDataControlTransfer(void)  ;
void  USBMIDI_1_NoDataControlStatusStage(void)   ;
void  USBMIDI_1_InitializeStatusBlock(void)      ;
void  USBMIDI_1_UpdateStatusBlock(uint8 completionCode) ;
uint8 USBMIDI_1_DispatchClassRqst(void)          ;

void USBMIDI_1_Config(uint8 clearAltSetting) ;
void USBMIDI_1_ConfigAltChanged(void)        ;
void USBMIDI_1_ConfigReg(void)               ;
void USBMIDI_1_EpStateInit(void)             ;


const T_USBMIDI_1_LUT CYCODE *USBMIDI_1_GetConfigTablePtr(uint8 confIndex);
const T_USBMIDI_1_LUT CYCODE *USBMIDI_1_GetDeviceTablePtr(void)           ;
#if (USBMIDI_1_BOS_ENABLE)
    const T_USBMIDI_1_LUT CYCODE *USBMIDI_1_GetBOSPtr(void)               ;
#endif /* (USBMIDI_1_BOS_ENABLE) */
const uint8 CYCODE *USBMIDI_1_GetInterfaceClassTablePtr(void)                    ;uint8 USBMIDI_1_ClearEndpointHalt(void)                                          ;
uint8 USBMIDI_1_SetEndpointHalt(void)                                            ;
uint8 USBMIDI_1_ValidateAlternateSetting(void)                                   ;

void USBMIDI_1_SaveConfig(void)      ;
void USBMIDI_1_RestoreConfig(void)   ;

#if (CY_PSOC3 || CY_PSOC5LP)
    #if (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO && (USBMIDI_1_EP_DMA_AUTO_OPT == 0u))
        void USBMIDI_1_LoadNextInEP(uint8 epNumber, uint8 mode)  ;
    #endif /* (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO && (USBMIDI_1_EP_DMA_AUTO_OPT == 0u)) */
#endif /* (CY_PSOC3 || CY_PSOC5LP) */

#if defined(USBMIDI_1_ENABLE_IDSN_STRING)
    void USBMIDI_1_ReadDieID(uint8 descr[])  ;
#endif /* USBMIDI_1_ENABLE_IDSN_STRING */

#if defined(USBMIDI_1_ENABLE_HID_CLASS)
    uint8 USBMIDI_1_DispatchHIDClassRqst(void) ;
#endif /* (USBMIDI_1_ENABLE_HID_CLASS) */

#if defined(USBMIDI_1_ENABLE_AUDIO_CLASS)
    uint8 USBMIDI_1_DispatchAUDIOClassRqst(void) ;
#endif /* (USBMIDI_1_ENABLE_AUDIO_CLASS) */

#if defined(USBMIDI_1_ENABLE_CDC_CLASS)
    uint8 USBMIDI_1_DispatchCDCClassRqst(void) ;
#endif /* (USBMIDI_1_ENABLE_CDC_CLASS) */

#if (USBMIDI_1_ENABLE_MSC_CLASS)
    #if (USBMIDI_1_HANDLE_MSC_REQUESTS)
        uint8 USBMIDI_1_DispatchMSCClassRqst(void) ;
    #endif /* (USBMIDI_1_HANDLE_MSC_REQUESTS) */
#endif /* (USBMIDI_1_ENABLE_MSC_CLASS */

CY_ISR_PROTO(USBMIDI_1_EP_0_ISR);
CY_ISR_PROTO(USBMIDI_1_BUS_RESET_ISR);

#if (USBMIDI_1_SOF_ISR_ACTIVE)
    CY_ISR_PROTO(USBMIDI_1_SOF_ISR);
#endif /* (USBMIDI_1_SOF_ISR_ACTIVE) */

#if (USBMIDI_1_EP1_ISR_ACTIVE)
    CY_ISR_PROTO(USBMIDI_1_EP_1_ISR);
#endif /* (USBMIDI_1_EP1_ISR_ACTIVE) */

#if (USBMIDI_1_EP2_ISR_ACTIVE)
    CY_ISR_PROTO(USBMIDI_1_EP_2_ISR);
#endif /* (USBMIDI_1_EP2_ISR_ACTIVE) */

#if (USBMIDI_1_EP3_ISR_ACTIVE)
    CY_ISR_PROTO(USBMIDI_1_EP_3_ISR);
#endif /* (USBMIDI_1_EP3_ISR_ACTIVE) */

#if (USBMIDI_1_EP4_ISR_ACTIVE)
    CY_ISR_PROTO(USBMIDI_1_EP_4_ISR);
#endif /* (USBMIDI_1_EP4_ISR_ACTIVE) */

#if (USBMIDI_1_EP5_ISR_ACTIVE)
    CY_ISR_PROTO(USBMIDI_1_EP_5_ISR);
#endif /* (USBMIDI_1_EP5_ISR_ACTIVE) */

#if (USBMIDI_1_EP6_ISR_ACTIVE)
    CY_ISR_PROTO(USBMIDI_1_EP_6_ISR);
#endif /* (USBMIDI_1_EP6_ISR_ACTIVE) */

#if (USBMIDI_1_EP7_ISR_ACTIVE)
    CY_ISR_PROTO(USBMIDI_1_EP_7_ISR);
#endif /* (USBMIDI_1_EP7_ISR_ACTIVE) */

#if (USBMIDI_1_EP8_ISR_ACTIVE)
    CY_ISR_PROTO(USBMIDI_1_EP_8_ISR);
#endif /* (USBMIDI_1_EP8_ISR_ACTIVE) */

#if (USBMIDI_1_EP_MANAGEMENT_DMA)
    CY_ISR_PROTO(USBMIDI_1_ARB_ISR);
#endif /* (USBMIDI_1_EP_MANAGEMENT_DMA) */

#if (USBMIDI_1_DP_ISR_ACTIVE)
    CY_ISR_PROTO(USBMIDI_1_DP_ISR);
#endif /* (USBMIDI_1_DP_ISR_ACTIVE) */

#if (CY_PSOC4)
    CY_ISR_PROTO(USBMIDI_1_INTR_HI_ISR);
    CY_ISR_PROTO(USBMIDI_1_INTR_MED_ISR);
    CY_ISR_PROTO(USBMIDI_1_INTR_LO_ISR);
    #if (USBMIDI_1_LPM_ACTIVE)
        CY_ISR_PROTO(USBMIDI_1_LPM_ISR);
    #endif /* (USBMIDI_1_LPM_ACTIVE) */
#endif /* (CY_PSOC4) */

#if (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO)
#if (CY_PSOC4)
    #if (USBMIDI_1_DMA1_ACTIVE)
        void USBMIDI_1_EP1_DMA_DONE_ISR(void);
    #endif /* (USBMIDI_1_DMA1_ACTIVE) */

    #if (USBMIDI_1_DMA2_ACTIVE)
        void USBMIDI_1_EP2_DMA_DONE_ISR(void);
    #endif /* (USBMIDI_1_DMA2_ACTIVE) */

    #if (USBMIDI_1_DMA3_ACTIVE)
        void USBMIDI_1_EP3_DMA_DONE_ISR(void);
    #endif /* (USBMIDI_1_DMA3_ACTIVE) */

    #if (USBMIDI_1_DMA4_ACTIVE)
        void USBMIDI_1_EP4_DMA_DONE_ISR(void);
    #endif /* (USBMIDI_1_DMA4_ACTIVE) */

    #if (USBMIDI_1_DMA5_ACTIVE)
        void USBMIDI_1_EP5_DMA_DONE_ISR(void);
    #endif /* (USBMIDI_1_DMA5_ACTIVE) */

    #if (USBMIDI_1_DMA6_ACTIVE)
        void USBMIDI_1_EP6_DMA_DONE_ISR(void);
    #endif /* (USBMIDI_1_DMA6_ACTIVE) */

    #if (USBMIDI_1_DMA7_ACTIVE)
        void USBMIDI_1_EP7_DMA_DONE_ISR(void);
    #endif /* (USBMIDI_1_DMA7_ACTIVE) */

    #if (USBMIDI_1_DMA8_ACTIVE)
        void USBMIDI_1_EP8_DMA_DONE_ISR(void);
    #endif /* (USBMIDI_1_DMA8_ACTIVE) */

#else
    #if (USBMIDI_1_EP_DMA_AUTO_OPT == 0u)
        CY_ISR_PROTO(USBMIDI_1_EP_DMA_DONE_ISR);
    #endif /* (USBMIDI_1_EP_DMA_AUTO_OPT == 0u) */
#endif /* (CY_PSOC4) */
#endif /* (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO) */


/***************************************
*         Request Handlers
***************************************/

uint8 USBMIDI_1_HandleStandardRqst(void) ;
uint8 USBMIDI_1_DispatchClassRqst(void)  ;
uint8 USBMIDI_1_HandleVendorRqst(void)   ;


/***************************************
*    HID Internal references
***************************************/

#if defined(USBMIDI_1_ENABLE_HID_CLASS)
    void USBMIDI_1_FindReport(void)            ;
    void USBMIDI_1_FindReportDescriptor(void)  ;
    void USBMIDI_1_FindHidClassDecriptor(void) ;
#endif /* USBMIDI_1_ENABLE_HID_CLASS */


/***************************************
*    MIDI Internal references
***************************************/

#if defined(USBMIDI_1_ENABLE_MIDI_STREAMING)
    void USBMIDI_1_MIDI_IN_EP_Service(void)  ;
#endif /* (USBMIDI_1_ENABLE_MIDI_STREAMING) */


/***************************************
*    CDC Internal references
***************************************/

#if defined(USBMIDI_1_ENABLE_CDC_CLASS)

    typedef struct
    {
        uint8  bRequestType;
        uint8  bNotification;
        uint8  wValue;
        uint8  wValueMSB;
        uint8  wIndex;
        uint8  wIndexMSB;
        uint8  wLength;
        uint8  wLengthMSB;
        uint8  wSerialState;
        uint8  wSerialStateMSB;
    } t_USBMIDI_1_cdc_notification;

    uint8 USBMIDI_1_GetInterfaceComPort(uint8 interface) ;
    uint8 USBMIDI_1_Cdc_EpInit( const T_USBMIDI_1_EP_SETTINGS_BLOCK CYCODE *pEP, uint8 epNum, uint8 cdcComNums) ;

    extern volatile uint8  USBMIDI_1_cdc_dataInEpList[USBMIDI_1_MAX_MULTI_COM_NUM];
    extern volatile uint8  USBMIDI_1_cdc_dataOutEpList[USBMIDI_1_MAX_MULTI_COM_NUM];
    extern volatile uint8  USBMIDI_1_cdc_commInEpList[USBMIDI_1_MAX_MULTI_COM_NUM];
#endif /* (USBMIDI_1_ENABLE_CDC_CLASS) */


#endif /* CY_USBFS_USBMIDI_1_pvt_H */


/* [] END OF FILE */
