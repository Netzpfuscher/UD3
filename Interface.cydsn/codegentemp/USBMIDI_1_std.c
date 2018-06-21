/***************************************************************************//**
* \file USBMIDI_1_std.c
* \version 3.10
*
* \brief
*  This file contains the USB Standard request handler.
*
********************************************************************************
* \copyright
* Copyright 2008-2016, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "USBMIDI_1_pvt.h"

/***************************************
*   Static data allocation
***************************************/

#if defined(USBMIDI_1_ENABLE_FWSN_STRING)
    static volatile uint8* USBMIDI_1_fwSerialNumberStringDescriptor;
    static volatile uint8  USBMIDI_1_snStringConfirm = USBMIDI_1_FALSE;
#endif  /* (USBMIDI_1_ENABLE_FWSN_STRING) */

#if defined(USBMIDI_1_ENABLE_FWSN_STRING)
    /***************************************************************************
    * Function Name: USBMIDI_1_SerialNumString
    ************************************************************************//**
    *
    *  This function is available only when the User Call Back option in the 
    *  Serial Number String descriptor properties is selected. Application 
    *  firmware can provide the source of the USB device serial number string 
    *  descriptor during run time. The default string is used if the application 
    *  firmware does not use this function or sets the wrong string descriptor.
    *
    *  \param snString:  Pointer to the user-defined string descriptor. The 
    *  string descriptor should meet the Universal Serial Bus Specification 
    *  revision 2.0 chapter 9.6.7
    *  Offset|Size|Value|Description
    *  ------|----|------|---------------------------------
    *  0     |1   |N     |Size of this descriptor in bytes
    *  1     |1   |0x03  |STRING Descriptor Type
    *  2     |N-2 |Number|UNICODE encoded string
    *  
    * *For example:* uint8 snString[16]={0x0E,0x03,'F',0,'W',0,'S',0,'N',0,'0',0,'1',0};
    *
    * \reentrant
    *  No.
    *
    ***************************************************************************/
    void  USBMIDI_1_SerialNumString(uint8 snString[]) 
    {
        USBMIDI_1_snStringConfirm = USBMIDI_1_FALSE;
        
        if (snString != NULL)
        {
            /* Check descriptor validation */
            if ((snString[0u] > 1u) && (snString[1u] == USBMIDI_1_DESCR_STRING))
            {
                USBMIDI_1_fwSerialNumberStringDescriptor = snString;
                USBMIDI_1_snStringConfirm = USBMIDI_1_TRUE;
            }
        }
    }
#endif  /* USBMIDI_1_ENABLE_FWSN_STRING */


