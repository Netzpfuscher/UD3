/* ============================================================================
 * File Name: `$INSTANCE_NAME`.h
 * Version `$CY_MAJOR_VERSION`.`$CY_MINOR_VERSION`
 * 
 * Description:
 *   DDS32: up to 32-bit DDS frequency generator component (v0.0)
 *   adjustable resolution: 8, 16, 24, 32-bits
 *   realized in UDB datapath
 *
 * Credits:
 *   based on original DDS code by <voha6>:
 *   http://kazus.ru/forums/showthread.php?t=21022&page=10
 * 
 *   Special thanks to <pavloven>, <kabron>
 *
 * Note:
 *   frequency resolution = clock_frequency / 2^N, N=8, 16, 24, 32 
 *   frequency maximum    = clock_frequency / 2
 *   frequency minimum    = clock_frequency / 2^N
 *
 * ============================================================================
 * PROVIDED AS-IS, NO WARRANTY OF ANY KIND, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * FREE TO SHARE, USE AND MODIFY UNDER TERMS: CREATIVE COMMONS - SHARE ALIKE
 * ============================================================================
*/

#include "cytypes.h"
#include "cyfitter.h"


#ifndef `$INSTANCE_NAME`_H
#define `$INSTANCE_NAME`_H
    

#define true  1 // but not True
#define false 0 // but not False
     

/***************************************        
*   Conditional Compilation Parameters
***************************************/

#define `$INSTANCE_NAME`_BUS_WIDTH           (`$BusWidth`u)             // DDS accumulator bit width
#define `$INSTANCE_NAME`_CLOCK_FREQ (double) `=$ClockFreq`              // input clock frequency
#define `$INSTANCE_NAME`_CR_ENABLE           `=($EnableMode == CR_only || $EnableMode == HW_and_CR)`  // enable mode = SW or SW_&_HW
#define `$INSTANCE_NAME`_PresetFreq0 (double) `=$Frequency0`              // preset startup frequency 
#define `$INSTANCE_NAME`_PresetFreq1 (double) `=$Frequency1`              // preset startup frequency 
 
    
    
/***************************************
*   Encapsulation        
***************************************/   

#define `$INSTANCE_NAME`_Disable()  `$INSTANCE_NAME`_Stop()             // stop clock (?)
    
#define `$INSTANCE_NAME`_Frequency  `$INSTANCE_NAME`_GetFrequency()     // return current set frequency
#define `$INSTANCE_NAME`_OutpFreq   `$INSTANCE_NAME`_GetOutFreq()       // read actual output frequency
#define `$INSTANCE_NAME`_Enabled    `$INSTANCE_NAME`_GetCREnable()      // read enabled status
    
// todo: Set/Get?    
#if   (`$INSTANCE_NAME`_BUS_WIDTH == 8u)
    #define `$INSTANCE_NAME`_ReadStep0()     ((uint8)  CY_GET_REG8(`$INSTANCE_NAME`_STEP0_PTR))
    #define `$INSTANCE_NAME`_ReadStep1()     ((uint8)  CY_GET_REG8(`$INSTANCE_NAME`_STEP1_PTR))
    #define `$INSTANCE_NAME`_WriteStep0(val)           CY_SET_REG8(`$INSTANCE_NAME`_STEP0_PTR , val) 
    #define `$INSTANCE_NAME`_WriteStep1(val)           CY_SET_REG8(`$INSTANCE_NAME`_STEP0_PTR , val) 
#elif (`$INSTANCE_NAME`_BUS_WIDTH == 16u)
    #define `$INSTANCE_NAME`_ReadStep0()     ((uint16) CY_GET_REG16(`$INSTANCE_NAME`_STEP0_PTR))
    #define `$INSTANCE_NAME`_ReadStep1()     ((uint16) CY_GET_REG16(`$INSTANCE_NAME`_STEP1_PTR))
    #define `$INSTANCE_NAME`_WriteStep0(val)          CY_SET_REG16(`$INSTANCE_NAME`_STEP0_PTR , val)
    #define `$INSTANCE_NAME`_WriteStep1(val)          CY_SET_REG16(`$INSTANCE_NAME`_STEP1_PTR , val)
