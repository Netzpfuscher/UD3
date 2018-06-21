/***************************************************************************//**
* \file USBMIDI_1_drv.c
* \version 3.10
*
* \brief
*  This file contains the Endpoint 0 Driver for the USBFS Component.  
*
********************************************************************************
* \copyright
* Copyright 2008-2016, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "USBMIDI_1_pvt.h"
#include "cyapicallbacks.h"


/***************************************
* Global data allocation
***************************************/

volatile T_USBMIDI_1_EP_CTL_BLOCK USBMIDI_1_EP[USBMIDI_1_MAX_EP];

/** Contains the current configuration number, which is set by the host using a 
 * SET_CONFIGURATION request. This variable is initialized to zero in 
 * USBFS_InitComponent() API and can be read by the USBFS_GetConfiguration() 
 * API.*/
volatile uint8 USBMIDI_1_configuration;

/** Contains the current interface number.*/
volatile uint8 USBMIDI_1_interfaceNumber;

/** This variable is set to one after SET_CONFIGURATION and SET_INTERFACE 
 *requests. It can be read by the USBFS_IsConfigurationChanged() API */
volatile uint8 USBMIDI_1_configurationChanged;

/** Contains the current device address.*/
volatile uint8 USBMIDI_1_deviceAddress;

/** This is a two-bit variable that contains power status in the bit 0 
 * (DEVICE_STATUS_BUS_POWERED or DEVICE_STATUS_SELF_POWERED) and remote wakeup 
 * status (DEVICE_STATUS_REMOTE_WAKEUP) in the bit 1. This variable is 
 * initialized to zero in USBFS_InitComponent() API, configured by the 
 * USBFS_SetPowerStatus() API. The remote wakeup status cannot be set using the 
 * API SetPowerStatus(). */
volatile uint8 USBMIDI_1_deviceStatus;

volatile uint8 USBMIDI_1_interfaceSetting[USBMIDI_1_MAX_INTERFACES_NUMBER];
volatile uint8 USBMIDI_1_interfaceSetting_last[USBMIDI_1_MAX_INTERFACES_NUMBER];
volatile uint8 USBMIDI_1_interfaceStatus[USBMIDI_1_MAX_INTERFACES_NUMBER];

/** Contains the started device number. This variable is set by the 
 * USBFS_Start() or USBFS_InitComponent() APIs.*/
volatile uint8 USBMIDI_1_device;

/** Initialized class array for each interface. It is used for handling Class 
 * specific requests depend on interface class. Different classes in multiple 
 * alternate settings are not supported.*/
const uint8 CYCODE *USBMIDI_1_interfaceClass;


/***************************************
* Local data allocation
***************************************/

volatile uint8  USBMIDI_1_ep0Toggle;
volatile uint8  USBMIDI_1_lastPacketSize;

/** This variable is used by the communication functions to handle the current 
* transfer state.
* Initialized to TRANS_STATE_IDLE in the USBFS_InitComponent() API and after a 
* complete transfer in the status stage.
* Changed to the TRANS_STATE_CONTROL_READ or TRANS_STATE_CONTROL_WRITE in setup 
* transaction depending on the request type.
*/
volatile uint8  USBMIDI_1_transferState;
volatile T_USBMIDI_1_TD USBMIDI_1_currentTD;
volatile uint8  USBMIDI_1_ep0Mode;
volatile uint8  USBMIDI_1_ep0Count;
volatile uint16 USBMIDI_1_transferByteCount;