/*******************************************************************************
* Function Name: USBMIDI_1_HandleStandardRqst
****************************************************************************//**
*
*  This Routine dispatches standard requests
*
*
* \return
*  TRUE if request handled.
*
* \reentrant
*  No.
*
*******************************************************************************/
uint8 USBMIDI_1_HandleStandardRqst(void) 
{
    uint8 requestHandled = USBMIDI_1_FALSE;
    uint8 interfaceNumber;
    uint8 configurationN;
    uint8 bmRequestType = USBMIDI_1_bmRequestTypeReg;

#if defined(USBMIDI_1_ENABLE_STRINGS)
    volatile uint8 *pStr = 0u;
    #if defined(USBMIDI_1_ENABLE_DESCRIPTOR_STRINGS)
        uint8 nStr;
        uint8 descrLength;
    #endif /* (USBMIDI_1_ENABLE_DESCRIPTOR_STRINGS) */
#endif /* (USBMIDI_1_ENABLE_STRINGS) */
    
    static volatile uint8 USBMIDI_1_tBuffer[USBMIDI_1_STATUS_LENGTH_MAX];
    const T_USBMIDI_1_LUT CYCODE *pTmp;

    USBMIDI_1_currentTD.count = 0u;

    if (USBMIDI_1_RQST_DIR_D2H == (bmRequestType & USBMIDI_1_RQST_DIR_MASK))
    {
        /* Control Read */
        switch (USBMIDI_1_bRequestReg)
        {
            case USBMIDI_1_GET_DESCRIPTOR:
                if (USBMIDI_1_DESCR_DEVICE ==USBMIDI_1_wValueHiReg)
                {
                    pTmp = USBMIDI_1_GetDeviceTablePtr();
                    USBMIDI_1_currentTD.pData = (volatile uint8 *)pTmp->p_list;
                    USBMIDI_1_currentTD.count = USBMIDI_1_DEVICE_DESCR_LENGTH;
                    
                    requestHandled  = USBMIDI_1_InitControlRead();
                }
                else if (USBMIDI_1_DESCR_CONFIG == USBMIDI_1_wValueHiReg)
                {
                    pTmp = USBMIDI_1_GetConfigTablePtr((uint8) USBMIDI_1_wValueLoReg);
                    
                    /* Verify that requested descriptor exists */
                    if (pTmp != NULL)
                    {
                        USBMIDI_1_currentTD.pData = (volatile uint8 *)pTmp->p_list;
                        USBMIDI_1_currentTD.count = (uint16)((uint16)(USBMIDI_1_currentTD.pData)[USBMIDI_1_CONFIG_DESCR_TOTAL_LENGTH_HI] << 8u) | \
                                                                            (USBMIDI_1_currentTD.pData)[USBMIDI_1_CONFIG_DESCR_TOTAL_LENGTH_LOW];
                        requestHandled  = USBMIDI_1_InitControlRead();
                    }
                }
                
            #if(USBMIDI_1_BOS_ENABLE)
                else if (USBMIDI_1_DESCR_BOS == USBMIDI_1_wValueHiReg)
                {
                    pTmp = USBMIDI_1_GetBOSPtr();
                    
                    /* Verify that requested descriptor exists */
                    if (pTmp != NULL)
                    {
                        USBMIDI_1_currentTD.pData = (volatile uint8 *)pTmp;
                        USBMIDI_1_currentTD.count = ((uint16)((uint16)(USBMIDI_1_currentTD.pData)[USBMIDI_1_BOS_DESCR_TOTAL_LENGTH_HI] << 8u)) | \
                                                                             (USBMIDI_1_currentTD.pData)[USBMIDI_1_BOS_DESCR_TOTAL_LENGTH_LOW];
                        requestHandled  = USBMIDI_1_InitControlRead();
                    }
                }
            #endif /*(USBMIDI_1_BOS_ENABLE)*/
            
            #if defined(USBMIDI_1_ENABLE_STRINGS)
                else if (USBMIDI_1_DESCR_STRING == USBMIDI_1_wValueHiReg)
                {
                /* Descriptor Strings */
                #if defined(USBMIDI_1_ENABLE_DESCRIPTOR_STRINGS)
                    nStr = 0u;
                    pStr = (volatile uint8 *) &USBMIDI_1_STRING_DESCRIPTORS[0u];
                    
                    while ((USBMIDI_1_wValueLoReg > nStr) && (*pStr != 0u))
                    {
                        /* Read descriptor length from 1st byte */
                        descrLength = *pStr;
                        /* Move to next string descriptor */
                        pStr = &pStr[descrLength];
                        nStr++;
                    }
                #endif /* (USBMIDI_1_ENABLE_DESCRIPTOR_STRINGS) */
                
                /* Microsoft OS String */
                #if defined(USBMIDI_1_ENABLE_MSOS_STRING)
                    if (USBMIDI_1_STRING_MSOS == USBMIDI_1_wValueLoReg)
                    {
                        pStr = (volatile uint8 *)& USBMIDI_1_MSOS_DESCRIPTOR[0u];
                    }
                #endif /* (USBMIDI_1_ENABLE_MSOS_STRING) */
                
                /* SN string */
                #if defined(USBMIDI_1_ENABLE_SN_STRING)
                    if ((USBMIDI_1_wValueLoReg != 0u) && 
                        (USBMIDI_1_wValueLoReg == USBMIDI_1_DEVICE0_DESCR[USBMIDI_1_DEVICE_DESCR_SN_SHIFT]))
                    {
                    #if defined(USBMIDI_1_ENABLE_IDSN_STRING)
                        /* Read DIE ID and generate string descriptor in RAM */
                        USBMIDI_1_ReadDieID(USBMIDI_1_idSerialNumberStringDescriptor);
                        pStr = USBMIDI_1_idSerialNumberStringDescriptor;
                    #elif defined(USBMIDI_1_ENABLE_FWSN_STRING)
                        
                        if(USBMIDI_1_snStringConfirm != USBMIDI_1_FALSE)
                        {
                            pStr = USBMIDI_1_fwSerialNumberStringDescriptor;
                        }
                        else
                        {
                            pStr = (volatile uint8 *)&USBMIDI_1_SN_STRING_DESCRIPTOR[0u];
                        }
                    #else
                        pStr = (volatile uint8 *)&USBMIDI_1_SN_STRING_DESCRIPTOR[0u];
                    #endif  /* (USBMIDI_1_ENABLE_IDSN_STRING) */
                    }
                #endif /* (USBMIDI_1_ENABLE_SN_STRING) */
                
                    if (*pStr != 0u)
                    {
                        USBMIDI_1_currentTD.count = *pStr;
                        USBMIDI_1_currentTD.pData = pStr;
                        requestHandled  = USBMIDI_1_InitControlRead();
                    }
                }
            #endif /*  USBMIDI_1_ENABLE_STRINGS */
                else
                {
                    requestHandled = USBMIDI_1_DispatchClassRqst();
                }
                break;
                
            case USBMIDI_1_GET_STATUS:
                switch (bmRequestType & USBMIDI_1_RQST_RCPT_MASK)
                {
                    case USBMIDI_1_RQST_RCPT_EP:
                        USBMIDI_1_currentTD.count = USBMIDI_1_EP_STATUS_LENGTH;
                        USBMIDI_1_tBuffer[0u]     = USBMIDI_1_EP[USBMIDI_1_wIndexLoReg & USBMIDI_1_DIR_UNUSED].hwEpState;
                        USBMIDI_1_tBuffer[1u]     = 0u;
                        USBMIDI_1_currentTD.pData = &USBMIDI_1_tBuffer[0u];
                        
                        requestHandled  = USBMIDI_1_InitControlRead();
                        break;
                    case USBMIDI_1_RQST_RCPT_DEV:
                        USBMIDI_1_currentTD.count = USBMIDI_1_DEVICE_STATUS_LENGTH;
                        USBMIDI_1_tBuffer[0u]     = USBMIDI_1_deviceStatus;
                        USBMIDI_1_tBuffer[1u]     = 0u;
                        USBMIDI_1_currentTD.pData = &USBMIDI_1_tBuffer[0u];
                        
                        requestHandled  = USBMIDI_1_InitControlRead();
                        break;
                    default:    /* requestHandled is initialized as FALSE by default */
                        break;
                }
                break;
                
            case USBMIDI_1_GET_CONFIGURATION:
                USBMIDI_1_currentTD.count = 1u;
                USBMIDI_1_currentTD.pData = (volatile uint8 *) &USBMIDI_1_configuration;
                requestHandled  = USBMIDI_1_InitControlRead();
                break;
                
            case USBMIDI_1_GET_INTERFACE:
                USBMIDI_1_currentTD.count = 1u;
                USBMIDI_1_currentTD.pData = (volatile uint8 *) &USBMIDI_1_interfaceSetting[USBMIDI_1_wIndexLoReg];
                requestHandled  = USBMIDI_1_InitControlRead();
                break;
                
            default: /* requestHandled is initialized as FALSE by default */
                break;
        }
    }
    else
    {
        /* Control Write */
        switch (USBMIDI_1_bRequestReg)
        {
            case USBMIDI_1_SET_ADDRESS:
                /* Store address to be set in USBMIDI_1_NoDataControlStatusStage(). */
                USBMIDI_1_deviceAddress = (uint8) USBMIDI_1_wValueLoReg;
                requestHandled = USBMIDI_1_InitNoDataControlTransfer();
                break;
                
            case USBMIDI_1_SET_CONFIGURATION:
                configurationN = USBMIDI_1_wValueLoReg;
                
                /* Verify that configuration descriptor exists */
                if(configurationN > 0u)
                {
                    pTmp = USBMIDI_1_GetConfigTablePtr((uint8) configurationN - 1u);
                }
                
                /* Responds with a Request Error when configuration number is invalid */
                if (((configurationN > 0u) && (pTmp != NULL)) || (configurationN == 0u))
                {
                    /* Set new configuration if it has been changed */
                    if(configurationN != USBMIDI_1_configuration)
                    {
                        USBMIDI_1_configuration = (uint8) configurationN;
                        USBMIDI_1_configurationChanged = USBMIDI_1_TRUE;
                        USBMIDI_1_Config(USBMIDI_1_TRUE);
                    }
                    requestHandled = USBMIDI_1_InitNoDataControlTransfer();
                }
                break;
                
            case USBMIDI_1_SET_INTERFACE:
                if (0u != USBMIDI_1_ValidateAlternateSetting())
                {
                    /* Get interface number from the request. */
                    interfaceNumber = USBMIDI_1_wIndexLoReg;
                    USBMIDI_1_interfaceNumber = (uint8) USBMIDI_1_wIndexLoReg;
                     
                    /* Check if alternate settings is changed for interface. */
                    if (USBMIDI_1_interfaceSettingLast[interfaceNumber] != USBMIDI_1_interfaceSetting[interfaceNumber])
                    {
                        USBMIDI_1_configurationChanged = USBMIDI_1_TRUE;
                    
                        /* Change alternate setting for the endpoints. */
                    #if (USBMIDI_1_EP_MANAGEMENT_MANUAL && USBMIDI_1_EP_ALLOC_DYNAMIC)
                        USBMIDI_1_Config(USBMIDI_1_FALSE);
                    #else
                        USBMIDI_1_ConfigAltChanged();
                    #endif /* (USBMIDI_1_EP_MANAGEMENT_MANUAL && USBMIDI_1_EP_ALLOC_DYNAMIC) */
                    }
                    
                    requestHandled = USBMIDI_1_InitNoDataControlTransfer();
                }
                break;
                
            case USBMIDI_1_CLEAR_FEATURE:
                switch (bmRequestType & USBMIDI_1_RQST_RCPT_MASK)
                {
                    case USBMIDI_1_RQST_RCPT_EP:
                        if (USBMIDI_1_wValueLoReg == USBMIDI_1_ENDPOINT_HALT)
                        {
                            requestHandled = USBMIDI_1_ClearEndpointHalt();
                        }
                        break;
                    case USBMIDI_1_RQST_RCPT_DEV:
                        /* Clear device REMOTE_WAKEUP */
                        if (USBMIDI_1_wValueLoReg == USBMIDI_1_DEVICE_REMOTE_WAKEUP)
                        {
                            USBMIDI_1_deviceStatus &= (uint8)~USBMIDI_1_DEVICE_STATUS_REMOTE_WAKEUP;
                            requestHandled = USBMIDI_1_InitNoDataControlTransfer();
                        }
                        break;
                    case USBMIDI_1_RQST_RCPT_IFC:
                        /* Validate interfaceNumber */
                        if (USBMIDI_1_wIndexLoReg < USBMIDI_1_MAX_INTERFACES_NUMBER)
                        {
                            USBMIDI_1_interfaceStatus[USBMIDI_1_wIndexLoReg] &= (uint8) ~USBMIDI_1_wValueLoReg;
                            requestHandled = USBMIDI_1_InitNoDataControlTransfer();
                        }
                        break;
                    default:    /* requestHandled is initialized as FALSE by default */
                        break;
                }
                break;
                
            case USBMIDI_1_SET_FEATURE:
                switch (bmRequestType & USBMIDI_1_RQST_RCPT_MASK)
                {
                    case USBMIDI_1_RQST_RCPT_EP:
                        if (USBMIDI_1_wValueLoReg == USBMIDI_1_ENDPOINT_HALT)
                        {
                            requestHandled = USBMIDI_1_SetEndpointHalt();
                        }
                        break;
                        
                    case USBMIDI_1_RQST_RCPT_DEV:
                        /* Set device REMOTE_WAKEUP */
                        if (USBMIDI_1_wValueLoReg == USBMIDI_1_DEVICE_REMOTE_WAKEUP)
                        {
                            USBMIDI_1_deviceStatus |= USBMIDI_1_DEVICE_STATUS_REMOTE_WAKEUP;
                            requestHandled = USBMIDI_1_InitNoDataControlTransfer();
                        }
                        break;
                        
                    case USBMIDI_1_RQST_RCPT_IFC:
                        /* Validate interfaceNumber */
                        if (USBMIDI_1_wIndexLoReg < USBMIDI_1_MAX_INTERFACES_NUMBER)
                        {
                            USBMIDI_1_interfaceStatus[USBMIDI_1_wIndexLoReg] &= (uint8) ~USBMIDI_1_wValueLoReg;
                            requestHandled = USBMIDI_1_InitNoDataControlTransfer();
                        }
                        break;
                    
                    default:    /* requestHandled is initialized as FALSE by default */
                        break;
                }
                break;
                
            default:    /* requestHandled is initialized as FALSE by default */
                break;
        }
    }
    
    return (requestHandled);
}