#elif (`$INSTANCE_NAME`_BUS_WIDTH == 24u)
    #define `$INSTANCE_NAME`_ReadStep0()     ((uint32) CY_GET_REG24(`$INSTANCE_NAME`_STEP0_PTR))
    #define `$INSTANCE_NAME`_ReadStep1()     ((uint32) CY_GET_REG24(`$INSTANCE_NAME`_STEP1_PTR))
    #define `$INSTANCE_NAME`_WriteStep0(val)          CY_SET_REG24(`$INSTANCE_NAME`_STEP0_PTR , val)
    #define `$INSTANCE_NAME`_WriteStep1(val)          CY_SET_REG24(`$INSTANCE_NAME`_STEP1_PTR , val)
#elif (`$INSTANCE_NAME`_BUS_WIDTH == 32u)
    #define `$INSTANCE_NAME`_ReadStep0()     ((uint32) CY_GET_REG32(`$INSTANCE_NAME`_STEP0_PTR))
    #define `$INSTANCE_NAME`_ReadStep1()     ((uint32) CY_GET_REG32(`$INSTANCE_NAME`_STEP1_PTR))
    #define `$INSTANCE_NAME`_WriteStep0(val)          CY_SET_REG32(`$INSTANCE_NAME`_STEP0_PTR , val)
    #define `$INSTANCE_NAME`_WriteStep1(val)          CY_SET_REG32(`$INSTANCE_NAME`_STEP1_PTR , val)
#endif  


  
/***************************************
*             Registers                 
***************************************/
// todo: not work with DMA
// DDS_1_sD16_DDSdp_u0__16BIT_D0_REG
#if   (`$INSTANCE_NAME`_BUS_WIDTH == 8u)
	#define `$INSTANCE_NAME`_STEP0_PTR	((reg8 *) `$INSTANCE_NAME`_sD8_DDSdp_u0__D0_REG)
    #define `$INSTANCE_NAME`_STEP1_PTR	((reg8 *) `$INSTANCE_NAME`_sD8_DDSdp_u0__D1_REG)
#elif (`$INSTANCE_NAME`_BUS_WIDTH == 16u)
	#define `$INSTANCE_NAME`_STEP0_PTR	((reg8 *) `$INSTANCE_NAME`_sD16_DDSdp_u0__D0_REG)
    #define `$INSTANCE_NAME`_STEP1_PTR	((reg8 *) `$INSTANCE_NAME`_sD16_DDSdp_u0__D1_REG)
#elif (`$INSTANCE_NAME`_BUS_WIDTH == 24u)
	#define `$INSTANCE_NAME`_STEP0_PTR	((reg8 *) `$INSTANCE_NAME`_sD24_DDSdp_u0__D0_REG)
    #define `$INSTANCE_NAME`_STEP1_PTR	((reg8 *) `$INSTANCE_NAME`_sD24_DDSdp_u0__D1_REG)
#elif (`$INSTANCE_NAME`_BUS_WIDTH == 32u)
	#define `$INSTANCE_NAME`_STEP0_PTR	((reg8 *) `$INSTANCE_NAME`_sD32_DDSdp_u0__D0_REG)
    #define `$INSTANCE_NAME`_STEP1_PTR	((reg8 *) `$INSTANCE_NAME`_sD32_DDSdp_u0__D1_REG)
#endif


/***************************************
 *   Function Prototypes
 **************************************/

// todo: void   `$INSTANCE_NAME`_Enable(uint8 val); // 1-enable 0-disable                    

void   `$INSTANCE_NAME`_Start();                     // Init and Enable DDS
void   `$INSTANCE_NAME`_Stop();                      // Stop (disable) DDS
void   `$INSTANCE_NAME`_Init();                      // initialize DDS
void   `$INSTANCE_NAME`_Enable();                    // Enable DDS (must init first) // todo: add _Enabled and _Initialized property
uint8  `$INSTANCE_NAME`_SetFrequency(uint8_t chan, double Freq ); // Set desired output frequency
double `$INSTANCE_NAME`_GetFrequency(uint8_t chan);              // return current set frequency 
double `$INSTANCE_NAME`_GetOutpFreq(uint8_t chan);               // read actual output frequency
uint8  `$INSTANCE_NAME`_GetCREnable();               // Software Enable status
uint32 `$INSTANCE_NAME`_CalcStep( double Freq );     // Calculate TuneWord from frequency
void `$INSTANCE_NAME`_Enable_ch(uint8_t chan);
void `$INSTANCE_NAME`_Disable_ch(uint8_t chan);




//uint8  `$INSTANCE_NAME`_SetAcc( uint32 value );      // Set DDS accumulator value
//uint32 `$INSTANCE_NAME`_GetAcc();                    // Get DDS accumulator value 

//todo: SetTuneWord()

#endif
//[] END OF FILE