/*******************************************************************************
* Function Name: USBMIDI_1_ep_0_Interrupt
****************************************************************************//**
*
*  This Interrupt Service Routine handles Endpoint 0 (Control Pipe) traffic.
*  It dispatches setup requests and handles the data and status stages.
*
*
*******************************************************************************/
CY_ISR(USBMIDI_1_EP_0_ISR)
{
    uint8 tempReg;
    uint8 modifyReg;

#ifdef USBMIDI_1_EP_0_ISR_ENTRY_CALLBACK
    USBMIDI_1_EP_0_ISR_EntryCallback();
#endif /* (USBMIDI_1_EP_0_ISR_ENTRY_CALLBACK) */
    
    tempReg = USBMIDI_1_EP0_CR_REG;
    if ((tempReg & USBMIDI_1_MODE_ACKD) != 0u)
    {
        modifyReg = 1u;
        if ((tempReg & USBMIDI_1_MODE_SETUP_RCVD) != 0u)
        {
            if ((tempReg & USBMIDI_1_MODE_MASK) != USBMIDI_1_MODE_NAK_IN_OUT)
            {
                /* Mode not equal to NAK_IN_OUT: invalid setup */
                modifyReg = 0u;
            }
            else
            {
                USBMIDI_1_HandleSetup();
                
                if ((USBMIDI_1_ep0Mode & USBMIDI_1_MODE_SETUP_RCVD) != 0u)
                {
                    /* SETUP bit set: exit without mode modificaiton */
                    modifyReg = 0u;
                }
            }
        }
        else if ((tempReg & USBMIDI_1_MODE_IN_RCVD) != 0u)
        {
            USBMIDI_1_HandleIN();
        }
        else if ((tempReg & USBMIDI_1_MODE_OUT_RCVD) != 0u)
        {
            USBMIDI_1_HandleOUT();
        }
        else
        {
            modifyReg = 0u;
        }
        
        /* Modify the EP0_CR register */
        if (modifyReg != 0u)
        {
            
            tempReg = USBMIDI_1_EP0_CR_REG;
            
            /* Make sure that SETUP bit is cleared before modification */
            if ((tempReg & USBMIDI_1_MODE_SETUP_RCVD) == 0u)
            {
                /* Update count register */
                tempReg = (uint8) USBMIDI_1_ep0Toggle | USBMIDI_1_ep0Count;
                USBMIDI_1_EP0_CNT_REG = tempReg;
               
                /* Make sure that previous write operaiton was successful */
                if (tempReg == USBMIDI_1_EP0_CNT_REG)
                {
                    /* Repeat until next successful write operation */
                    do
                    {
                        /* Init temporary variable */
                        modifyReg = USBMIDI_1_ep0Mode;
                        
                        /* Unlock register */
                        tempReg = (uint8) (USBMIDI_1_EP0_CR_REG & USBMIDI_1_MODE_SETUP_RCVD);
                        
                        /* Check if SETUP bit is not set */
                        if (0u == tempReg)
                        {
                            /* Set the Mode Register  */
                            USBMIDI_1_EP0_CR_REG = USBMIDI_1_ep0Mode;
                            
                            /* Writing check */
                            modifyReg = USBMIDI_1_EP0_CR_REG & USBMIDI_1_MODE_MASK;
                        }
                    }
                    while (modifyReg != USBMIDI_1_ep0Mode);
                }
            }
        }
    }

    USBMIDI_1_ClearSieInterruptSource(USBMIDI_1_INTR_SIE_EP0_INTR);
	
#ifdef USBMIDI_1_EP_0_ISR_EXIT_CALLBACK
    USBMIDI_1_EP_0_ISR_ExitCallback();
#endif /* (USBMIDI_1_EP_0_ISR_EXIT_CALLBACK) */
}


