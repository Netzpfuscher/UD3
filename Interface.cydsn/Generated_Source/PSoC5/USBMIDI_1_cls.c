/***************************************************************************//**
* \file USBMIDI_1_cls.c
* \version 3.10
*
* \brief
*  This file contains the USB Class request handler.
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

#if(USBMIDI_1_EXTERN_CLS == USBMIDI_1_FALSE)

/***************************************
* User Implemented Class Driver Declarations.
***************************************/
/* `#START USER_DEFINED_CLASS_DECLARATIONS` Place your declaration here */

/* `#END` */


/*******************************************************************************
* Function Name: USBMIDI_1_DispatchClassRqst
****************************************************************************//**
*  This routine dispatches class specific requests depend on interface class.
*
* \return
*  requestHandled.
*
* \reentrant
*  No.
*
*******************************************************************************/
uint8 USBMIDI_1_DispatchClassRqst(void) 
{
    uint8 requestHandled;
    uint8 interfaceNumber;

    /* Get interface to which request is intended. */
    switch (USBMIDI_1_bmRequestTypeReg & USBMIDI_1_RQST_RCPT_MASK)
    {
        case USBMIDI_1_RQST_RCPT_IFC:
            /* Class-specific request directed to interface: wIndexLoReg 
            * contains interface number.
            */
            interfaceNumber = (uint8) USBMIDI_1_wIndexLoReg;
            break;
        
        case USBMIDI_1_RQST_RCPT_EP:
            /* Class-specific request directed to endpoint: wIndexLoReg contains 
            * endpoint number. Find interface related to endpoint, 
            */
            interfaceNumber = USBMIDI_1_EP[USBMIDI_1_wIndexLoReg & USBMIDI_1_DIR_UNUSED].interface;
            break;
            
        default:
            /* Default interface is zero. */
            interfaceNumber = 0u;
            break;
    }

#if (defined(USBMIDI_1_ENABLE_HID_CLASS) ||\
            defined(USBMIDI_1_ENABLE_AUDIO_CLASS) ||\
            defined(USBMIDI_1_ENABLE_CDC_CLASS) ||\
            USBMIDI_1_ENABLE_MSC_CLASS)

    /* Handle class request depends on interface type. */
    switch (USBMIDI_1_interfaceClass[interfaceNumber])
    {
    #if defined(USBMIDI_1_ENABLE_HID_CLASS)
        case USBMIDI_1_CLASS_HID:
            requestHandled = USBMIDI_1_DispatchHIDClassRqst();
            break;
    #endif /* (USBMIDI_1_ENABLE_HID_CLASS) */
            
    #if defined(USBMIDI_1_ENABLE_AUDIO_CLASS)
        case USBMIDI_1_CLASS_AUDIO:
            requestHandled = USBMIDI_1_DispatchAUDIOClassRqst();
            break;
    #endif /* (USBMIDI_1_CLASS_AUDIO) */
            
    #if defined(USBMIDI_1_ENABLE_CDC_CLASS)
        case USBMIDI_1_CLASS_CDC:
            requestHandled = USBMIDI_1_DispatchCDCClassRqst();
            break;
    #endif /* (USBMIDI_1_ENABLE_CDC_CLASS) */
        
    #if (USBMIDI_1_ENABLE_MSC_CLASS)
        case USBMIDI_1_CLASS_MSD:
        #if (USBMIDI_1_HANDLE_MSC_REQUESTS)
            /* MSC requests are handled by the component. */
            requestHandled = USBMIDI_1_DispatchMSCClassRqst();
        #elif defined(USBMIDI_1_DISPATCH_MSC_CLASS_RQST_CALLBACK)
            /* MSC requests are handled by user defined callbcak. */
            requestHandled = USBMIDI_1_DispatchMSCClassRqst_Callback();
        #else
            /* MSC requests are not handled. */
            requestHandled = USBMIDI_1_FALSE;
        #endif /* (USBMIDI_1_HANDLE_MSC_REQUESTS) */
            break;
    #endif /* (USBMIDI_1_ENABLE_MSC_CLASS) */
        
        default:
            /* Request is not handled: unknown class request type. */
            requestHandled = USBMIDI_1_FALSE;
            break;
    }
#else /*No class is defined*/
    if (0u != interfaceNumber)
    {
        /* Suppress warning message */
    }
    requestHandled = USBMIDI_1_FALSE;
#endif /*HID or AUDIO or MSC or CDC class enabled*/

    /* `#START USER_DEFINED_CLASS_CODE` Place your Class request here */

    /* `#END` */

#ifdef USBMIDI_1_DISPATCH_CLASS_RQST_CALLBACK
    if (USBMIDI_1_FALSE == requestHandled)
    {
        requestHandled = USBMIDI_1_DispatchClassRqst_Callback(interfaceNumber);
    }
#endif /* (USBMIDI_1_DISPATCH_CLASS_RQST_CALLBACK) */

    return (requestHandled);
}


/*******************************************************************************
* Additional user functions supporting Class Specific Requests
********************************************************************************/

/* `#START CLASS_SPECIFIC_FUNCTIONS` Place any additional functions here */

/* `#END` */

#endif /* USBMIDI_1_EXTERN_CLS */


/* [] END OF FILE */
