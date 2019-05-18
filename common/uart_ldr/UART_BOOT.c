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
#include "ETH.h"

#if defined(CYDEV_BOOTLOADER_IO_COMP) && (0u != ((CYDEV_BOOTLOADER_IO_COMP == CyBtldr_UART) || \
                                          (CYDEV_BOOTLOADER_IO_COMP == CyBtldr_Custom_Interface)))
    
#define PORT_BOOTLDR 666
#define LOCAL_ETH_BUFFER_SIZE 512
#define NUM_CON 2    
uint8_t socket_bootldr[NUM_CON];
uint8_t socket_bootldr_state[2];
uint8_t socket_tel;
uint8_t socket_tel_state;
uint8_t active_socket=0;
    
uint8_t buffer[LOCAL_ETH_BUFFER_SIZE];
uint16_t len;


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


cystatus ETH_CyBtldrCommWrite(const uint8 pData[], uint16 size, uint16 * count, uint8 timeOut) CYSMALL
         
{

    if(0u != timeOut)
    {
        /* Suppress compiler warning */
    }

    /* Clear receive buffers */
    if (active_socket != 0xFF) {

        if(size){
    	    ETH_TcpSend(active_socket,pData,size,ETH_TXRX_FLG_WAIT);
        }
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


//cystatus UART_CyBtldrCommRead(uint8 pData[], uint16 size, uint16 * count, uint8 timeOut) CYSMALL
//         
//{
//    uint16 iCntr;
//    uint16 dataIndexCntr;
//    uint16 tempCount;
//    uint16 oldDataCount;
//
//    cystatus status = CYRET_EMPTY;
//    
//    if (socket_bootldr != 0xFF) {
//    socket_bootldr_state = ETH_TcpPollSocket(socket_bootldr);
//
//    if (socket_bootldr_state == ETH_SR_ESTABLISHED) {
//        /* Check whether data is received within the time out period.
//        *  Time out period is in units of 10ms.
//        *  If at least one byte is received within the time out interval, wait for more data */
//            for (iCntr = 0u; iCntr < ((uint16)10u * timeOut); iCntr++)
//            {
//                /* If at least one byte is received within the timeout interval
//                *  enter the next loop waiting for more data reception
//                */
//                if(0u != ETH_RxDataReady(socket_bootldr))
//                {
//                    /* Wait for more data until 25ms byte to byte time out interval.
//                    * If no data is received during the last 25 ms(BYTE2BYTE_TIME_OUT)
//                    * then it is considered as end of transmitted data block(packet)
//                    * from the host and the program execution will break from the
//                    * data awaiting loop with status=CYRET_SUCCESS
//                    */
//                    do
//                    {
//                        oldDataCount = ETH_RxDataReady(socket_bootldr);
//                        CyDelayUs(UART_BYTE2BYTE_TIME_OUT);
//                        //CyDelay(5);
//                    }
//                    while(ETH_RxDataReady(socket_bootldr) > oldDataCount);
//
//                    status = CYRET_SUCCESS;
//                    break;
//                }
//                /* If the data is not received, give a delay of 
//                *  UART_BL_CHK_DELAY_MS and check again until the timeOut specified.
//                */
//                else
//                {
//                    CyDelay(UART_BL_CHK_DELAY_MS);
//                }
//            }
//
//            /* Initialize the data read indexes and Count value */
//            *count = 0u;
//            dataIndexCntr = 0u;
//
//            /* If GetRxBufferSize()>0 , move the received data to the pData buffer */
//            while(ETH_RxDataReady(socket_bootldr) > 0u)
//            {
//                tempCount = ETH_RxDataReady(socket_bootldr);
//                *count  =(*count) + tempCount;
//
//                /* Check if buffer overflow will occur before moving the data */
//                if(*count < size)
//                {
//                    for (iCntr = 0u; iCntr < tempCount; iCntr++)
//                    {
//                        /* Read the data and move it to the pData buffer */
//                        pData[dataIndexCntr] = ETH_receive_char();
//                        dataIndexCntr++;
//                    }
//
//                    /* Check if the last received byte is end of packet defined by bootloader
//                    *  If not wait for additional UART_WAIT_EOP_DELAY ms.
//                    */
//                    if(pData[dataIndexCntr - 1u] != UART_PACKET_EOP)
//                    {
//                        CyDelay(UART_WAIT_EOP_DELAY);
//                    }
//                }
//                /* If there is no space to move data, break from the loop */
//                else
//                {
//                    *count = (*count) - tempCount;
//                    break;
//                }
//            }
//        }
//        else if (socket_bootldr_state != ETH_SR_LISTEN) {
//        	ETH_SocketClose(socket_bootldr,1);
//        	socket_bootldr = 0xFF;
//        }
//    }
//    else {
//    socket_bootldr = ETH_TcpOpenServer(PORT_BOOTLDR);
//    }
//    CyDelay(1);
//    return (status);
//}

cystatus ETH_CyBtldrCommRead(uint8 pData[], uint16 size, uint16 * count, uint8 timeOut) CYSMALL
         
{
    uint16 iCntr;
    uint16 dataIndexCntr;
    uint16 oldDataCount;

    cystatus status = CYRET_EMPTY;
    for(uint8_t i=0;i<NUM_CON;i++){
        if (socket_bootldr[i] != 0xFF) {
            socket_bootldr_state[i] = ETH_TcpPollSocket(socket_bootldr[i]);
            if (socket_bootldr_state[i] == ETH_SR_ESTABLISHED) {
            /* Check whether data is received within the time out period.
            *  Time out period is in units of 10ms.
            *  If at least one byte is received within the time out interval, wait for more data */
                for (iCntr = 0u; iCntr < ((uint16)10u * timeOut); iCntr++)
                {
                    /* If at least one byte is received within the timeout interval
                    *  enter the next loop waiting for more data reception
                    */
                    if(0u != ETH_RxDataReady(socket_bootldr[i]))
                    {
                        active_socket=socket_bootldr[i];
                        /* Wait for more data until 25ms byte to byte time out interval.
                        * If no data is received during the last 25 ms(BYTE2BYTE_TIME_OUT)
                        * then it is considered as end of transmitted data block(packet)
                        * from the host and the program execution will break from the
                        * data awaiting loop with status=CYRET_SUCCESS
                        */
                        do
                        {
                            oldDataCount = ETH_RxDataReady(socket_bootldr[i]);
                            CyDelayUs(UART_BYTE2BYTE_TIME_OUT);
                            //CyDelay(5);
                        }
                        while(ETH_RxDataReady(socket_bootldr[i]) > oldDataCount);

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
                
                *count=ETH_TcpReceive(socket_bootldr[i],pData,size,0);
                //*count=size;
                

         
            }
            else if (socket_bootldr_state[i] != ETH_SR_LISTEN) {
            	ETH_SocketClose(socket_bootldr[i],1);
                active_socket=0xFF;
            	socket_bootldr[i] = 0xFF;
            }
        }
        else {
        socket_bootldr[i] = ETH_TcpOpenServer(PORT_BOOTLDR);
        }
    }
       
    if (socket_tel != 0xFF) {
        socket_tel_state = ETH_TcpPollSocket(socket_tel);

        if (socket_tel_state == ETH_SR_ESTABLISHED) {
               

         
        }else if (socket_tel_state != ETH_SR_LISTEN) {
            ETH_SocketClose(socket_tel,1);
            socket_tel = 0xFF;
            }
    }else {
        socket_tel = ETH_TcpOpenServer(23);
    }
    CyDelay(1);
    return (status);
 }


#endif /* end CYDEV_BOOTLOADER_IO_COMP */


/* [] END OF FILE */