/*******************************************************************************
* Function Name: USBMIDI_1_HandleSetup
****************************************************************************//**
*
*  This Routine dispatches requests for the four USB request types
*
*
* \reentrant
*  No.
*
*******************************************************************************/
void USBMIDI_1_HandleSetup(void) 
{
    uint8 requestHandled;
    
    /* Clear register lock by SIE (read register) and clear setup bit 
    * (write any value in register).
    */
    requestHandled = (uint8) USBMIDI_1_EP0_CR_REG;
    USBMIDI_1_EP0_CR_REG = (uint8) requestHandled;
    requestHandled = (uint8) USBMIDI_1_EP0_CR_REG;

    if ((requestHandled & USBMIDI_1_MODE_SETUP_RCVD) != 0u)
    {
        /* SETUP bit is set: exit without mode modification. */
        USBMIDI_1_ep0Mode = requestHandled;
    }
    else
    {
        /* In case the previous transfer did not complete, close it out */
        USBMIDI_1_UpdateStatusBlock(USBMIDI_1_XFER_PREMATURE);

        /* Check request type. */
        switch (USBMIDI_1_bmRequestTypeReg & USBMIDI_1_RQST_TYPE_MASK)
        {
            case USBMIDI_1_RQST_TYPE_STD:
                requestHandled = USBMIDI_1_HandleStandardRqst();
                break;
                
            case USBMIDI_1_RQST_TYPE_CLS:
                requestHandled = USBMIDI_1_DispatchClassRqst();
                break;
                
            case USBMIDI_1_RQST_TYPE_VND:
                requestHandled = USBMIDI_1_HandleVendorRqst();
                break;
                
            default:
                requestHandled = USBMIDI_1_FALSE;
                break;
        }
        
        /* If request is not recognized. Stall endpoint 0 IN and OUT. */
        if (requestHandled == USBMIDI_1_FALSE)
        {
            USBMIDI_1_ep0Mode = USBMIDI_1_MODE_STALL_IN_OUT;
        }
    }
}


/*******************************************************************************
* Function Name: USBMIDI_1_HandleIN
****************************************************************************//**
*
*  This routine handles EP0 IN transfers.
*
*
* \reentrant
*  No.
*
*******************************************************************************/
void USBMIDI_1_HandleIN(void) 
{
    switch (USBMIDI_1_transferState)
    {
        case USBMIDI_1_TRANS_STATE_IDLE:
            break;
        
        case USBMIDI_1_TRANS_STATE_CONTROL_READ:
            USBMIDI_1_ControlReadDataStage();
            break;
            
        case USBMIDI_1_TRANS_STATE_CONTROL_WRITE:
            USBMIDI_1_ControlWriteStatusStage();
            break;
            
        case USBMIDI_1_TRANS_STATE_NO_DATA_CONTROL:
            USBMIDI_1_NoDataControlStatusStage();
            break;
            
        default:    /* there are no more states */
            break;
    }
}


/*******************************************************************************
* Function Name: USBMIDI_1_HandleOUT
****************************************************************************//**
*
*  This routine handles EP0 OUT transfers.
*
*
* \reentrant
*  No.
*
*******************************************************************************/
void USBMIDI_1_HandleOUT(void) 
{
    switch (USBMIDI_1_transferState)
    {
        case USBMIDI_1_TRANS_STATE_IDLE:
            break;
        
        case USBMIDI_1_TRANS_STATE_CONTROL_READ:
            USBMIDI_1_ControlReadStatusStage();
            break;
            
        case USBMIDI_1_TRANS_STATE_CONTROL_WRITE:
            USBMIDI_1_ControlWriteDataStage();
            break;
            
        case USBMIDI_1_TRANS_STATE_NO_DATA_CONTROL:
            /* Update the completion block */
            USBMIDI_1_UpdateStatusBlock(USBMIDI_1_XFER_ERROR);
            
            /* We expect no more data, so stall INs and OUTs */
            USBMIDI_1_ep0Mode = USBMIDI_1_MODE_STALL_IN_OUT;
            break;
            
        default:    
            /* There are no more states */
            break;
    }
}


