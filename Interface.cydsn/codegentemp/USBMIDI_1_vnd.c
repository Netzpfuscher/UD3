/***************************************************************************//**
* \file USBMIDI_1_vnd.c
* \version 3.10
*
* \brief
*  This file contains the  USB vendor request handler.
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

#if(USBMIDI_1_EXTERN_VND == USBMIDI_1_FALSE)

/***************************************
* Vendor Specific Declarations
***************************************/

/* `#START VENDOR_SPECIFIC_DECLARATIONS` Place your declaration here */

/* `#END` */


/*******************************************************************************
* Function Name: USBMIDI_1_HandleVendorRqst
****************************************************************************//**
*
*  This routine provide users with a method to implement vendor specific
*  requests.
*
*  To implement vendor specific requests, add your code in this function to
*  decode and disposition the request.  If the request is handled, your code
*  must set the variable "requestHandled" to TRUE, indicating that the
*  request has been handled.
*
* \return
*  requestHandled.
*
* \reentrant
*  No.
*
*******************************************************************************/
uint8 USBMIDI_1_HandleVendorRqst(void) 
{
    uint8 requestHandled = USBMIDI_1_FALSE;

    /* Check request direction: D2H or H2D. */
    if (0u != (USBMIDI_1_bmRequestTypeReg & USBMIDI_1_RQST_DIR_D2H))
    {
        /* Handle direction from device to host. */
        
        switch (USBMIDI_1_bRequestReg)
        {
            case USBMIDI_1_GET_EXTENDED_CONFIG_DESCRIPTOR:
            #if defined(USBMIDI_1_ENABLE_MSOS_STRING)
                USBMIDI_1_currentTD.pData = (volatile uint8 *) &USBMIDI_1_MSOS_CONFIGURATION_DESCR[0u];
                USBMIDI_1_currentTD.count = USBMIDI_1_MSOS_CONFIGURATION_DESCR[0u];
                requestHandled  = USBMIDI_1_InitControlRead();
            #endif /* (USBMIDI_1_ENABLE_MSOS_STRING) */
                break;
            
            default:
                break;
        }
    }

    /* `#START VENDOR_SPECIFIC_CODE` Place your vendor specific request here */

    /* `#END` */

#ifdef USBMIDI_1_HANDLE_VENDOR_RQST_CALLBACK
    if (USBMIDI_1_FALSE == requestHandled)
    {
        requestHandled = USBMIDI_1_HandleVendorRqst_Callback();
    }
#endif /* (USBMIDI_1_HANDLE_VENDOR_RQST_CALLBACK) */

    return (requestHandled);
}


/*******************************************************************************
* Additional user functions supporting Vendor Specific Requests
********************************************************************************/

/* `#START VENDOR_SPECIFIC_FUNCTIONS` Place any additional functions here */

/* `#END` */


#endif /* USBMIDI_1_EXTERN_VND */


/* [] END OF FILE */