#if defined(USBMIDI_1_ENABLE_IDSN_STRING)
    /***************************************************************************
    * Function Name: USBMIDI_1_ReadDieID
    ************************************************************************//**
    *
    *  This routine read Die ID and generate Serial Number string descriptor.
    *
    *  \param descr:  pointer on string descriptor. This string size has to be equal or
    *          greater than USBMIDI_1_IDSN_DESCR_LENGTH.
    *
    *
    * \reentrant
    *  No.
    *
    ***************************************************************************/
    void USBMIDI_1_ReadDieID(uint8 descr[]) 
    {
        const char8 CYCODE hex[] = "0123456789ABCDEF";
        uint8 i;
        uint8 j = 0u;
        uint8 uniqueId[8u];

        if (NULL != descr)
        {
            /* Initialize descriptor header. */
            descr[0u] = USBMIDI_1_IDSN_DESCR_LENGTH;
            descr[1u] = USBMIDI_1_DESCR_STRING;
            
            /* Unique ID size is 8 bytes. */
            CyGetUniqueId((uint32 *) uniqueId);

            /* Fill descriptor with unique device ID. */
            for (i = 2u; i < USBMIDI_1_IDSN_DESCR_LENGTH; i += 4u)
            {
                descr[i]      = (uint8) hex[(uniqueId[j] >> 4u)];
                descr[i + 1u] = 0u;
                descr[i + 2u] = (uint8) hex[(uniqueId[j] & 0x0Fu)];
                descr[i + 3u] = 0u;
                ++j;
            }
        }
    }
#endif /* (USBMIDI_1_ENABLE_IDSN_STRING) */


