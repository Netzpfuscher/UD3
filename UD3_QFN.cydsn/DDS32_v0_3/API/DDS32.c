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
#include <stdlib.h>
#include "`$INSTANCE_NAME`.h"

// todo: to public?
#if   (`$INSTANCE_NAME`_BUS_WIDTH == 8u)
    #define DDS_RESOLUTION (double) 256.0 // 256 = 2^8
    #define TUNE_WORD_MAX  (uint32) 127u  // 127 = 2^8 - 1 = DDS_RESOLUTION/2 - 1
#elif (`$INSTANCE_NAME`_BUS_WIDTH == 16u)
    #define DDS_RESOLUTION (double) 65536.0 // 65536 = 2^16
    #define TUNE_WORD_MAX  (uint32) 32767u  // 32767 = 2^16 - 1 = DDS_RESOLUTION/2 - 1
#elif (`$INSTANCE_NAME`_BUS_WIDTH == 24u)
    #define DDS_RESOLUTION (double) 16777216.0      // 16777216 = 2^24
    #define TUNE_WORD_MAX  (uint32)  8388607u       //  8388607 = 2^23 - 1 = DDS_RESOLUTION/2 - 1
#elif (`$INSTANCE_NAME`_BUS_WIDTH == 32u)
    #define DDS_RESOLUTION (double) 4294967296.0    // 4294967296 = 2^32 
    #define TUNE_WORD_MAX  (uint32) 2147483647u     //  2147483647 = 2^23 - 1 = DDS_RESOLUTION/2 - 1
#endif


// ststic must present for multiple instances of component

static double Tdiv;		    // time period per bit increment
uint32 `$INSTANCE_NAME`_tune_word[2];       // DDS tune word


//==============================================================================
// Initialize component
// Set input clock frequency and default values for output frequency and phase 
//==============================================================================

void `$INSTANCE_NAME`_Init()
{   
    // make record of the input clock frequency into period   
    Tdiv = (double) (DDS_RESOLUTION / `$INSTANCE_NAME`_CLOCK_FREQ *2);	
    
    //Initialized = true;

    `$INSTANCE_NAME`_SetFrequency(0, `$INSTANCE_NAME`_PresetFreq0 ); 
    `$INSTANCE_NAME`_SetFrequency(1, `$INSTANCE_NAME`_PresetFreq1 ); 
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

void `$INSTANCE_NAME`_Disable_ch(uint8_t chan)
{   
    if(chan>1) return;
    chan++;
    uint8_t val = CY_GET_REG8( `$INSTANCE_NAME`_sCTRLReg_ctrlreg__CONTROL_REG);
    val &= ~(1UL<<chan);
    #if `$INSTANCE_NAME`_CR_ENABLE
        CY_SET_REG8( `$INSTANCE_NAME`_sCTRLReg_ctrlreg__CONTROL_REG, val ); //DDS software stop
    #endif                    
}

void `$INSTANCE_NAME`_Enable_ch(uint8_t chan)
{   
    if(chan>1) return;
    chan++;
    uint8_t val = CY_GET_REG8( `$INSTANCE_NAME`_sCTRLReg_ctrlreg__CONTROL_REG);
    val |= 1UL<<chan;
    #if `$INSTANCE_NAME`_CR_ENABLE
        CY_SET_REG8( `$INSTANCE_NAME`_sCTRLReg_ctrlreg__CONTROL_REG, val ); //DDS software stop
    #endif                    
}


//==============================================================================
// Set output frequency todo: 8, 16, 24-bit?
//==============================================================================

uint8 `$INSTANCE_NAME`_SetFrequency(uint8_t chan, double freq )
{ 
    if(chan>1) return 0;
    uint8 result = 0; // success = 1, fail = 0 
    
        
    // todo: uint64 for possible overflow?    
    uint32 tmp = (uint32) (freq * Tdiv + 0.5);           // calculate tune word
    if ( (tmp < 1) || (tmp > TUNE_WORD_MAX) )  return 0; // fail -> exit if outside of the valid raange // todo: allow exact 0?
          
    tune_word[chan] = tmp;
    
    //todo: tune_word not used anywhere
    switch(chan){
        case 0:
            `$INSTANCE_NAME`_WriteStep0(tune_word[chan]);
        break;
        case 1:
            `$INSTANCE_NAME`_WriteStep1(tune_word[chan]);
        break;
    }    
         
    result = 1;     // success

    return result;
}

uint32 `$INSTANCE_NAME`_SetFrequency_FP8(uint8_t chan, uint32 freq )
{ 
    if(chan>1) return 0;
        
    // todo: uint64 for possible overflow?    
    uint32_t tmp = (((uint64_t)freq * (1<<16))/ (`$INSTANCE_NAME`_CLOCK_FREQ /2) );           // calculate tune word

    if ( (tmp < 1) || (tmp > TUNE_WORD_MAX) )  return 0; // fail -> exit if outside of the valid raange // todo: allow exact 0?
          
    tune_word[chan] = tmp;
    
    //todo: tune_word not used anywhere
    switch(chan){
        case 0:
            `$INSTANCE_NAME`_WriteStep0(tune_word[chan]);
        break;
        case 1:
            `$INSTANCE_NAME`_WriteStep1(tune_word[chan]);
        break;
    }    
         
    return freq>>8;

}

void `$INSTANCE_NAME`_Rand(uint8_t chan){
    switch(chan){
        case 0:
            `$INSTANCE_NAME`_WriteRand0(rand());
        break;
        case 1:
            `$INSTANCE_NAME`_WriteRand1(rand());
        break;
    }
    
}


//==============================================================================
// Calculate TuneWord from output frequency
//==============================================================================
uint32 `$INSTANCE_NAME`_CalcStep( double freq )
{ 
    uint32 tmp = (uint32) (freq * Tdiv + 0.5);           // calculate tune word
    return tmp;
}

//==============================================================================
//     Helper functions->
//==============================================================================

//==============================================================================
//     Return actual output frequency
//==============================================================================
double `$INSTANCE_NAME`_GetOutpFreq(uint8_t chan)  
{	  
   
    // todo: use Tdiv
    switch(chan){
        case 0:
            return ((((double) `$INSTANCE_NAME`_ReadStep0()) / DDS_RESOLUTION ) * `$INSTANCE_NAME`_CLOCK_FREQ);   
        break;
        case 1:   
            return ((((double) `$INSTANCE_NAME`_ReadStep1()) / DDS_RESOLUTION ) * `$INSTANCE_NAME`_CLOCK_FREQ);   
        break;            
    }
    return 0.0;
 }



//==============================================================================
//      
//==============================================================================
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

