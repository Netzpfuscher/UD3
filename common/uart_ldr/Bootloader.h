/****************************************************************************//**
* \file Bootloader.c
* \version 1.60
*
* \brief
*   Provides an API for the Bootloader. The API includes functions for starting
*   bootloading operations, validating the application and jumping to the
*   application.
*
********************************************************************************
* \copyright
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_BOOTLOADER_Bootloader_H)
#define CY_BOOTLOADER_Bootloader_H

#include "cytypes.h"
#include "CyFlash.h"

#define Bootloader_DUAL_APP_BOOTLOADER        (0u)
#define Bootloader_BOOTLOADER_APP_VERSION     (0u)
#define Bootloader_FAST_APP_VALIDATION        (0u)
#define Bootloader_PACKET_CHECKSUM_CRC        (0u)
#define Bootloader_WAIT_FOR_COMMAND           (1u)
#define Bootloader_WAIT_FOR_COMMAND_TIME      (40u)
#define Bootloader_BOOTLOADER_APP_VALIDATION  (1u)

#define Bootloader_CMD_GET_FLASH_SIZE_AVAIL   (1u)
#define Bootloader_CMD_ERASE_ROW_AVAIL        (1u)
#define Bootloader_CMD_GET_ROW_CHKSUM_AVAIL   (1u)
#define Bootloader_CMD_SYNC_BOOTLOADER_AVAIL  (1u)
#define Bootloader_CMD_SEND_DATA_AVAIL        (1u)
#define Bootloader_CMD_VERIFY_APP_CHKSUM_AVAIL (1u)
#define Bootloader_CMD_GET_METADATA           (0u)
#define Bootloader_CMD_VERIFY_FLS_ROW_AVAIL   (0u)
#define Bootloader_GOLDEN_IMAGE_AVAIL         (0u)
#define Bootloader_SECURITY_KEY_AVAIL         (0u)
#define Bootloader_AUTO_SWITCHING_AVAIL       (1u)

#if ((0u != Bootloader_DUAL_APP_BOOTLOADER) || \
     (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER)) 
    #define Bootloader_CMD_GET_APP_STATUS_AVAIL   (1u)
#endif  /* (0u != Bootloader_DUAL_APP_BOOTLOADER) ||
           (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER)*/

#if (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LAUNCHER)
    #define Bootloader_COPIER_AVAIL           (0u)
#endif /*(CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LAUNCHER)*/

#if (0u != Bootloader_SECURITY_KEY_AVAIL) 
    #define Bootloader_SECURITY_KEY_LENGTH        (6u)
#endif    /*(0u != Bootloader_SECURITY_KEY_AVAIL)*/

/*******************************************************************************
* Copier definitions
*******************************************************************************/
#if (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LAUNCHER)
    #if (0u != Bootloader_COPIER_AVAIL)
    /**************************************************************************** 
    * Defines how many times Copier will try to copy the new app#0 image from
    * the temporary location to app#0 flash space (if it was not successful 
    * the first time) before giving up.
    ****************************************************************************/    
    #define Bootloader_MAX_ATTEMPTS      (3u)
    #endif /* (0u != Bootloader_COPIER_AVAIL) */
#endif /* (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LAUNCHER) */

#if ((CYDEV_PROJ_TYPE != CYDEV_PROJ_TYPE_LAUNCHER) && (!CY_PSOC3))
/*******************************************************************************
* Callback definitions
*******************************************************************************/   
/**
 \defgroup structures_group Structures
 @{
*/           
typedef struct 
{
    uint8 command;
    uint16 packetLength;
    uint8* pInputBuffer;
} Bootloader_in_packet_type;

typedef struct 
{
    cystatus status;
    uint16 packetLength;
    uint8* pOutputBuffer;
} Bootloader_out_packet_type;

/** @}*/

typedef void (*Bootloader_callback_type)(Bootloader_in_packet_type* inputPacket, 
                                               Bootloader_out_packet_type* outputPacket);

void Bootloader_InitCallback(Bootloader_callback_type userCallback) \
           CYSMALL ;