/*******************************************************************************
* Function Name: USBMIDI_1_ConfigReg
****************************************************************************//**
*
*  This routine configures hardware registers from the variables.
*  It is called from USBMIDI_1_Config() function and from RestoreConfig
*  after Wakeup.
*
*******************************************************************************/
void USBMIDI_1_ConfigReg(void) 
{
    uint8 ep;

#if (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO)
    uint8 epType = 0u;
#endif /* (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO) */

    /* Go thought all endpoints and set hardware configuration */
    for (ep = USBMIDI_1_EP1; ep < USBMIDI_1_MAX_EP; ++ep)
    {
        USBMIDI_1_ARB_EP_BASE.arbEp[ep].epCfg = USBMIDI_1_ARB_EPX_CFG_DEFAULT;
        
    #if (USBMIDI_1_EP_MANAGEMENT_DMA)
        /* Enable arbiter endpoint interrupt sources */
        USBMIDI_1_ARB_EP_BASE.arbEp[ep].epIntEn = USBMIDI_1_ARB_EPX_INT_MASK;
    #endif /* (USBMIDI_1_EP_MANAGEMENT_DMA) */
    
        if (USBMIDI_1_EP[ep].epMode != USBMIDI_1_MODE_DISABLE)
        {
            if (0u != (USBMIDI_1_EP[ep].addr & USBMIDI_1_DIR_IN))
            {
                USBMIDI_1_SIE_EP_BASE.sieEp[ep].epCr0 = USBMIDI_1_MODE_NAK_IN;
                
            #if (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO && CY_PSOC4)
                /* Clear DMA_TERMIN for IN endpoint. */
                USBMIDI_1_ARB_EP_BASE.arbEp[ep].epIntEn &= (uint32) ~USBMIDI_1_ARB_EPX_INT_DMA_TERMIN;
            #endif /* (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO && CY_PSOC4) */
            }
            else
            {
                USBMIDI_1_SIE_EP_BASE.sieEp[ep].epCr0 = USBMIDI_1_MODE_NAK_OUT;

            #if (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO)
                /* (CY_PSOC4): DMA_TERMIN for OUT endpoint is set above. */
                
                /* Prepare endpoint type mask. */
                epType |= (uint8) (0x01u << (ep - USBMIDI_1_EP1));
            #endif /* (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO) */
            }
        }
        else
        {
            USBMIDI_1_SIE_EP_BASE.sieEp[ep].epCr0 = USBMIDI_1_MODE_STALL_DATA_EP;
        }
        
    #if (!USBMIDI_1_EP_MANAGEMENT_DMA_AUTO)
        #if (CY_PSOC4)
            USBMIDI_1_ARB_EP16_BASE.arbEp[ep].rwRa16  = (uint32) USBMIDI_1_EP[ep].buffOffset;
            USBMIDI_1_ARB_EP16_BASE.arbEp[ep].rwWa16  = (uint32) USBMIDI_1_EP[ep].buffOffset;
        #else
            USBMIDI_1_ARB_EP_BASE.arbEp[ep].rwRa    = LO8(USBMIDI_1_EP[ep].buffOffset);
            USBMIDI_1_ARB_EP_BASE.arbEp[ep].rwRaMsb = HI8(USBMIDI_1_EP[ep].buffOffset);
            USBMIDI_1_ARB_EP_BASE.arbEp[ep].rwWa    = LO8(USBMIDI_1_EP[ep].buffOffset);
            USBMIDI_1_ARB_EP_BASE.arbEp[ep].rwWaMsb = HI8(USBMIDI_1_EP[ep].buffOffset);
        #endif /* (CY_PSOC4) */
    #endif /* (!USBMIDI_1_EP_MANAGEMENT_DMA_AUTO) */
    }

#if (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO)
     /* BUF_SIZE depend on DMA_THRESS value:0x55-32 bytes  0x44-16 bytes 0x33-8 bytes 0x22-4 bytes 0x11-2 bytes */
    USBMIDI_1_BUF_SIZE_REG = USBMIDI_1_DMA_BUF_SIZE;

    /* Configure DMA burst threshold */
#if (CY_PSOC4)
    USBMIDI_1_DMA_THRES16_REG   = USBMIDI_1_DMA_BYTES_PER_BURST;
#else
    USBMIDI_1_DMA_THRES_REG     = USBMIDI_1_DMA_BYTES_PER_BURST;
    USBMIDI_1_DMA_THRES_MSB_REG = 0u;
#endif /* (CY_PSOC4) */
    USBMIDI_1_EP_ACTIVE_REG = USBMIDI_1_DEFAULT_ARB_INT_EN;
    USBMIDI_1_EP_TYPE_REG   = epType;
    
    /* Cfg_cmp bit set to 1 once configuration is complete. */
    /* Lock arbiter configtuation */
    USBMIDI_1_ARB_CFG_REG |= (uint8)  USBMIDI_1_ARB_CFG_CFG_CMP;
    /* Cfg_cmp bit set to 0 during configuration of PFSUSB Registers. */
    USBMIDI_1_ARB_CFG_REG &= (uint8) ~USBMIDI_1_ARB_CFG_CFG_CMP;

#endif /* (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO) */

    /* Enable interrupt SIE interurpt source from EP0-EP1 */
    USBMIDI_1_SIE_EP_INT_EN_REG = (uint8) USBMIDI_1_DEFAULT_SIE_EP_INT_EN;
}


/*******************************************************************************
* Function Name: USBMIDI_1_EpStateInit
****************************************************************************//**
*
*  This routine initialize state of Data end points based of its type: 
*   IN  - USBMIDI_1_IN_BUFFER_EMPTY (USBMIDI_1_EVENT_PENDING)
*   OUT - USBMIDI_1_OUT_BUFFER_EMPTY (USBMIDI_1_NO_EVENT_PENDING)
*
*******************************************************************************/
void USBMIDI_1_EpStateInit(void) 
{
    uint8 i;

    for (i = USBMIDI_1_EP1; i < USBMIDI_1_MAX_EP; i++)
    { 
        if (0u != (USBMIDI_1_EP[i].addr & USBMIDI_1_DIR_IN))
        {
            /* IN Endpoint */
            USBMIDI_1_EP[i].apiEpState = USBMIDI_1_EVENT_PENDING;
        }
        else
        {
            /* OUT Endpoint */
            USBMIDI_1_EP[i].apiEpState = USBMIDI_1_NO_EVENT_PENDING;
        }
    }
                    
}