/*******************************************************************************
* Function Name: USBMIDI_1_LoadEP0
****************************************************************************//**
*
*  This routine loads the EP0 data registers for OUT transfers. It uses the
*  currentTD (previously initialized by the _InitControlWrite function and
*  updated for each OUT transfer, and the bLastPacketSize) to determine how
*  many uint8s to transfer on the current OUT.
*
*  If the number of uint8s remaining is zero and the last transfer was full,
*  we need to send a zero length packet.  Otherwise we send the minimum
*  of the control endpoint size (8) or remaining number of uint8s for the
*  transaction.
*
*
* \globalvars
*  USBMIDI_1_transferByteCount - Update the transfer byte count from the
*     last transaction.
*  USBMIDI_1_ep0Count - counts the data loaded to the SIE memory in
*     current packet.
*  USBMIDI_1_lastPacketSize - remembers the USBFS_ep0Count value for the
*     next packet.
*  USBMIDI_1_transferByteCount - sum of the previous bytes transferred
*     on previous packets(sum of USBFS_lastPacketSize)
*  USBMIDI_1_ep0Toggle - inverted
*  USBMIDI_1_ep0Mode  - prepare for mode register content.
*  USBMIDI_1_transferState - set to TRANS_STATE_CONTROL_READ
*
* \reentrant
*  No.
*
*******************************************************************************/
void USBMIDI_1_LoadEP0(void) 
{
    uint8 ep0Count = 0u;

    /* Update the transfer byte count from the last transaction */
    USBMIDI_1_transferByteCount += USBMIDI_1_lastPacketSize;

    /* Now load the next transaction */
    while ((USBMIDI_1_currentTD.count > 0u) && (ep0Count < 8u))
    {
        USBMIDI_1_EP0_DR_BASE.epData[ep0Count] = (uint8) *USBMIDI_1_currentTD.pData;
        USBMIDI_1_currentTD.pData = &USBMIDI_1_currentTD.pData[1u];
        ep0Count++;
        USBMIDI_1_currentTD.count--;
    }

    /* Support zero-length packet */
    if ((USBMIDI_1_lastPacketSize == 8u) || (ep0Count > 0u))
    {
        /* Update the data toggle */
        USBMIDI_1_ep0Toggle ^= USBMIDI_1_EP0_CNT_DATA_TOGGLE;
        /* Set the Mode Register  */
        USBMIDI_1_ep0Mode = USBMIDI_1_MODE_ACK_IN_STATUS_OUT;
        /* Update the state (or stay the same) */
        USBMIDI_1_transferState = USBMIDI_1_TRANS_STATE_CONTROL_READ;
    }
    else
    {
        /* Expect Status Stage Out */
        USBMIDI_1_ep0Mode = USBMIDI_1_MODE_STATUS_OUT_ONLY;
        /* Update the state (or stay the same) */
        USBMIDI_1_transferState = USBMIDI_1_TRANS_STATE_CONTROL_READ;
    }

    /* Save the packet size for next time */
    USBMIDI_1_ep0Count =       (uint8) ep0Count;
    USBMIDI_1_lastPacketSize = (uint8) ep0Count;
}


/*******************************************************************************
* Function Name: USBMIDI_1_InitControlRead
****************************************************************************//**
*
*  Initialize a control read transaction. It is used to send data to the host.
*  The following global variables should be initialized before this function
*  called. To send zero length packet use InitZeroLengthControlTransfer
*  function.
*
*
* \return
*  requestHandled state.
*
* \globalvars
*  USBMIDI_1_currentTD.count - counts of data to be sent.
*  USBMIDI_1_currentTD.pData - data pointer.
*
* \reentrant
*  No.
*
*******************************************************************************/
uint8 USBMIDI_1_InitControlRead(void) 
{
    uint16 xferCount;

    if (USBMIDI_1_currentTD.count == 0u)
    {
        (void) USBMIDI_1_InitZeroLengthControlTransfer();
    }
    else
    {
        /* Set up the state machine */
        USBMIDI_1_transferState = USBMIDI_1_TRANS_STATE_CONTROL_READ;
        
        /* Set the toggle, it gets updated in LoadEP */
        USBMIDI_1_ep0Toggle = 0u;
        
        /* Initialize the Status Block */
        USBMIDI_1_InitializeStatusBlock();
        
        xferCount = ((uint16)((uint16) USBMIDI_1_lengthHiReg << 8u) | ((uint16) USBMIDI_1_lengthLoReg));

        if (USBMIDI_1_currentTD.count > xferCount)
        {
            USBMIDI_1_currentTD.count = xferCount;
        }
        
        USBMIDI_1_LoadEP0();
    }

    return (USBMIDI_1_TRUE);
}