#endif /* (CYDEV_PROJ_TYPE != CYDEV_PROJ_TYPE_LAUNCHER) && (!CY_PSOC3) */
           
/*******************************************************************************
* Bootloadable applications identification
*******************************************************************************/
#define Bootloader_MD_BTLDB_ACTIVE_0          (0x00u)

#if ((0u != Bootloader_DUAL_APP_BOOTLOADER) || \
     (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER))
    #define Bootloader_MD_BTLDB_ACTIVE_1      (0x01u)
    #define Bootloader_MD_BTLDB_ACTIVE_NONE   (0x02u)

    #if (1u == Bootloader_GOLDEN_IMAGE_AVAIL)
        #define Bootloader_GOLDEN_IMAGE           (0x00u)
    #endif /*(1u == Bootloader_GOLDEN_IMAGE_AVAIL)*/
#endif  /* (0u != Bootloader_DUAL_APP_BOOTLOADER) || 
           (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER) */


/* Mask used to indicate starting application */
#define Bootloader_SCHEDULE_BTLDB             (0x80u)
#define Bootloader_SCHEDULE_BTLDR             (0x40u)
#define Bootloader_SCHEDULE_MASK              (0xC0u)
#define Bootloader_SCHEDULE_BTLDR_INIT_STATE  (0x00u)

#if (CYDEV_PROJ_TYPE != CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER)
#if defined(__ARMCC_VERSION) || defined (__GNUC__)
    __attribute__((section (".bootloader")))
#elif defined (__ICCARM__)
    #pragma location=".bootloader"
#endif  /* defined(__ARMCC_VERSION) || defined (__GNUC__) */
extern const uint8  CYCODE Bootloader_Checksum;
extern const uint8  CYCODE  *Bootloader_ChecksumAccess;


#if defined(__ARMCC_VERSION) || defined (__GNUC__)
    __attribute__((section (".bootloader")))
#elif defined (__ICCARM__)
    #pragma location=".bootloader"
#endif  /* defined(__ARMCC_VERSION) || defined (__GNUC__) */
extern const uint32 CYCODE Bootloader_SizeBytes;
extern const uint32 CYCODE *Bootloader_SizeBytesAccess;
#endif /*CYDEV_PROJ_TYPE != CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER*/

#if (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER) 
extern uint8 Bootloader_initVar;
extern uint8 Bootloader_runningApp;
extern uint8 Bootloader_isBootloading;
#endif /*CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER*/

#if ((CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LAUNCHER) && (CY_PSOC4))
    extern void Bootloader_UserCopierCallback(void);
    #define Bootloader_USER_COPIER_CALLBACK Bootloader_UserCopierCallback()
#endif /* (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LAUNCHER) && (CY_PSOC4) */

/*******************************************************************************
* This variable is used by the Bootloader/Bootloadable components to schedule which
* application will be started after a software reset.
*******************************************************************************/
#if (CY_PSOC4)
    #if defined(__ARMCC_VERSION)
        __attribute__ ((section(".bootloaderruntype"), zero_init))
    #elif defined (__GNUC__)
        __attribute__ ((section(".bootloaderruntype")))
   #elif defined (__ICCARM__)
        #pragma location=".bootloaderruntype"
    #endif  /* defined(__ARMCC_VERSION) */
    extern volatile uint32 cyBtldrRunType;
#endif  /* (CY_PSOC4) */


#if ((0u != Bootloader_DUAL_APP_BOOTLOADER) || \
     (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER))
    extern volatile uint8 Bootloader_activeApp;
#endif  /* (0u != Bootloader_DUAL_APP_BOOTLOADER) ||
           (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER)*/


#if(CY_PSOC4)
    /* Reset Cause Observation Register */
    #define Bootloader_RES_CAUSE_REG           (* (reg32 *) CYREG_RES_CAUSE)
    #define Bootloader_RES_CAUSE_PTR           (  (reg32 *) CYREG_RES_CAUSE)
#else
    #define Bootloader_RESET_SR0_REG           (* (reg8 *) CYREG_RESET_SR0)
    #define Bootloader_RESET_SR0_PTR           (  (reg8 *) CYREG_RESET_SR0)
