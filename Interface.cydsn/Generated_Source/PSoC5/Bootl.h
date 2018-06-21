/*******************************************************************************
* File Name: Bootl.h  
* Version 2.20
*
* Description:
*  This file contains Pin function prototypes and register defines
*
* Note:
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_PINS_Bootl_H) /* Pins Bootl_H */
#define CY_PINS_Bootl_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Bootl_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Bootl__PORT == 15 && ((Bootl__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Bootl_Write(uint8 value);
void    Bootl_SetDriveMode(uint8 mode);
uint8   Bootl_ReadDataReg(void);
uint8   Bootl_Read(void);
void    Bootl_SetInterruptMode(uint16 position, uint16 mode);
uint8   Bootl_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Bootl_SetDriveMode() function.
     *  @{
     */
        #define Bootl_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Bootl_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Bootl_DM_RES_UP          PIN_DM_RES_UP
        #define Bootl_DM_RES_DWN         PIN_DM_RES_DWN
        #define Bootl_DM_OD_LO           PIN_DM_OD_LO
        #define Bootl_DM_OD_HI           PIN_DM_OD_HI
        #define Bootl_DM_STRONG          PIN_DM_STRONG
        #define Bootl_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Bootl_MASK               Bootl__MASK
#define Bootl_SHIFT              Bootl__SHIFT
#define Bootl_WIDTH              1u

/* Interrupt constants */
#if defined(Bootl__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Bootl_SetInterruptMode() function.
     *  @{
     */
        #define Bootl_INTR_NONE      (uint16)(0x0000u)
        #define Bootl_INTR_RISING    (uint16)(0x0001u)
        #define Bootl_INTR_FALLING   (uint16)(0x0002u)
        #define Bootl_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Bootl_INTR_MASK      (0x01u) 
#endif /* (Bootl__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Bootl_PS                     (* (reg8 *) Bootl__PS)
/* Data Register */
#define Bootl_DR                     (* (reg8 *) Bootl__DR)
/* Port Number */
#define Bootl_PRT_NUM                (* (reg8 *) Bootl__PRT) 
/* Connect to Analog Globals */                                                  
#define Bootl_AG                     (* (reg8 *) Bootl__AG)                       
/* Analog MUX bux enable */
#define Bootl_AMUX                   (* (reg8 *) Bootl__AMUX) 
/* Bidirectional Enable */                                                        
#define Bootl_BIE                    (* (reg8 *) Bootl__BIE)
/* Bit-mask for Aliased Register Access */
#define Bootl_BIT_MASK               (* (reg8 *) Bootl__BIT_MASK)
/* Bypass Enable */
#define Bootl_BYP                    (* (reg8 *) Bootl__BYP)
/* Port wide control signals */                                                   
#define Bootl_CTL                    (* (reg8 *) Bootl__CTL)
/* Drive Modes */
#define Bootl_DM0                    (* (reg8 *) Bootl__DM0) 
#define Bootl_DM1                    (* (reg8 *) Bootl__DM1)
#define Bootl_DM2                    (* (reg8 *) Bootl__DM2) 
/* Input Buffer Disable Override */
#define Bootl_INP_DIS                (* (reg8 *) Bootl__INP_DIS)
/* LCD Common or Segment Drive */
#define Bootl_LCD_COM_SEG            (* (reg8 *) Bootl__LCD_COM_SEG)
/* Enable Segment LCD */
#define Bootl_LCD_EN                 (* (reg8 *) Bootl__LCD_EN)
/* Slew Rate Control */
#define Bootl_SLW                    (* (reg8 *) Bootl__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Bootl_PRTDSI__CAPS_SEL       (* (reg8 *) Bootl__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Bootl_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Bootl__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Bootl_PRTDSI__OE_SEL0        (* (reg8 *) Bootl__PRTDSI__OE_SEL0) 
#define Bootl_PRTDSI__OE_SEL1        (* (reg8 *) Bootl__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Bootl_PRTDSI__OUT_SEL0       (* (reg8 *) Bootl__PRTDSI__OUT_SEL0) 
#define Bootl_PRTDSI__OUT_SEL1       (* (reg8 *) Bootl__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Bootl_PRTDSI__SYNC_OUT       (* (reg8 *) Bootl__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Bootl__SIO_CFG)
    #define Bootl_SIO_HYST_EN        (* (reg8 *) Bootl__SIO_HYST_EN)
    #define Bootl_SIO_REG_HIFREQ     (* (reg8 *) Bootl__SIO_REG_HIFREQ)
    #define Bootl_SIO_CFG            (* (reg8 *) Bootl__SIO_CFG)
    #define Bootl_SIO_DIFF           (* (reg8 *) Bootl__SIO_DIFF)
#endif /* (Bootl__SIO_CFG) */

/* Interrupt Registers */
#if defined(Bootl__INTSTAT)
    #define Bootl_INTSTAT            (* (reg8 *) Bootl__INTSTAT)
    #define Bootl_SNAP               (* (reg8 *) Bootl__SNAP)
    
	#define Bootl_0_INTTYPE_REG 		(* (reg8 *) Bootl__0__INTTYPE)
#endif /* (Bootl__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Bootl_H */


/* [] END OF FILE */