/*******************************************************************************
* Function Name: USBMIDI_1_InitZeroLengthControlTransfer
****************************************************************************//**
*
*  Initialize a zero length data IN transfer.
*
* \return
*  requestHandled state.
*
* \globalvars
*  USBMIDI_1_ep0Toggle - set to EP0_CNT_DATA_TOGGLE
*  USBMIDI_1_ep0Mode  - prepare for mode register content.
*  USBMIDI_1_transferState - set to TRANS_STATE_CONTROL_READ
*  USBMIDI_1_ep0Count - cleared, means the zero-length packet.
*  USBMIDI_1_lastPacketSize - cleared.
*
* \reentrant
*  No.
*
*******************************************************************************/
uint8 USBMIDI_1_InitZeroLengthControlTransfer(void)
                                                
{
    /* Update the state */
    USBMIDI_1_transferState = USBMIDI_1_TRANS_STATE_CONTROL_READ;
    
    /* Set the data toggle */
    USBMIDI_1_ep0Toggle = USBMIDI_1_EP0_CNT_DATA_TOGGLE;
    
    /* Set the Mode Register  */
    USBMIDI_1_ep0Mode = USBMIDI_1_MODE_ACK_IN_STATUS_OUT;
    
    /* Save the packet size for next time */
    USBMIDI_1_lastPacketSize = 0u;
    
    USBMIDI_1_ep0Count = 0u;

    return (USBMIDI_1_TRUE);
}


/*******************************************************************************
* Function Name: USBMIDI_1_ControlReadDataStage
****************************************************************************//**
*
*  Handle the Data Stage of a control read transfer.
*
*
* \reentrant
*  No.
*
*******************************************************************************/
void USBMIDI_1_ControlReadDataStage(void) 

{
    USBMIDI_1_LoadEP0();
}


/*******************************************************************************
* Function Name: USBMIDI_1_ControlReadStatusStage
****************************************************************************//**
*
*  Handle the Status Stage of a control read transfer.
*
*
* \globalvars
*  USBMIDI_1_USBFS_transferByteCount - updated with last packet size.
*  USBMIDI_1_transferState - set to TRANS_STATE_IDLE.
*  USBMIDI_1_ep0Mode  - set to MODE_STALL_IN_OUT.
*
* \reentrant
*  No.
*
*******************************************************************************/
void USBMIDI_1_ControlReadStatusStage(void) 
{
    /* Update the transfer byte count */
    USBMIDI_1_transferByteCount += USBMIDI_1_lastPacketSize;
    
    /* Go Idle */
    USBMIDI_1_transferState = USBMIDI_1_TRANS_STATE_IDLE;
    
    /* Update the completion block */
    USBMIDI_1_UpdateStatusBlock(USBMIDI_1_XFER_STATUS_ACK);
    
    /* We expect no more data, so stall INs and OUTs */
    USBMIDI_1_ep0Mode = USBMIDI_1_MODE_STALL_IN_OUT;
}


/*******************************************************************************
* Function Name: USBMIDI_1_InitControlWrite
****************************************************************************//**
*
*  Initialize a control write transaction
*
* \return
*  requestHandled state.
*
* \globalvars
*  USBMIDI_1_USBFS_transferState - set to TRANS_STATE_CONTROL_WRITE
*  USBMIDI_1_ep0Toggle - set to EP0_CNT_DATA_TOGGLE
*  USBMIDI_1_ep0Mode  - set to MODE_ACK_OUT_STATUS_IN
*
* \reentrant
*  No.
*
*******************************************************************************/
uint8 USBMIDI_1_InitControlWrite(void) 
{
    uint16 xferCount;

    /* Set up the state machine */
    USBMIDI_1_transferState = USBMIDI_1_TRANS_STATE_CONTROL_WRITE;
    
    /* This might not be necessary */
    USBMIDI_1_ep0Toggle = USBMIDI_1_EP0_CNT_DATA_TOGGLE;
    
    /* Initialize the Status Block */
    USBMIDI_1_InitializeStatusBlock();

    xferCount = ((uint16)((uint16) USBMIDI_1_lengthHiReg << 8u) | ((uint16) USBMIDI_1_lengthLoReg));

    if (USBMIDI_1_currentTD.count > xferCount)
    {
        USBMIDI_1_currentTD.count = xferCount;
    }

    /* Expect Data or Status Stage */
    USBMIDI_1_ep0Mode = USBMIDI_1_MODE_ACK_OUT_STATUS_IN;

    return(USBMIDI_1_TRUE);
}