/*******************************************************************************
* Function Name: USBMIDI_1_Config
****************************************************************************//**
*
*  This routine configures endpoints for the entire configuration by scanning
*  the configuration descriptor.
*
*  \param clearAltSetting: It configures the bAlternateSetting 0 for each interface.
*
* USBMIDI_1_interfaceClass - Initialized class array for each interface.
*   It is used for handling Class specific requests depend on interface class.
*   Different classes in multiple Alternate settings does not supported.
*
* \reentrant
*  No.
*
*******************************************************************************/
void USBMIDI_1_Config(uint8 clearAltSetting) 
{
    uint8 ep;
    uint8 curEp;
    uint8 i;
    uint8 epType;
    const uint8 *pDescr;
    
    #if (!USBMIDI_1_EP_MANAGEMENT_DMA_AUTO)
        uint16 buffCount = 0u;
    #endif /* (!USBMIDI_1_EP_MANAGEMENT_DMA_AUTO) */

    const T_USBMIDI_1_LUT CYCODE *pTmp;
    const T_USBMIDI_1_EP_SETTINGS_BLOCK CYCODE *pEP;

    /* Clear endpoints settings */
    for (ep = 0u; ep < USBMIDI_1_MAX_EP; ++ep)
    {
        USBMIDI_1_EP[ep].attrib     = 0u;
        USBMIDI_1_EP[ep].hwEpState  = 0u;
        USBMIDI_1_EP[ep].epToggle   = 0u;
        USBMIDI_1_EP[ep].bufferSize = 0u;
        USBMIDI_1_EP[ep].interface  = 0u;
        USBMIDI_1_EP[ep].apiEpState = USBMIDI_1_NO_EVENT_PENDING;
        USBMIDI_1_EP[ep].epMode     = USBMIDI_1_MODE_DISABLE;   
    }

    /* Clear Alternate settings for all interfaces. */
    if (0u != clearAltSetting)
    {
        for (i = 0u; i < USBMIDI_1_MAX_INTERFACES_NUMBER; ++i)
        {
            USBMIDI_1_interfaceSetting[i]     = 0u;
            USBMIDI_1_interfaceSettingLast[i] = 0u;
        }
    }

    /* Init Endpoints and Device Status if configured */
    if (USBMIDI_1_configuration > 0u)
    {
        #if defined(USBMIDI_1_ENABLE_CDC_CLASS)
            uint8 cdcComNums = 0u;
        #endif  /* (USBMIDI_1_ENABLE_CDC_CLASS) */  

        pTmp = USBMIDI_1_GetConfigTablePtr(USBMIDI_1_configuration - 1u);
        
        /* Set Power status for current configuration */
        pDescr = (const uint8 *)pTmp->p_list;
        if ((pDescr[USBMIDI_1_CONFIG_DESCR_ATTRIB] & USBMIDI_1_CONFIG_DESCR_ATTRIB_SELF_POWERED) != 0u)
        {
            USBMIDI_1_deviceStatus |= (uint8)  USBMIDI_1_DEVICE_STATUS_SELF_POWERED;
        }
        else
        {
            USBMIDI_1_deviceStatus &= (uint8) ~USBMIDI_1_DEVICE_STATUS_SELF_POWERED;
        }
        
        /* Move to next element */
        pTmp = &pTmp[1u];
        ep = pTmp->c;  /* For this table, c is the number of endpoints configurations  */

        #if (USBMIDI_1_EP_MANAGEMENT_MANUAL && USBMIDI_1_EP_ALLOC_DYNAMIC)
            /* Configure for dynamic EP memory allocation */
            /* p_list points the endpoint setting table. */
            pEP = (T_USBMIDI_1_EP_SETTINGS_BLOCK *) pTmp->p_list;
            
            for (i = 0u; i < ep; i++)
            {     
                /* Compare current Alternate setting with EP Alt */
                if (USBMIDI_1_interfaceSetting[pEP->interface] == pEP->altSetting)
                {                                                          
                    curEp  = pEP->addr & USBMIDI_1_DIR_UNUSED;
                    epType = pEP->attributes & USBMIDI_1_EP_TYPE_MASK;
                    
                    USBMIDI_1_EP[curEp].addr       = pEP->addr;
                    USBMIDI_1_EP[curEp].attrib     = pEP->attributes;
                    USBMIDI_1_EP[curEp].bufferSize = pEP->bufferSize;

                    if (0u != (pEP->addr & USBMIDI_1_DIR_IN))
                    {
                        /* IN Endpoint */
                        USBMIDI_1_EP[curEp].epMode     = USBMIDI_1_GET_ACTIVE_IN_EP_CR0_MODE(epType);
                        USBMIDI_1_EP[curEp].apiEpState = USBMIDI_1_EVENT_PENDING;
                    
                    #if (defined(USBMIDI_1_ENABLE_MIDI_STREAMING) && (USBMIDI_1_MIDI_IN_BUFF_SIZE > 0))
                        if ((pEP->bMisc == USBMIDI_1_CLASS_AUDIO) && (epType == USBMIDI_1_EP_TYPE_BULK))
                        {
                            USBMIDI_1_midi_in_ep = curEp;
                        }
                    #endif  /* (USBMIDI_1_ENABLE_MIDI_STREAMING) */
                    }
                    else
                    {
                        /* OUT Endpoint */
                        USBMIDI_1_EP[curEp].epMode     = USBMIDI_1_GET_ACTIVE_OUT_EP_CR0_MODE(epType);
                        USBMIDI_1_EP[curEp].apiEpState = USBMIDI_1_NO_EVENT_PENDING;
                        
                    #if (defined(USBMIDI_1_ENABLE_MIDI_STREAMING) && (USBMIDI_1_MIDI_OUT_BUFF_SIZE > 0))
                        if ((pEP->bMisc == USBMIDI_1_CLASS_AUDIO) && (epType == USBMIDI_1_EP_TYPE_BULK))
                        {
                            USBMIDI_1_midi_out_ep = curEp;
                        }
                    #endif  /* (USBMIDI_1_ENABLE_MIDI_STREAMING) */
                    }

                #if(defined (USBMIDI_1_ENABLE_CDC_CLASS))
                    if((pEP->bMisc == USBMIDI_1_CLASS_CDC_DATA) ||(pEP->bMisc == USBMIDI_1_CLASS_CDC))
                    {
                        cdcComNums = USBMIDI_1_Cdc_EpInit(pEP, curEp, cdcComNums);
                    }
                #endif  /* (USBMIDI_1_ENABLE_CDC_CLASS) */
                }
                
                pEP = &pEP[1u];
            }
            
        #else
            for (i = USBMIDI_1_EP1; i < USBMIDI_1_MAX_EP; ++i)
            {
                /* p_list points the endpoint setting table. */
                pEP = (const T_USBMIDI_1_EP_SETTINGS_BLOCK CYCODE *) pTmp->p_list;
                /* Find max length for each EP and select it (length could be different in different Alt settings) */
                /* but other settings should be correct with regards to Interface alt Setting */
                
                for (curEp = 0u; curEp < ep; ++curEp)
                {
                    if (i == (pEP->addr & USBMIDI_1_DIR_UNUSED))
                    {
                        /* Compare endpoint buffers size with current size to find greater. */
                        if (USBMIDI_1_EP[i].bufferSize < pEP->bufferSize)
                        {
                            USBMIDI_1_EP[i].bufferSize = pEP->bufferSize;
                        }
                        
                        /* Compare current Alternate setting with EP Alt */
                        if (USBMIDI_1_interfaceSetting[pEP->interface] == pEP->altSetting)
                        {                            
                            USBMIDI_1_EP[i].addr = pEP->addr;
                            USBMIDI_1_EP[i].attrib = pEP->attributes;
                            
                            epType = pEP->attributes & USBMIDI_1_EP_TYPE_MASK;
                            
                            if (0u != (pEP->addr & USBMIDI_1_DIR_IN))
                            {
                                /* IN Endpoint */
                                USBMIDI_1_EP[i].epMode     = USBMIDI_1_GET_ACTIVE_IN_EP_CR0_MODE(epType);
                                USBMIDI_1_EP[i].apiEpState = USBMIDI_1_EVENT_PENDING;
                                
                            #if (defined(USBMIDI_1_ENABLE_MIDI_STREAMING) && (USBMIDI_1_MIDI_IN_BUFF_SIZE > 0))
                                if ((pEP->bMisc == USBMIDI_1_CLASS_AUDIO) && (epType == USBMIDI_1_EP_TYPE_BULK))
                                {
                                    USBMIDI_1_midi_in_ep = i;
                                }
                            #endif  /* (USBMIDI_1_ENABLE_MIDI_STREAMING) */
                            }
                            else
                            {
                                /* OUT Endpoint */
                                USBMIDI_1_EP[i].epMode     = USBMIDI_1_GET_ACTIVE_OUT_EP_CR0_MODE(epType);
                                USBMIDI_1_EP[i].apiEpState = USBMIDI_1_NO_EVENT_PENDING;
                                
                            #if (defined(USBMIDI_1_ENABLE_MIDI_STREAMING) && (USBMIDI_1_MIDI_OUT_BUFF_SIZE > 0))
                                if ((pEP->bMisc == USBMIDI_1_CLASS_AUDIO) && (epType == USBMIDI_1_EP_TYPE_BULK))
                                {
                                    USBMIDI_1_midi_out_ep = i;
                                }
                            #endif  /* (USBMIDI_1_ENABLE_MIDI_STREAMING) */
                            }

                        #if (defined(USBMIDI_1_ENABLE_CDC_CLASS))
                            if((pEP->bMisc == USBMIDI_1_CLASS_CDC_DATA) ||(pEP->bMisc == USBMIDI_1_CLASS_CDC))
                            {
                                cdcComNums = USBMIDI_1_Cdc_EpInit(pEP, i, cdcComNums);
                            }
                        #endif  /* (USBMIDI_1_ENABLE_CDC_CLASS) */

                            #if (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO)
                                break;  /* Use first EP setting in Auto memory management */
                            #endif /* (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO) */
                        }
                    }
                    
                    pEP = &pEP[1u];
                }
            }
        #endif /*  (USBMIDI_1_EP_MANAGEMENT_MANUAL && USBMIDI_1_EP_ALLOC_DYNAMIC) */

        /* Init class array for each interface and interface number for each EP.
        *  It is used for handling Class specific requests directed to either an
        *  interface or the endpoint.
        */
        /* p_list points the endpoint setting table. */
        pEP = (const T_USBMIDI_1_EP_SETTINGS_BLOCK CYCODE *) pTmp->p_list;
        for (i = 0u; i < ep; i++)
        {
            /* Configure interface number for each EP */
            USBMIDI_1_EP[pEP->addr & USBMIDI_1_DIR_UNUSED].interface = pEP->interface;
            pEP = &pEP[1u];
        }
        
        /* Init pointer on interface class table */
        USBMIDI_1_interfaceClass = USBMIDI_1_GetInterfaceClassTablePtr();
        
    /* Set the endpoint buffer addresses */
    #if (!USBMIDI_1_EP_MANAGEMENT_DMA_AUTO)
        buffCount = 0u;
        for (ep = USBMIDI_1_EP1; ep < USBMIDI_1_MAX_EP; ++ep)
        {
            USBMIDI_1_EP[ep].buffOffset = buffCount;        
            buffCount += USBMIDI_1_EP[ep].bufferSize;
            
        #if (USBMIDI_1_GEN_16BITS_EP_ACCESS)
            /* Align EP buffers to be event size to access 16-bits DR register. */
            buffCount += (0u != (buffCount & 0x01u)) ? 1u : 0u;
        #endif /* (USBMIDI_1_GEN_16BITS_EP_ACCESS) */            
        }
    #endif /* (!USBMIDI_1_EP_MANAGEMENT_DMA_AUTO) */

        /* Configure hardware registers */
        USBMIDI_1_ConfigReg();
    }
}