#endif /* (CY_PSOC4) */


/*******************************************************************************
* Get the reason for a device reset
*  Return cyBtldrRunType in the case if a software reset was the reset reason and
*  set cyBtldrRunType to zero (the Bootloader application is scheduled - that is
*  the initial clean state) and return zero.
*******************************************************************************/
#if(CY_PSOC4)
    #define Bootloader_GET_RUN_TYPE           (cyBtldrRunType)
#else
    #define Bootloader_GET_RUN_TYPE       (Bootloader_RESET_SR0_REG & Bootloader_SCHEDULE_MASK)
#endif  /* (CY_PSOC4) */


/*******************************************************************************
* Schedule Bootloader/Bootloadable to be run after a software reset
*******************************************************************************/
#if(CY_PSOC4)
    #define Bootloader_SET_RUN_TYPE(x)                (cyBtldrRunType = (x))
#else
    #define Bootloader_SET_RUN_TYPE(x)                (Bootloader_RESET_SR0_REG = (x))
#endif  /* (CY_PSOC4) */


/* Returns number of flash arrays available in device. */
#ifndef CY_FLASH_NUMBER_ARRAYS
    #define CY_FLASH_NUMBER_ARRAYS                  (CYDEV_FLASH_SIZE / CYDEV_FLS_SECTOR_SIZE)
#endif /* CY_FLASH_NUMBER_ARRAYS */


/*******************************************************************************
* External References
*******************************************************************************/
/**
 \defgroup functions_group Functions
 @{
*/
#if (CYDEV_PROJ_TYPE != CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER)
    void CyBtldr_CheckLaunch(void)  CYSMALL ;
#endif /*CYDEV_PROJ_TYPE != CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER*/

void Bootloader_SetFlashByte(uint32 address, uint8 runType);
void Bootloader_Start(void) CYSMALL ;
cystatus Bootloader_ValidateBootloadable(uint8 appId) \
            CYSMALL ;
uint8 Bootloader_Calc8BitSum(uint32 baseAddr, uint32 start, uint32 size) CYSMALL \
                                    ;
uint32   Bootloader_GetMetadata(uint8 field, uint8 appId) \
                                    ;
void Bootloader_Exit(uint8 appId) CYSMALL ;

#if (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER)
    void Bootloader_HostLink(uint8 timeOut) CYLARGE ;    
    void Bootloader_Initialize(void)  CYSMALL ;
    uint8 Bootloader_GetRunningAppStatus(void) CYSMALL ;
    uint8 Bootloader_GetActiveAppStatus(void) CYSMALL ;
#endif /*(CYDEV_PROJ_TYPE != CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER)*/
/** @} */

#if(CY_PSOC3)
    /* Implementation for PSoC 3 resides in Bootloader_psoc3.a51 file.  */
    void     Bootloader_LaunchBootloadable(uint32 appAddr);
#endif  /* (CY_PSOC3) */

/* When using a custom interface as the IO Component, the user must provide these functions */
#if defined(CYDEV_BOOTLOADER_IO_COMP) && (CYDEV_BOOTLOADER_IO_COMP == CyBtldr_Custom_Interface)
#if ((CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER) || (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_BOOTLOADER) || \
     (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_MULTIAPPBOOTLOADER)) 
    extern void CyBtldrCommStart(void);
    extern void CyBtldrCommStop (void);
    extern void CyBtldrCommReset(void);
    extern cystatus CyBtldrCommWrite(uint8* buffer, uint16 size, uint16* count, uint8 timeOut);
    extern cystatus CyBtldrCommRead (uint8* buffer, uint16 size, uint16* count, uint8 timeOut);
#endif /*(CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER) || (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_BOOTLOADER) || \
         (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_MULTIAPPBOOTLOADER)*/
#endif  /* defined(CYDEV_BOOTLOADER_IO_COMP) && (CYDEV_BOOTLOADER_IO_COMP == CyBtldr_Custom_Interface) */

/*******************************************************************************
* Bootloader_Initialize()
*******************************************************************************/
#define Bootloader_BOTH_ACTIVE                (0x03u)

