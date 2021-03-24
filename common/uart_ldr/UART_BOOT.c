/*******************************************************************************
* File Name: UART_BOOT.c
* Version 2.50
*
* Description:
*  This file provides the source code of bootloader communication APIs for the
*  UART component.
*
* Note:
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "UART.h"
#include <project.h>
#include "cli_basic.h"
#include "Bootloader.h"

#if defined(CYDEV_BOOTLOADER_IO_COMP) && (0u != ((CYDEV_BOOTLOADER_IO_COMP == CyBtldr_UART) || \
                                          (CYDEV_BOOTLOADER_IO_COMP == CyBtldr_Custom_Interface)))
 
static uint8  USBUART_started = 0u;

/*******************************************************************************
* Function Name: UART_CyBtldrCommStart
********************************************************************************
*
* Summary:
*  Starts the UART communication component.
*
* Parameters:
*  None
*
* Return:
*  None
*
* Side Effects:
*  This component automatically enables global interrupt.
*
*******************************************************************************/

    
void UART_CyBtldrCommStart(void) CYSMALL 
{
    
    CyGlobalIntEnable;      /* Enable Global Interrupts */

    /*Start USB Operation/device 0 and with 5V or 3V operation depend on Voltage Configuration in DWR */
    USBUART_Start(0u, USBUART_DWR_VDDD_OPERATION);

    /* USB component started, the correct enumeration will be checked in first Read operation */
    USBUART_started = 1u;
    uint16_t timeoutMs = 2000;
    

        /* Wait for Device to enumerate */
        while ((0u ==USBUART_GetConfiguration()) && (0u != timeoutMs))
        {
            CyDelay(1);
            timeoutMs--;
        }
   //if(timeoutMs>0) com_type = USB;
    
    /* Start UART component and clear the Tx,Rx buffers */
    UART_Start();
    UART_ClearRxBuffer();
    UART_ClearTxBuffer();
}

/*******************************************************************************
* Function Name: UART_CyBtldrCommStop
********************************************************************************
*
* Summary:
*  Disables the communication component and disables the interrupt.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void UART_CyBtldrCommStop(void) CYSMALL 
{
    /* Stop UART component */
    UART_Stop();
    USBUART_Stop();
}


/*******************************************************************************
* Function Name: UART_CyBtldrCommReset
********************************************************************************
*
* Summary:
*  Resets the receive and transmit communication Buffers.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void UART_CyBtldrCommReset(void) CYSMALL 
{
    /* Clear RX and TX buffers */
    UART_ClearRxBuffer();
    UART_ClearTxBuffer();
    USBUART_CDC_Init();
}


/*******************************************************************************
* Function Name: UART_CyBtldrCommWrite
********************************************************************************
*
* Summary:
*  Allows the caller to write data to the boot loader host. This function uses
* a blocking write function for writing data using UART communication component.
*
* Parameters:
*  pData:    A pointer to the block of data to send to the device
*  size:     The number of bytes to write.
*  count:    Pointer to an unsigned short variable to write the number of
*             bytes actually written.
*  timeOut:  Number of units to wait before returning because of a timeout.
*
* Return:
*   cystatus: This function will return CYRET_SUCCESS if data is sent
*             successfully.
*
* Side Effects:
*  This function should be called after command was received .
*
*******************************************************************************/

cystatus UART_CyBtldrCommWrite(const uint8 pData[], uint16 size, uint16 * count, uint8 timeOut) CYSMALL
         
{
    uint16 bufIndex = 0u;

    if(0u != timeOut)
    {
        /* Suppress compiler warning */
    }

    /* Clear receive buffers */
    UART_ClearRxBuffer();

    /* Write TX data using blocking function */
    while(bufIndex < size)
    {
        UART_PutChar(pData[bufIndex]);
        bufIndex++;
    }

    /* Return success code */
    *count = size;

    return (CYRET_SUCCESS);
}


