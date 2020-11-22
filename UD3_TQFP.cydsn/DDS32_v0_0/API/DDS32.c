/* ============================================================================
 * File Name: `$INSTANCE_NAME`.c
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
#include "`$INSTANCE_NAME`.h"


#if (`$INSTANCE_NAME`_BUS_WIDTH == 8u)
    #define DDS_RESOLUTION (double) 256.0 // 256 = 2^8
    #define TUNE_WORD_MAX  (uint32) 127u  // 127 = 2^8 - 1 = DDS_RESOLUTION/2 - 1
#endif
#if (`$INSTANCE_NAME`_BUS_WIDTH == 16u)
    #define DDS_RESOLUTION (double) 65536.0 // 65536 = 2^16
    #define TUNE_WORD_MAX  (uint32) 32767u  // 32767 = 2^16 - 1 = DDS_RESOLUTION/2 - 1
#endif
#if (`$INSTANCE_NAME`_BUS_WIDTH == 24u)
    #define DDS_RESOLUTION (double) 16777216.0      // 16777216 = 2^24
    #define TUNE_WORD_MAX  (uint32)  8388607u       //  8388607 = 2^23 - 1 = DDS_RESOLUTION/2 - 1
#endif
#if (`$INSTANCE_NAME`_BUS_WIDTH == 32u)
    #define DDS_RESOLUTION (double) 4294967296.0    // 4294967296 = 2^32 
    #define TUNE_WORD_MAX  (uint32) 2147483647u     //  2147483647 = 2^23 - 1 = DDS_RESOLUTION/2 - 1
#endif



static double Tdiv;		    // time period per bit increment
static uint32 tune_word;       // DDS tune word
static double SetFreq;         // DDS set frequency 
//static uint8  PhaseShift;      // out2 phase shift //todo: if <0?




#if (`$INSTANCE_NAME`_BUS_WIDTH == 8u)
void `$INSTANCE_NAME`_WriteStep(uint8 val) `=ReentrantKeil($INSTANCE_NAME . "_WriteStep")`
{               
	//`$INSTANCE_NAME`_STEP_REG = val; 
    CY_SET_REG8(`$INSTANCE_NAME`_STEP_PTR , val); 
}
#endif
#if (`$INSTANCE_NAME`_BUS_WIDTH == 16u)
void `$INSTANCE_NAME`_WriteStep(uint16 val) `=ReentrantKeil($INSTANCE_NAME . "_WriteStep")`
{               
	//`$INSTANCE_NAME`_STEP_REG = val;
	CY_SET_REG16(`$INSTANCE_NAME`_STEP_PTR , val);
}
#endif
#if (`$INSTANCE_NAME`_BUS_WIDTH == 24u)
void `$INSTANCE_NAME`_WriteStep(uint32 val) `=ReentrantKeil($INSTANCE_NAME . "_WriteStep")`
{               
	//`$INSTANCE_NAME`_STEP_REG = val;
	CY_SET_REG24(`$INSTANCE_NAME`_STEP_PTR , val);
}
#endif
#if (`$INSTANCE_NAME`_BUS_WIDTH == 32u)
void `$INSTANCE_NAME`_WriteStep(uint32 val) `=ReentrantKeil($INSTANCE_NAME . "_WriteStep")`
{               
	//`$INSTANCE_NAME`_STEP_REG = val;
	CY_SET_REG32(`$INSTANCE_NAME`_STEP_PTR , val);
}
#endif



//==============================================================================
// Initialize component
// Set input clock frequency and default values for output frequency and phase 
//==============================================================================

void `$INSTANCE_NAME`_Init()
{   
    // make record of the input clock frequency into period   
    Tdiv = (double) (DDS_RESOLUTION / `$INSTANCE_NAME`_CLOCK_FREQ);	
    
    //Initialized = true;

    `$INSTANCE_NAME`_SetFrequency( `$INSTANCE_NAME`_PresetFreq );  
}

//==============================================================================
// Enable component
// Have effect only in Software_only and Software_and_Hardware modes 
//==============================================================================

void `$INSTANCE_NAME`_Enable()
{   
    //todo: must be initialized first
    #if `$INSTANCE_NAME`_CR_ENABLE
        CY_SET_REG8( `$INSTANCE_NAME`_sCTRLReg_ctrlreg__CONTROL_REG, 1 ); //software enable
    #endif                    
}

//==============================================================================
// Start component
// Initialize and enable DDS component
//==============================================================================

void `$INSTANCE_NAME`_Start()
{   
    `$INSTANCE_NAME`_Init();    
    `$INSTANCE_NAME`_Enable();
}

//==============================================================================
// Stop component
// Have effect only in Software_only and Software_and_Hardware modes 
//==============================================================================

void `$INSTANCE_NAME`_Stop()
{   
    #if `$INSTANCE_NAME`_CR_ENABLE
        CY_SET_REG8( `$INSTANCE_NAME`_sCTRLReg_ctrlreg__CONTROL_REG, 0 ); //DDS software stop
    #endif                    
}

//==============================================================================
// Set output frequency
//==============================================================================

uint8 `$INSTANCE_NAME`_SetFrequency( double freq )
{ 
    uint8 result = 0; // success = 1, fail = 0 
    
        
        
    uint32 tmp = (uint32) (freq * Tdiv + 0.5);           // calculate tune word
    if ( (tmp < 1) || (tmp > TUNE_WORD_MAX) )  return 0; // fail -> exit if outside of the valid raange 
          
    tune_word = tmp;
    
    
    `$INSTANCE_NAME`_WriteStep(tune_word);    
        
        
    SetFreq = freq; // backup value
    result = 1;     // success
        
    
    return result;
}



//==============================================================================
//     Helper functions
//==============================================================================

double `$INSTANCE_NAME`_GetOutpFreq() //return actual output frequency 
{	  
    double OutputFreq  = (((double) tune_word) / DDS_RESOLUTION ) * `$INSTANCE_NAME`_CLOCK_FREQ;   
    return OutputFreq;
}

double `$INSTANCE_NAME`_GetFrequency() // return current set frequency 
{	  
    return SetFreq;
}

uint8 `$INSTANCE_NAME`_GetCREnable()
{   
    //todo: must be initialized first
    #if `$INSTANCE_NAME`_CR_ENABLE
        return CY_GET_REG8( `$INSTANCE_NAME`_sCTRLReg_ctrlreg__CONTROL_REG); //software enable
    #else
        return 0;
    #endif
}

//==============================================================================
/* [] END OF FILE */