/**
 \defgroup constants_group Constants
 @{
*/

/** \addtogroup group_metadataFields Metadata Fields
 *  Metadata fields description 
 *  @{
 */
/*******************************************************************************
* Bootloader_GetMetadata()
*******************************************************************************/
#define Bootloader_GET_BTLDB_CHECKSUM         (1u)   /**< Bootloadable Application Checksum */
#define Bootloader_GET_BTLDB_ADDR             (2u)   /**< Bootloadable Application Start Routine Address */
#define Bootloader_GET_BTLDR_LAST_ROW         (3u)   /**< Bootloader Last Flash Row */
#define Bootloader_GET_BTLDB_LENGTH           (4u)   /**< Bootloadable Application Length */
#define Bootloader_GET_BTLDB_ACTIVE           (5u)   /**< Active Bootloadable Application */
#define Bootloader_GET_BTLDB_STATUS           (6u)   /**< Bootloadable Application Verification Status */
#define Bootloader_GET_BTLDR_APP_VERSION      (7u)   /**< Bootloader Application Version */
#define Bootloader_GET_BTLDB_APP_VERSION      (8u)   /**< Bootloadable Application Version */
#define Bootloader_GET_BTLDB_APP_ID           (9u)   /**< Bootloadable Application ID */
#define Bootloader_GET_BTLDB_APP_CUST_ID      (10u)  /**< Bootloadable Application Custom ID */
#define Bootloader_GET_BTLDB_COPY_FLAG        (11u)  /**< "need-to-copy flag */
#define Bootloader_GET_BTLDB_USER_DATA        (12u)  /**< User data */

/** @} group_metadataFields */
/** @} constants_group */

#define Bootloader_GET_METADATA_RESPONSE_SIZE (56u)  

/*******************************************************************************
* Bootloader_Exit()
*******************************************************************************/
#define Bootloader_EXIT_TO_BTLDR              (2u)
#define Bootloader_EXIT_TO_BTLDB              (0u)
#if ((0u != Bootloader_DUAL_APP_BOOTLOADER) || \
    (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER))
    #define Bootloader_EXIT_TO_BTLDB_1        (0u)
    #define Bootloader_EXIT_TO_BTLDB_2        (1u)
#endif  /* (0u != Bootloader_DUAL_APP_BOOTLOADER) ||
           (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER)*/

#if (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER)
/*******************************************************************************
* In-app Bootloader definitions
*******************************************************************************/
#define Bootloader_BOOTLOADING_IN_PROGRESS     (1u)
#define Bootloader_BOOTLOADING_COMPLETED       (0u)

#define Bootloader_BOOTLADING_INITIALIZED      (1u)
#define Bootloader_BOOTLOADING_NOT_INITIALIZED (0u)

#define Bootloader_RUNNING_APPLICATION_0       (0u)     
#define Bootloader_RUNNING_APPLICATION_1       (1u)     
#define Bootloader_RUNNING_APPLICATION_UNKNOWN (2u)     
#endif /*CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER*/    


/*******************************************************************************
* Input/output packet defines
*******************************************************************************/
#define Bootloader_MIN_PKT_SIZE             (7u)   /* The minimum number of bytes in a packet */
#define Bootloader_SIZEOF_COMMAND_BUFFER    (300u) /* Maximum number of bytes accepted in one packet plus some */

/*******************************************************************************
* Kept for backward compatibility.
*******************************************************************************/
#if ((0u != Bootloader_DUAL_APP_BOOTLOADER) || (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER))
    #define Bootloader_ValidateApp(x)                 Bootloader_ValidateBootloadable((x))
    #define Bootloader_ValidateApplication()            \
                            Bootloader_ValidateBootloadable(Bootloader_MD_BTLDB_ACTIVE_0)
#else
    #define Bootloader_ValidateApplication()            \
                            Bootloader_ValidateBootloadable(Bootloader_MD_BTLDB_ACTIVE_0)
    #define Bootloader_ValidateApp(x)                 Bootloader_ValidateBootloadable((x))