/*******************************************************************************
* Function Name: USBMIDI_1_ControlWriteDataStage
****************************************************************************//**
*
*  Handle the Data Stage of a control write transfer
*       1. Get the data (We assume the destination was validated previously)
*       2. Update the count and data toggle
*       3. Update the mode register for the next transaction
*
*
* \globalvars
*  USBMIDI_1_transferByteCount - Update the transfer byte count from the
*    last transaction.
*  USBMIDI_1_ep0Count - counts the data loaded from the SIE memory
*    in current packet.
*  USBMIDI_1_transferByteCount - sum of the previous bytes transferred
*    on previous packets(sum of USBFS_lastPacketSize)
*  USBMIDI_1_ep0Toggle - inverted
*  USBMIDI_1_ep0Mode  - set to MODE_ACK_OUT_STATUS_IN.
*
* \reentrant
*  No.
*
*******************************************************************************/
void USBMIDI_1_ControlWriteDataStage(void) 
{
    uint8 ep0Count;
    uint8 regIndex = 0u;

    ep0Count = (USBMIDI_1_EP0_CNT_REG & USBMIDI_1_EPX_CNT0_MASK) - USBMIDI_1_EPX_CNTX_CRC_COUNT;

    USBMIDI_1_transferByteCount += (uint8)ep0Count;

    while ((USBMIDI_1_currentTD.count > 0u) && (ep0Count > 0u))
    {
        *USBMIDI_1_currentTD.pData = (uint8) USBMIDI_1_EP0_DR_BASE.epData[regIndex];
        USBMIDI_1_currentTD.pData = &USBMIDI_1_currentTD.pData[1u];
        regIndex++;
        ep0Count--;
        USBMIDI_1_currentTD.count--;
    }
    
    USBMIDI_1_ep0Count = (uint8)ep0Count;
    
    /* Update the data toggle */
    USBMIDI_1_ep0Toggle ^= USBMIDI_1_EP0_CNT_DATA_TOGGLE;
    
    /* Expect Data or Status Stage */
    USBMIDI_1_ep0Mode = USBMIDI_1_MODE_ACK_OUT_STATUS_IN;
}


/*******************************************************************************
* Function Name: USBMIDI_1_ControlWriteStatusStage
****************************************************************************//**
*
*  Handle the Status Stage of a control write transfer
*
* \globalvars
*  USBMIDI_1_transferState - set to TRANS_STATE_IDLE.
*  USBMIDI_1_USBFS_ep0Mode  - set to MODE_STALL_IN_OUT.
*
* \reentrant
*  No.
*
*******************************************************************************/
void USBMIDI_1_ControlWriteStatusStage(void) 
{
    /* Go Idle */
    USBMIDI_1_transferState = USBMIDI_1_TRANS_STATE_IDLE;
    
    /* Update the completion block */    
    USBMIDI_1_UpdateStatusBlock(USBMIDI_1_XFER_STATUS_ACK);
    
    /* We expect no more data, so stall INs and OUTs */
    USBMIDI_1_ep0Mode = USBMIDI_1_MODE_STALL_IN_OUT;
}


/*******************************************************************************
* Function Name: USBMIDI_1_InitNoDataControlTransfer
****************************************************************************//**
*
*  Initialize a no data control transfer
*
* \return
*  requestHandled state.
*
* \globalvars
*  USBMIDI_1_transferState - set to TRANS_STATE_NO_DATA_CONTROL.
*  USBMIDI_1_ep0Mode  - set to MODE_STATUS_IN_ONLY.
*  USBMIDI_1_ep0Count - cleared.
*  USBMIDI_1_ep0Toggle - set to EP0_CNT_DATA_TOGGLE
*
* \reentrant
*  No.
*
*******************************************************************************/
uint8 USBMIDI_1_InitNoDataControlTransfer(void) 
{
    USBMIDI_1_transferState = USBMIDI_1_TRANS_STATE_NO_DATA_CONTROL;
    USBMIDI_1_ep0Mode       = USBMIDI_1_MODE_STATUS_IN_ONLY;
    USBMIDI_1_ep0Toggle     = USBMIDI_1_EP0_CNT_DATA_TOGGLE;
    USBMIDI_1_ep0Count      = 0u;

    return (USBMIDI_1_TRUE);
}