/*******************************************************************************
* Function Name: USBMIDI_1_ConfigAltChanged
****************************************************************************//**
*
*  This routine update configuration for the required endpoints only.
*  It is called after SET_INTERFACE request when Static memory allocation used.
*
* \reentrant
*  No.
*
*******************************************************************************/
void USBMIDI_1_ConfigAltChanged(void) 
{
    uint8 ep;
    uint8 curEp;
    uint8 epType;
    uint8 i;
    uint8 interfaceNum;

    const T_USBMIDI_1_LUT CYCODE *pTmp;
    const T_USBMIDI_1_EP_SETTINGS_BLOCK CYCODE *pEP;

    /* Init Endpoints and Device Status if configured */
    if (USBMIDI_1_configuration > 0u)
    {
        /* Get number of endpoints configurations (ep). */
        pTmp = USBMIDI_1_GetConfigTablePtr(USBMIDI_1_configuration - 1u);
        pTmp = &pTmp[1u];
        ep = pTmp->c;

        /* Get pointer to endpoints setting table (pEP). */
        pEP = (const T_USBMIDI_1_EP_SETTINGS_BLOCK CYCODE *) pTmp->p_list;
        
        /* Look through all possible endpoint configurations. Find endpoints 
        * which belong to current interface and alternate settings for 
        * re-configuration.
        */
        interfaceNum = USBMIDI_1_interfaceNumber;
        for (i = 0u; i < ep; i++)
        {
            /* Find endpoints which belong to current interface and alternate settings. */
            if ((interfaceNum == pEP->interface) && 
                (USBMIDI_1_interfaceSetting[interfaceNum] == pEP->altSetting))
            {
                curEp  = ((uint8) pEP->addr & USBMIDI_1_DIR_UNUSED);
                epType = ((uint8) pEP->attributes & USBMIDI_1_EP_TYPE_MASK);
                
                /* Change the SIE mode for the selected EP to NAK ALL */
                USBMIDI_1_EP[curEp].epToggle   = 0u;
                USBMIDI_1_EP[curEp].addr       = pEP->addr;
                USBMIDI_1_EP[curEp].attrib     = pEP->attributes;
                USBMIDI_1_EP[curEp].bufferSize = pEP->bufferSize;

                if (0u != (pEP->addr & USBMIDI_1_DIR_IN))
                {
                    /* IN Endpoint */
                    USBMIDI_1_EP[curEp].epMode     = USBMIDI_1_GET_ACTIVE_IN_EP_CR0_MODE(epType);
                    USBMIDI_1_EP[curEp].apiEpState = USBMIDI_1_EVENT_PENDING;
                }
                else
                {
                    /* OUT Endpoint */
                    USBMIDI_1_EP[curEp].epMode     = USBMIDI_1_GET_ACTIVE_OUT_EP_CR0_MODE(epType);
                    USBMIDI_1_EP[curEp].apiEpState = USBMIDI_1_NO_EVENT_PENDING;
                }
                
                /* Make SIE to NAK any endpoint requests */
                USBMIDI_1_SIE_EP_BASE.sieEp[curEp].epCr0 = USBMIDI_1_MODE_NAK_IN_OUT;

            #if (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO)
                /* Clear IN data ready. */
                USBMIDI_1_ARB_EP_BASE.arbEp[curEp].epCfg &= (uint8) ~USBMIDI_1_ARB_EPX_CFG_IN_DATA_RDY;

                /* Select endpoint number of reconfiguration */
                USBMIDI_1_DYN_RECONFIG_REG = (uint8) ((curEp - 1u) << USBMIDI_1_DYN_RECONFIG_EP_SHIFT);
                
                /* Request for dynamic re-configuration of endpoint. */
                USBMIDI_1_DYN_RECONFIG_REG |= USBMIDI_1_DYN_RECONFIG_ENABLE;
                
                /* Wait until block is ready for re-configuration */
                while (0u == (USBMIDI_1_DYN_RECONFIG_REG & USBMIDI_1_DYN_RECONFIG_RDY_STS))
                {
                }
                
                /* Once DYN_RECONFIG_RDY_STS bit is set, FW can change the EP configuration. */
                /* Change EP Type with new direction */
                if (0u != (pEP->addr & USBMIDI_1_DIR_IN))
                {
                    /* Set endpoint type: 0 - IN and 1 - OUT. */
                    USBMIDI_1_EP_TYPE_REG &= (uint8) ~(uint8)((uint8) 0x01u << (curEp - 1u));
                    
                #if (CY_PSOC4)
                    /* Clear DMA_TERMIN for IN endpoint */
                    USBMIDI_1_ARB_EP_BASE.arbEp[curEp].epIntEn &= (uint32) ~USBMIDI_1_ARB_EPX_INT_DMA_TERMIN;
                #endif /* (CY_PSOC4) */
                }
                else
                {
                    /* Set endpoint type: 0 - IN and 1- OUT. */
                    USBMIDI_1_EP_TYPE_REG |= (uint8) ((uint8) 0x01u << (curEp - 1u));
                    
                #if (CY_PSOC4)
                    /* Set DMA_TERMIN for OUT endpoint */
                    USBMIDI_1_ARB_EP_BASE.arbEp[curEp].epIntEn |= (uint32) USBMIDI_1_ARB_EPX_INT_DMA_TERMIN;
                #endif /* (CY_PSOC4) */
                }
                
                /* Complete dynamic re-configuration: all endpoint related status and signals 
                * are set into the default state.
                */
                USBMIDI_1_DYN_RECONFIG_REG &= (uint8) ~USBMIDI_1_DYN_RECONFIG_ENABLE;

            #else
                USBMIDI_1_SIE_EP_BASE.sieEp[curEp].epCnt0 = HI8(USBMIDI_1_EP[curEp].bufferSize);
                USBMIDI_1_SIE_EP_BASE.sieEp[curEp].epCnt1 = LO8(USBMIDI_1_EP[curEp].bufferSize);
                
                #if (CY_PSOC4)
                    USBMIDI_1_ARB_EP16_BASE.arbEp[curEp].rwRa16  = (uint32) USBMIDI_1_EP[curEp].buffOffset;
                    USBMIDI_1_ARB_EP16_BASE.arbEp[curEp].rwWa16  = (uint32) USBMIDI_1_EP[curEp].buffOffset;
                #else
                    USBMIDI_1_ARB_EP_BASE.arbEp[curEp].rwRa    = LO8(USBMIDI_1_EP[curEp].buffOffset);
                    USBMIDI_1_ARB_EP_BASE.arbEp[curEp].rwRaMsb = HI8(USBMIDI_1_EP[curEp].buffOffset);
                    USBMIDI_1_ARB_EP_BASE.arbEp[curEp].rwWa    = LO8(USBMIDI_1_EP[curEp].buffOffset);
                    USBMIDI_1_ARB_EP_BASE.arbEp[curEp].rwWaMsb = HI8(USBMIDI_1_EP[curEp].buffOffset);
                #endif /* (CY_PSOC4) */                
            #endif /* (USBMIDI_1_EP_MANAGEMENT_DMA_AUTO) */
            }
            
            pEP = &pEP[1u]; /* Get next EP element */
        }
        
        /* The main loop has to re-enable DMA and OUT endpoint */
    }
}