#endif  /* (0u != Bootloader_DUAL_APP_BOOTLOADER) || (CYDEV_PROJ_TYPE == CYDEV_PROJ_TYPE_LOADABLEANDBOOTLOADER)*/
#define Bootloader_Calc8BitFlashSum(start, size)      Bootloader_Calc8BitSum(CY_FLASH_BASE, (start), (size))


/*******************************************************************************
* The following code is DEPRECATED and must not be used.
*******************************************************************************/
#define Bootloader_CMD_VERIFY_ROW_AVAIL           Bootloader_CMD_GET_ROW_CHKSUM_AVAIL

#define Bootloader_BOOTLOADABLE_APP_VALID         (Bootloader_BOOTLOADER_APP_VALIDATION)
#define CyBtldr_Start                                    Bootloader_Start

#define Bootloader_NUM_OF_FLASH_ARRAYS            (CYDEV_FLASH_SIZE / CYDEV_FLS_SECTOR_SIZE)
#define Bootloader_META_BASE(x)                   (CYDEV_FLASH_BASE + \
                                                            (CYDEV_FLASH_SIZE - (( uint32 )(x) * CYDEV_FLS_ROW_SIZE) - \
                                                            Bootloader_META_DATA_SIZE))
#define Bootloader_META_ARRAY                     (Bootloader_NUM_OF_FLASH_ARRAYS - 1u)
#define Bootloader_META_APP_ENTRY_POINT_ADDR(x)   (Bootloader_META_BASE(x) + \
                                                            Bootloader_META_APP_ADDR_OFFSET)
#define Bootloader_META_APP_BYTE_LEN(x)           (Bootloader_META_BASE(x) + \
                                                            Bootloader_META_APP_BYTE_LEN_OFFSET)
#define Bootloader_META_APP_RUN_ADDR(x)           (Bootloader_META_BASE(x) + \
                                                            Bootloader_META_APP_RUN_TYPE_OFFSET)
#define Bootloader_META_APP_ACTIVE_ADDR(x)        (Bootloader_META_BASE(x) + \
                                                            Bootloader_META_APP_ACTIVE_OFFSET)
#define Bootloader_META_APP_VERIFIED_ADDR(x)      (Bootloader_META_BASE(x) + \
                                                            Bootloader_META_APP_VERIFIED_OFFSET)
#define Bootloader_META_APP_BLDBL_VER_ADDR(x)     (Bootloader_META_BASE(x) + \
                                                            Bootloader_META_APP_BL_BUILD_VER_OFFSET)
#define Bootloader_META_APP_VER_ADDR(x)           (Bootloader_META_BASE(x) + \
                                                            Bootloader_META_APP_VER_OFFSET)
#define Bootloader_META_APP_ID_ADDR(x)            (Bootloader_META_BASE(x) + \
                                                            Bootloader_META_APP_ID_OFFSET)
#define Bootloader_META_APP_CUST_ID_ADDR(x)       (Bootloader_META_BASE(x) + \
                                                            Bootloader_META_APP_CUST_ID_OFFSET)
#define Bootloader_META_LAST_BLDR_ROW_ADDR(x)     (Bootloader_META_BASE(x) + \
                                                            Bootloader_META_APP_BL_LAST_ROW_OFFSET)
#define Bootloader_META_CHECKSUM_ADDR(x)          (Bootloader_META_BASE(x) + \
                                                            Bootloader_META_APP_CHECKSUM_OFFSET)
#if(0u == Bootloader_DUAL_APP_BOOTLOADER)
    #define Bootloader_MD_BASE                    Bootloader_META_BASE(0u)

    #if(!CY_PSOC4)
        #define Bootloader_MD_ROW                 ((CY_FLASH_NUMBER_ROWS / Bootloader_NUM_OF_FLASH_ARRAYS) \
                                                        - 1u)
    #else
        #define Bootloader_MD_ROW                 (CY_FLASH_NUMBER_ROWS - 1u)
    #endif /* (CY_PSOC4) */

    #define Bootloader_MD_CHECKSUM_ADDR           Bootloader_META_CHECKSUM_ADDR(0u)
    #define Bootloader_MD_LAST_BLDR_ROW_ADDR      Bootloader_META_LAST_BLDR_ROW_ADDR(0u)
    #define Bootloader_MD_APP_BYTE_LEN            Bootloader_META_APP_BYTE_LEN(0u)
    #define Bootloader_MD_APP_VERIFIED_ADDR       Bootloader_META_APP_VERIFIED_ADDR(0u)
    #define Bootloader_MD_APP_ENTRY_POINT_ADDR    Bootloader_META_APP_ENTRY_POINT_ADDR(0u)
    #define Bootloader_MD_APP_RUN_ADDR            Bootloader_META_APP_RUN_ADDR(0u)