/*******************************************************************************
* Function Name: UART_CyBtldrCommRead
********************************************************************************
*
* Summary:
*  Receives the command.
*
* Parameters:
*  pData:    A pointer to the area to store the block of data received
*             from the device.
*  size:     Maximum size of the read buffer
*  count:    Pointer to an unsigned short variable to write the number
*             of bytes actually read.
*  timeOut:  Number of units to wait before returning because of a timeOut.
*            Time out is measured in 10s of ms.
*
* Return:
*  cystatus: This function will return CYRET_SUCCESS if at least one byte is
*            received successfully within the time out interval. If no data is
*            received  this function will return CYRET_EMPTY.
*
*  BYTE2BYTE_TIME_OUT is used for detecting time out marking end of block data
*  from host. This has to be set to a value which is greater than the expected
*  maximum delay between two bytes during a block/packet transmission from the
*  host. You have to account for the delay in hardware converters while
*  calculating this value, if you are using any USB-UART bridges.
*******************************************************************************/
cystatus UART_CyBtldrCommRead(uint8 pData[], uint16 size, uint16 * count, uint8 timeOut) CYSMALL
         
{
    uint16 iCntr;
    uint16 dataIndexCntr;
    uint16 tempCount;
    uint16 oldDataCount;

    cystatus status = CYRET_EMPTY;

    /* Check whether data is received within the time out period.
    *  Time out period is in units of 10ms.
    *  If at least one byte is received within the time out interval, wait for more data */
    for (iCntr = 0u; iCntr < ((uint16)10u * timeOut); iCntr++)
    {
        /* If at least one byte is received within the timeout interval
        *  enter the next loop waiting for more data reception
        */
        if(0u != UART_GetRxBufferSize())
        {
            /* Wait for more data until 25ms byte to byte time out interval.
            * If no data is received during the last 25 ms(BYTE2BYTE_TIME_OUT)
            * then it is considered as end of transmitted data block(packet)
            * from the host and the program execution will break from the
            * data awaiting loop with status=CYRET_SUCCESS
            */
            do
            {
                oldDataCount = UART_GetRxBufferSize();
                CyDelayUs(UART_BYTE2BYTE_TIME_OUT);
                //CyDelay(UART_BYTE2BYTE_TIME_OUT);
            }
            while(UART_GetRxBufferSize() > oldDataCount);

            status = CYRET_SUCCESS;
            break;
        }
        /* If the data is not received, give a delay of 
        *  UART_BL_CHK_DELAY_MS and check again until the timeOut specified.
        */
        else
        {
            CyDelay(UART_BL_CHK_DELAY_MS);
        }
    }

    /* Initialize the data read indexes and Count value */
    *count = 0u;
    dataIndexCntr = 0u;

    /* If GetRxBufferSize()>0 , move the received data to the pData buffer */
    while(UART_GetRxBufferSize() > 0u)
    {
        tempCount = UART_GetRxBufferSize();
        *count  =(*count) + tempCount;

        /* Check if buffer overflow will occur before moving the data */
        if(*count < size)
        {
            for (iCntr = 0u; iCntr < tempCount; iCntr++)
            {
                /* Read the data and move it to the pData buffer */
                pData[dataIndexCntr] = UART_ReadRxData();
                dataIndexCntr++;
            }

            /* Check if the last received byte is end of packet defined by bootloader
            *  If not wait for additional UART_WAIT_EOP_DELAY ms.
            */
            if(pData[dataIndexCntr - 1u] != UART_PACKET_EOP)
            {
                CyDelay(UART_WAIT_EOP_DELAY);
            }
        }
        /* If there is no space to move data, break from the loop */
        else
        {
            *count = (*count) - tempCount;
            break;
        }
    }

    return (status);
}

/***************************************
*    Bootloader Variables
***************************************/
#define USBUART_BTLDR_OUT_EP_1 0x03
#define USBUART_BTLDR_IN_EP_1  0x02





/*******************************************************************************
* Function Name: CyBtldrCommWrite.
********************************************************************************
*
* Summary:
*  Allows the caller to write data to the boot loader host. The function will
*  handle polling to allow a block of data to be completely sent to the host
*  device.
*
* Parameters:
*  pData:    A pointer to the block of data to send to the device
*  size:     The number of bytes to write.
*  count:    Pointer to an unsigned short variable to write the number of
*             bytes actually written.
*  timeOut:  Number of units to wait before returning because of a timeout.
*
* Return:
*  Returns the value that best describes the problem.
*
* Reentrant:
*  No
*
*******************************************************************************/

#define USBUART_BTLDR_SIZEOF_WRITE_BUFFER  (64u)   /* Endpoint 1 (OUT) buffer size. */
#define USBUART_BTLDR_SIZEOF_READ_BUFFER   (64u)   /* Endpoint 2 (IN)  buffer size. */