/*******************************************************************************
* Function Name: USBMIDI_1_GetConfigTablePtr
****************************************************************************//**
*
*  This routine returns a pointer a configuration table entry
*
*  \param confIndex:  Configuration Index
*
* \return
*  Device Descriptor pointer or NULL when descriptor does not exist.
*
*******************************************************************************/
const T_USBMIDI_1_LUT CYCODE *USBMIDI_1_GetConfigTablePtr(uint8 confIndex)
                                                        
{
    /* Device Table */
    const T_USBMIDI_1_LUT CYCODE *pTmp;

    pTmp = (const T_USBMIDI_1_LUT CYCODE *) USBMIDI_1_TABLE[USBMIDI_1_device].p_list;

    /* The first entry points to the Device Descriptor,
    *  the second entry point to the BOS Descriptor
    *  the rest configuration entries.
    *  Set pointer to the first Configuration Descriptor
    */
    pTmp = &pTmp[2u];
    /* For this table, c is the number of configuration descriptors  */
    if(confIndex >= pTmp->c)   /* Verify that required configuration descriptor exists */
    {
        pTmp = (const T_USBMIDI_1_LUT CYCODE *) NULL;
    }
    else
    {
        pTmp = (const T_USBMIDI_1_LUT CYCODE *) pTmp[confIndex].p_list;
    }

    return (pTmp);
}


#if (USBMIDI_1_BOS_ENABLE)
    /*******************************************************************************
    * Function Name: USBMIDI_1_GetBOSPtr
    ****************************************************************************//**
    *
    *  This routine returns a pointer a BOS table entry
    *
    *  
    *
    * \return
    *  BOS Descriptor pointer or NULL when descriptor does not exist.
    *
    *******************************************************************************/
    const T_USBMIDI_1_LUT CYCODE *USBMIDI_1_GetBOSPtr(void)
                                                            
    {
        /* Device Table */
        const T_USBMIDI_1_LUT CYCODE *pTmp;

        pTmp = (const T_USBMIDI_1_LUT CYCODE *) USBMIDI_1_TABLE[USBMIDI_1_device].p_list;

        /* The first entry points to the Device Descriptor,
        *  the second entry points to the BOS Descriptor
        */
        pTmp = &pTmp[1u];
        pTmp = (const T_USBMIDI_1_LUT CYCODE *) pTmp->p_list;
        return (pTmp);
    }
#endif /* (USBMIDI_1_BOS_ENABLE) */


/*******************************************************************************
* Function Name: USBMIDI_1_GetDeviceTablePtr
****************************************************************************//**
*
*  This routine returns a pointer to the Device table
*
* \return
*  Device Table pointer
*
*******************************************************************************/
const T_USBMIDI_1_LUT CYCODE *USBMIDI_1_GetDeviceTablePtr(void)
                                                            
{
    /* Device Table */
    return( (const T_USBMIDI_1_LUT CYCODE *) USBMIDI_1_TABLE[USBMIDI_1_device].p_list );
}


/*******************************************************************************
* Function Name: USB_GetInterfaceClassTablePtr
****************************************************************************//**
*
*  This routine returns Interface Class table pointer, which contains
*  the relation between interface number and interface class.
*
* \return
*  Interface Class table pointer.
*
*******************************************************************************/
const uint8 CYCODE *USBMIDI_1_GetInterfaceClassTablePtr(void)
                                                        
{
    const T_USBMIDI_1_LUT CYCODE *pTmp;
    const uint8 CYCODE *pInterfaceClass;
    uint8 currentInterfacesNum;

    pTmp = USBMIDI_1_GetConfigTablePtr(USBMIDI_1_configuration - 1u);
    if (pTmp != NULL)
    {
        currentInterfacesNum  = ((const uint8 *) pTmp->p_list)[USBMIDI_1_CONFIG_DESCR_NUM_INTERFACES];
        /* Third entry in the LUT starts the Interface Table pointers */
        /* The INTERFACE_CLASS table is located after all interfaces */
        pTmp = &pTmp[currentInterfacesNum + 2u];
        pInterfaceClass = (const uint8 CYCODE *) pTmp->p_list;
    }
    else
    {
        pInterfaceClass = (const uint8 CYCODE *) NULL;
    }

    return (pInterfaceClass);
}