#else
    #if(!CY_PSOC4)
        #define Bootloader_MD_ROW(x)              ((CY_FLASH_NUMBER_ROWS / Bootloader_NUM_OF_FLASH_ARRAYS) \
                                                        - 1u - ( uint32 )(x))
    #else
        #define Bootloader_MD_ROW(x)              (CY_FLASH_NUMBER_ROWS - 1u - ( uint32 )(x))
    #endif /* (CY_PSOC4) */

    #define Bootloader_MD_CHECKSUM_ADDR           Bootloader_META_CHECKSUM_ADDR(appId)
    #define Bootloader_MD_LAST_BLDR_ROW_ADDR      Bootloader_META_LAST_BLDR_ROW_ADDR(appId)
    #define Bootloader_MD_APP_BYTE_LEN            Bootloader_META_APP_BYTE_LEN(appId)
    #define Bootloader_MD_APP_VERIFIED_ADDR       Bootloader_META_APP_VERIFIED_ADDR(appId)
    #define Bootloader_MD_APP_ENTRY_POINT_ADDR    \
                                                Bootloader_META_APP_ENTRY_POINT_ADDR(Bootloader_activeApp)
    #define Bootloader_MD_APP_RUN_ADDR            Bootloader_META_APP_RUN_ADDR(Bootloader_activeApp)
#endif  /* (0u == Bootloader_DUAL_APP_BOOTLOADER) */

#define Bootloader_P_APP_ACTIVE(x)                ((uint8 CYCODE *) Bootloader_META_APP_ACTIVE_ADDR(x))
#define Bootloader_MD_PTR_CHECKSUM                ((uint8  CYCODE *) Bootloader_MD_CHECKSUM_ADDR)
#define Bootloader_MD_PTR_APP_ENTRY_POINT         ((Bootloader_APP_ADDRESS CYCODE *) \
                                                                Bootloader_MD_APP_ENTRY_POINT_ADDR)
#define Bootloader_MD_PTR_LAST_BLDR_ROW            ((uint16 CYCODE *) Bootloader_MD_LAST_BLDR_ROW_ADDR)
#define Bootloader_MD_PTR_APP_BYTE_LEN             ((Bootloader_APP_ADDRESS CYCODE *) \
                                                                Bootloader_MD_APP_BYTE_LEN)
#define Bootloader_MD_PTR_APP_RUN_ADDR             ((uint8  CYCODE *) Bootloader_MD_APP_RUN_ADDR)
#define Bootloader_MD_PTR_APP_VERIFIED             ((uint8  CYCODE *) Bootloader_MD_APP_VERIFIED_ADDR)
#define Bootloader_MD_PTR_APP_BLD_BL_VER           ((uint16 CYCODE *) Bootloader_MD_APP_BLDBL_VER_ADDR)
#define Bootloader_MD_PTR_APP_VER                  ((uint16 CYCODE *) Bootloader_MD_APP_VER_ADDR)
#define Bootloader_MD_PTR_APP_ID                   ((uint16 CYCODE *) Bootloader_MD_APP_ID_ADDR)
#define Bootloader_MD_PTR_APP_CUST_ID              ((uint32 CYCODE *) Bootloader_MD_APP_CUST_ID_ADDR)
#if(CY_PSOC3)
    #define Bootloader_APP_ADDRESS                    uint16
    #define Bootloader_GET_CODE_DATA(idx)             (*((uint8  CYCODE *) (idx)))
    #define Bootloader_GET_CODE_WORD(idx)             (*((uint32 CYCODE *) (idx)))
    #define Bootloader_META_APP_ADDR_OFFSET           (3u)
    #define Bootloader_META_APP_BL_LAST_ROW_OFFSET    (7u)
    #define Bootloader_META_APP_BYTE_LEN_OFFSET       (11u)
    #define Bootloader_META_APP_RUN_TYPE_OFFSET       (15u)