/*******************************************************************************
* Function Name: USBMIDI_1_NoDataControlStatusStage
****************************************************************************//**
*  Handle the Status Stage of a no data control transfer.
*
*  SET_ADDRESS is special, since we need to receive the status stage with
*  the old address.
*
* \globalvars
*  USBMIDI_1_transferState - set to TRANS_STATE_IDLE.
*  USBMIDI_1_ep0Mode  - set to MODE_STALL_IN_OUT.
*  USBMIDI_1_ep0Toggle - set to EP0_CNT_DATA_TOGGLE
*  USBMIDI_1_deviceAddress - used to set new address and cleared
*
* \reentrant
*  No.
*
*******************************************************************************/
void USBMIDI_1_NoDataControlStatusStage(void) 
{
    if (0u != USBMIDI_1_deviceAddress)
    {
        /* Update device address if we got new address. */
        USBMIDI_1_CR0_REG = (uint8) USBMIDI_1_deviceAddress | USBMIDI_1_CR0_ENABLE;
        USBMIDI_1_deviceAddress = 0u;
    }

    USBMIDI_1_transferState = USBMIDI_1_TRANS_STATE_IDLE;
    
    /* Update the completion block. */
    USBMIDI_1_UpdateStatusBlock(USBMIDI_1_XFER_STATUS_ACK);
    
    /* Stall IN and OUT, no more data is expected. */
    USBMIDI_1_ep0Mode = USBMIDI_1_MODE_STALL_IN_OUT;
}


/*******************************************************************************
* Function Name: USBMIDI_1_UpdateStatusBlock
****************************************************************************//**
*
*  Update the Completion Status Block for a Request.  The block is updated
*  with the completion code the USBMIDI_1_transferByteCount.  The
*  StatusBlock Pointer is set to NULL.
*
*  completionCode - status.
*
*
* \globalvars
*  USBMIDI_1_currentTD.pStatusBlock->status - updated by the
*    completionCode parameter.
*  USBMIDI_1_currentTD.pStatusBlock->length - updated.
*  USBMIDI_1_currentTD.pStatusBlock - cleared.
*
* \reentrant
*  No.
*
*******************************************************************************/
void USBMIDI_1_UpdateStatusBlock(uint8 completionCode) 
{
    if (USBMIDI_1_currentTD.pStatusBlock != NULL)
    {
        USBMIDI_1_currentTD.pStatusBlock->status = completionCode;
        USBMIDI_1_currentTD.pStatusBlock->length = USBMIDI_1_transferByteCount;
        USBMIDI_1_currentTD.pStatusBlock = NULL;
    }
}


/*******************************************************************************
* Function Name: USBMIDI_1_InitializeStatusBlock
****************************************************************************//**
*
*  Initialize the Completion Status Block for a Request.  The completion
*  code is set to USB_XFER_IDLE.
*
*  Also, initializes USBMIDI_1_transferByteCount.  Save some space,
*  this is the only consumer.
*
* \globalvars
*  USBMIDI_1_currentTD.pStatusBlock->status - set to XFER_IDLE.
*  USBMIDI_1_currentTD.pStatusBlock->length - cleared.
*  USBMIDI_1_transferByteCount - cleared.
*
* \reentrant
*  No.
*
*******************************************************************************/
void USBMIDI_1_InitializeStatusBlock(void) 
{
    USBMIDI_1_transferByteCount = 0u;
    
    if (USBMIDI_1_currentTD.pStatusBlock != NULL)
    {
        USBMIDI_1_currentTD.pStatusBlock->status = USBMIDI_1_XFER_IDLE;
        USBMIDI_1_currentTD.pStatusBlock->length = 0u;
    }
}


/* [] END OF FILE */