cystatus USB_CyBtldrCommWrite(const uint8 pData[], uint16 size, uint16 * count, uint8 timeOut) CYSMALL                                                          
{
    cystatus retCode;
    uint16 timeoutMs;
	uint16 bufIndex = 0;
	uint8 transfer_Size;
	*count = 0;
    timeoutMs = ((uint16) 10u * timeOut);  /* Convert from 10mS check to number 1mS checks */
    while(bufIndex < size)
	{
		if ((size -bufIndex) >  USBUART_BTLDR_SIZEOF_READ_BUFFER)
		{
			transfer_Size = USBUART_BTLDR_SIZEOF_READ_BUFFER;
		}
		else 
		{
			transfer_Size = (size -bufIndex);
		}
		/* Enable IN transfer */
    	USBUART_LoadInEP(USBUART_BTLDR_IN_EP_1, &pData[bufIndex], transfer_Size);
		/* Wait for the master to read it. */
   		while ((USBUART_GetEPState(USBUART_BTLDR_IN_EP_1) == USBUART_IN_BUFFER_FULL) &&
           (0u != timeoutMs))
    	{
        	CyDelay(1);
        	timeoutMs--;
    	}
		if (USBUART_GetEPState(USBUART_BTLDR_IN_EP_1) == USBUART_IN_BUFFER_FULL)
    	{
        	retCode = CYRET_TIMEOUT;
			break; 
    	}
	    else
	    {
	        *count += transfer_Size;
	        retCode = CYRET_SUCCESS;
			bufIndex  += transfer_Size;
	    }
	}
		
    return(retCode);
}

/*******************************************************************************
* Function Name: CyBtldrCommRead.
********************************************************************************
*
* Summary:
*  Allows the caller to read data from the boot loader host. The function will
*  handle polling to allow a block of data to be completely received from the
*  host device.
*
* Parameters:
*  pData:    A pointer to the area to store the block of data received
*             from the device.
*  size:     The number of bytes to read.
*  count:    Pointer to an unsigned short variable to write the number
*             of bytes actually read.
*  timeOut:  Number of units to wait before returning because of a timeOut.
*            Timeout is measured in 10s of ms.
*
* Return:
*  Returns the value that best describes the problem.
*
* Reentrant:
*  No
*
*******************************************************************************/
cystatus USB_CyBtldrCommRead(uint8 pData[], uint16 size, uint16 * count, uint8 timeOut) CYSMALL                                                            
{
    cystatus retCode;
    uint16 timeoutMs;

    timeoutMs = ((uint16) 10u * timeOut);  /* Convert from 10mS check to number 1mS checks */
    
    if (size > USBUART_BTLDR_SIZEOF_WRITE_BUFFER)
    {
        size = USBUART_BTLDR_SIZEOF_WRITE_BUFFER;
    }

    /* Wait on enumeration in first time */
    if (0u != USBUART_started)
    {
        /* Wait for Device to enumerate */
        while ((0u ==USBUART_GetConfiguration()) && (0u != timeoutMs))
        {
            CyDelay(1);
            timeoutMs--;
        }
        
        /* Enable first OUT, if enumeration complete */
        if (0u != USBUART_GetConfiguration())
        {
            (void) USBUART_IsConfigurationChanged();  /* Clear configuration changes state status */
            CyBtldrCommReset();
            USBUART_started = 0u;
        }
    }
    else /* Check for configuration changes, has been done by Host */
    {
        if (0u != USBUART_IsConfigurationChanged()) /* Host could send double SET_INTERFACE request or RESET */
        {
            if (0u != USBUART_GetConfiguration())   /* Init OUT endpoints when device reconfigured */
            {
                CyBtldrCommReset();
            }
        }
    }
    
    timeoutMs = ((uint16) 10u * timeOut); /* Re-arm timeout */
    
    /* Wait on next packet */
    while((USBUART_GetEPState(USBUART_BTLDR_OUT_EP_1) != USBUART_OUT_BUFFER_FULL) && \
          (0u != timeoutMs))
    {
        CyDelay(1);
        timeoutMs--;
    }

    /* OUT EP has completed */
    if (USBUART_GetEPState(USBUART_BTLDR_OUT_EP_1) == USBUART_OUT_BUFFER_FULL)
    {
        *count = USBUART_ReadOutEP(USBUART_BTLDR_OUT_EP_1, pData, size);
        retCode = CYRET_SUCCESS;
    }
    else
    {
        *count = 0u;
        retCode = CYRET_TIMEOUT;
    }
    
    return(retCode);
}



#endif /* end CYDEV_BOOTLOADER_IO_COMP */


/* [] END OF FILE */