#else
    #define Bootloader_APP_ADDRESS                    uint32
    #define Bootloader_GET_CODE_DATA(idx)             (*((uint8  *)(CYDEV_FLASH_BASE + (idx))))
    #define Bootloader_GET_CODE_WORD(idx)             (*((uint32 *)(CYDEV_FLASH_BASE + (idx))))
    #define Bootloader_META_APP_ADDR_OFFSET           (1u)
    #define Bootloader_META_APP_BL_LAST_ROW_OFFSET    (5u)
    #define Bootloader_META_APP_BYTE_LEN_OFFSET       (9u)
    #define Bootloader_META_APP_RUN_TYPE_OFFSET       (13u)
#endif /* (CY_PSOC3) */

#define Bootloader_META_APP_ACTIVE_OFFSET             (16u)
#define Bootloader_META_APP_VERIFIED_OFFSET           (17u)
#define Bootloader_META_APP_BL_BUILD_VER_OFFSET       (18u)
#define Bootloader_META_APP_ID_OFFSET                 (20u)
#define Bootloader_META_APP_VER_OFFSET                (22u)
#define Bootloader_META_APP_CUST_ID_OFFSET            (24u)

#if (CY_PSOC4)
    #define Bootloader_GET_REG16(x)   ((uint16)(                                                          \
                                                (( uint16 )(( uint16 )CY_GET_XTND_REG8((x)     )       ))   |   \
                                                (( uint16 )(( uint16 )CY_GET_XTND_REG8((x) + 1u) <<  8u))       \
                                            ))

    #define Bootloader_GET_REG32(x)   (                                                                    \
                                                (( uint32 )(( uint32 ) CY_GET_XTND_REG8((x)     )       ))   |   \
                                                (( uint32 )(( uint32 ) CY_GET_XTND_REG8((x) + 1u) <<  8u))   |   \
                                                (( uint32 )(( uint32 ) CY_GET_XTND_REG8((x) + 2u) << 16u))   |   \
                                                (( uint32 )(( uint32 ) CY_GET_XTND_REG8((x) + 3u) << 24u))       \
                                            )
#endif  /* (CY_PSOC4) */
#define Bootloader_META_APP_CHECKSUM_OFFSET           (0u)
#define Bootloader_META_DATA_SIZE                     (64u)
#if(CY_PSOC4)
    extern uint8 appRunType;
#endif  /* (CY_PSOC4) */

#if(CY_PSOC4)
    #define Bootloader_SOFTWARE_RESET                 CY_SET_REG32(CYREG_CM0_AIRCR, 0x05FA0004u)
#else
    #define Bootloader_SOFTWARE_RESET                 CY_SET_REG8(CYREG_RESET_CR2, 0x01u)
#endif  /* (CY_PSOC4) */

#define Bootloader_SetFlashRunType(runType)        Bootloader_SetFlashByte( \
                                                            Bootloader_MD_APP_RUN_ADDR(0), (runType))

#define Bootloader_START_APP                  (Bootloader_SCHEDULE_BTLDB)
#define Bootloader_START_BTLDR                (Bootloader_SCHEDULE_BTLDR)

/* Some PSoC Creator versions are used to generate only one name types */
#if !defined (CYDEV_FLASH_BASE)
    #define CYDEV_FLASH_BASE                                (CYDEV_FLS_BASE)
#endif /* !defined (CYDEV_FLASH_BASE) */

#if !defined (CYDEV_FLASH_SIZE)
    #define CYDEV_FLASH_SIZE                                (CYDEV_FLS_SIZE)
#endif /* CYDEV_FLASH_SIZE */


#endif /* CY_BOOTLOADER_Bootloader_H */


/* [] END OF FILE */