/*******************************************************************************
* Function Name: USBMIDI_1_TerminateEP
****************************************************************************//**
*
*  This function terminates the specified USBFS endpoint.
*  This function should be used before endpoint reconfiguration.
*
*  \param ep Contains the data endpoint number.
*
*  \reentrant
*  No.
*
* \sideeffect
* 
* The device responds with a NAK for any transactions on the selected endpoint.
*   
*******************************************************************************/
void USBMIDI_1_TerminateEP(uint8 epNumber) 
{
    /* Get endpoint number */
    epNumber &= USBMIDI_1_DIR_UNUSED;

    if ((epNumber > USBMIDI_1_EP0) && (epNumber < USBMIDI_1_MAX_EP))
    {
        /* Set the endpoint Halt */
        USBMIDI_1_EP[epNumber].hwEpState |= USBMIDI_1_ENDPOINT_STATUS_HALT;

        /* Clear the data toggle */
        USBMIDI_1_EP[epNumber].epToggle = 0u;
        USBMIDI_1_EP[epNumber].apiEpState = USBMIDI_1_NO_EVENT_ALLOWED;

        if ((USBMIDI_1_EP[epNumber].addr & USBMIDI_1_DIR_IN) != 0u)
        {   
            /* IN Endpoint */
            USBMIDI_1_SIE_EP_BASE.sieEp[epNumber].epCr0 = USBMIDI_1_MODE_NAK_IN;
        }
        else
        {
            /* OUT Endpoint */
            USBMIDI_1_SIE_EP_BASE.sieEp[epNumber].epCr0 = USBMIDI_1_MODE_NAK_OUT;
        }
    }
}


/*******************************************************************************
* Function Name: USBMIDI_1_SetEndpointHalt
****************************************************************************//**
*
*  This routine handles set endpoint halt.
*
* \return
*  requestHandled.
*
* \reentrant
*  No.
*
*******************************************************************************/
uint8 USBMIDI_1_SetEndpointHalt(void) 
{
    uint8 requestHandled = USBMIDI_1_FALSE;
    uint8 ep;
    
    /* Set endpoint halt */
    ep = USBMIDI_1_wIndexLoReg & USBMIDI_1_DIR_UNUSED;

    if ((ep > USBMIDI_1_EP0) && (ep < USBMIDI_1_MAX_EP))
    {
        /* Set the endpoint Halt */
        USBMIDI_1_EP[ep].hwEpState |= (USBMIDI_1_ENDPOINT_STATUS_HALT);

        /* Clear the data toggle */
        USBMIDI_1_EP[ep].epToggle = 0u;
        USBMIDI_1_EP[ep].apiEpState |= USBMIDI_1_NO_EVENT_ALLOWED;

        if ((USBMIDI_1_EP[ep].addr & USBMIDI_1_DIR_IN) != 0u)
        {
            /* IN Endpoint */
            USBMIDI_1_SIE_EP_BASE.sieEp[ep].epCr0 = (USBMIDI_1_MODE_STALL_DATA_EP | 
                                                            USBMIDI_1_MODE_ACK_IN);
        }
        else
        {
            /* OUT Endpoint */
            USBMIDI_1_SIE_EP_BASE.sieEp[ep].epCr0 = (USBMIDI_1_MODE_STALL_DATA_EP | 
                                                            USBMIDI_1_MODE_ACK_OUT);
        }
        requestHandled = USBMIDI_1_InitNoDataControlTransfer();
    }

    return (requestHandled);
}


/*******************************************************************************
* Function Name: USBMIDI_1_ClearEndpointHalt
****************************************************************************//**
*
*  This routine handles clear endpoint halt.
*
* \return
*  requestHandled.
*
* \reentrant
*  No.
*
*******************************************************************************/
uint8 USBMIDI_1_ClearEndpointHalt(void) 
{
    uint8 requestHandled = USBMIDI_1_FALSE;
    uint8 ep;

    /* Clear endpoint halt */
    ep = USBMIDI_1_wIndexLoReg & USBMIDI_1_DIR_UNUSED;

    if ((ep > USBMIDI_1_EP0) && (ep < USBMIDI_1_MAX_EP))
    {
        /* Clear the endpoint Halt */
        USBMIDI_1_EP[ep].hwEpState &= (uint8) ~USBMIDI_1_ENDPOINT_STATUS_HALT;

        /* Clear the data toggle */
        USBMIDI_1_EP[ep].epToggle = 0u;
        
        /* Clear toggle bit for already armed packet */
        USBMIDI_1_SIE_EP_BASE.sieEp[ep].epCnt0 = (uint8) ~(uint8)USBMIDI_1_EPX_CNT_DATA_TOGGLE;
        
        /* Return API State as it was defined before */
        USBMIDI_1_EP[ep].apiEpState &= (uint8) ~USBMIDI_1_NO_EVENT_ALLOWED;

        if ((USBMIDI_1_EP[ep].addr & USBMIDI_1_DIR_IN) != 0u)
        {
            /* IN Endpoint */
            if(USBMIDI_1_EP[ep].apiEpState == USBMIDI_1_IN_BUFFER_EMPTY)
            {       
                /* Wait for next packet from application */
                USBMIDI_1_SIE_EP_BASE.sieEp[ep].epCr0 = USBMIDI_1_MODE_NAK_IN;
            }
            else    /* Continue armed transfer */
            {
                USBMIDI_1_SIE_EP_BASE.sieEp[ep].epCr0 = USBMIDI_1_MODE_ACK_IN;
            }
        }
        else
        {
            /* OUT Endpoint */
            if (USBMIDI_1_EP[ep].apiEpState == USBMIDI_1_OUT_BUFFER_FULL)
            {       
                /* Allow application to read full buffer */
                USBMIDI_1_SIE_EP_BASE.sieEp[ep].epCr0 = USBMIDI_1_MODE_NAK_OUT;
            }
            else    /* Mark endpoint as empty, so it will be reloaded */
            {
                USBMIDI_1_SIE_EP_BASE.sieEp[ep].epCr0 = USBMIDI_1_MODE_ACK_OUT;
            }
        }
        
        requestHandled = USBMIDI_1_InitNoDataControlTransfer();
    }

    return(requestHandled);
}


/*******************************************************************************
* Function Name: USBMIDI_1_ValidateAlternateSetting
****************************************************************************//**
*
*  Validates (and records) a SET INTERFACE request.
*
* \return
*  requestHandled.
*
* \reentrant
*  No.
*
*******************************************************************************/
uint8 USBMIDI_1_ValidateAlternateSetting(void) 
{
    uint8 requestHandled = USBMIDI_1_FALSE;
    
    uint8 interfaceNum;
    uint8 curInterfacesNum;
    const T_USBMIDI_1_LUT CYCODE *pTmp;
    
    /* Get interface number from the request. */
    interfaceNum = (uint8) USBMIDI_1_wIndexLoReg;
    
    /* Get number of interfaces for current configuration. */
    pTmp = USBMIDI_1_GetConfigTablePtr(USBMIDI_1_configuration - 1u);
    curInterfacesNum  = ((const uint8 *) pTmp->p_list)[USBMIDI_1_CONFIG_DESCR_NUM_INTERFACES];

    /* Validate that interface number is within range. */
    if ((interfaceNum <= curInterfacesNum) || (interfaceNum <= USBMIDI_1_MAX_INTERFACES_NUMBER))
    {
        /* Save current and new alternate settings (come with request) to make 
        * desicion about following endpoint re-configuration.
        */
        USBMIDI_1_interfaceSettingLast[interfaceNum] = USBMIDI_1_interfaceSetting[interfaceNum];
        USBMIDI_1_interfaceSetting[interfaceNum]     = (uint8) USBMIDI_1_wValueLoReg;
        
        requestHandled = USBMIDI_1_TRUE;
    }

    return (requestHandled);
}


/* [] END OF FILE */
