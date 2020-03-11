/* =========================================================
 *
 * Strip Light Controller
 * By Mark Hastings
 *
 * 10/28/2015 MEH Added support for SF2812RGBW
 *                Allow for a more flexible bus clock selection
 * 12/12/2015 MEH Fixed a bug that may cause the interface
 *                to hang when many other ISRs are running.
 *
 * This file contains the functions required to control each
 * lighting control channel.
 *
 * =========================================================
*/

#include <project.h>
#include "cytypes.h"
#include "stdlib.h"
#include "cyfitter.h"
#include "`$INSTANCE_NAME`.h"
#include "`$INSTANCE_NAME`_fonts.h"


void   (*`$INSTANCE_NAME`_EnterCritical)() = (funcPtr)0;
void   (*`$INSTANCE_NAME`_ExitCritical)() = (funcPtr)0;


uint8  `$INSTANCE_NAME`_initvar = 0;

#if(`$INSTANCE_NAME`_MEMORY_TYPE == `$INSTANCE_NAME`_MEMORY_RGB)
uint32  `$INSTANCE_NAME`_ledArray[`$INSTANCE_NAME`_ARRAY_ROWS][`$INSTANCE_NAME`_ARRAY_COLS];
#else
uint8   `$INSTANCE_NAME`_ledArray[`$INSTANCE_NAME`_ARRAY_ROWS][`$INSTANCE_NAME`_ARRAY_COLS];
#endif

uint32  `$INSTANCE_NAME`_ledIndex = 0;  
uint32  `$INSTANCE_NAME`_row = 0;
uint32  `$INSTANCE_NAME`_refreshComplete;

uint32  `$INSTANCE_NAME`_DimMask;
uint32  `$INSTANCE_NAME`_DimShift;

#if( `$INSTANCE_NAME`_GAMMA_CORRECTION == `$INSTANCE_NAME`_GAMMA_ON )
const uint8 `$INSTANCE_NAME`_gamma[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

#endif

#if (`$INSTANCE_NAME`_LED_LAYOUT == `$INSTANCE_NAME`_LED_LAYOUT_GRID16X16)
 uint8 const `$INSTANCE_NAME`_cTrans[16][16] =  {
	/*  0  */  {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15},
    /*  1  */  { 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16},
	/*  2  */  { 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47},
	/*  3  */  { 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48}, 
	/*  4  */  { 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79},
	/*  5  */  { 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80},
	/*  6  */  { 96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111},
	/*  7  */  {127,126,125,124,123,122,121,120,119,118,117,116,115,114,113,112},
	/*  8  */  {128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143},
	/*  9  */  {159,158,157,156,155,154,153,152,151,150,149,148,147,146,145,144},
	/* 10  */  {160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175},
	/* 11  */  {191,190,189,188,187,186,185,184,183,182,181,180,179,178,177,176},
	/* 12  */  {192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207},
	/* 13  */  {223,222,221,220,219,218,217,216,215,214,213,212,211,210,209,208},
	/* 14  */  {224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239},
	/* 15  */  {255,254,253,252,251,250,249,248,247,246,245,244,243,242,241,240}};   
#endif

#if (`$INSTANCE_NAME`_LED_LAYOUT == `$INSTANCE_NAME`_LED_LAYOUT_GRID8X8)
 uint8 const `$INSTANCE_NAME`_cTrans[8][8] =  {
	/*  0  */  {  0,   1,  2,  3,  4,  5,  6,   7},
    /*  1  */  {  8,   9, 10, 11, 12, 13, 14,  15},
    /*  2  */  {  16, 17, 18, 19, 20, 21, 22,  23},
    /*  3  */  {  24, 25, 26, 27, 28, 29, 30,  31},
    /*  4  */  {  32, 33, 34, 35, 36, 37, 38,  39},
    /*  5  */  {  40, 41, 42, 43, 44, 45, 46,  47},
    /*  6  */  {  48, 49, 50, 51, 52, 53, 54,  55},
    /*  7  */  {  56, 57, 58, 59, 60, 64, 62,  63}};    
#endif

#if((`$INSTANCE_NAME`_CHIP == `$INSTANCE_NAME`_CHIP_WS2812) || (`$INSTANCE_NAME`_CHIP == `$INSTANCE_NAME`_CHIP_SF2812RGBW))
const uint32 `$INSTANCE_NAME`_CLUT[ ] = {
//xxBBRRGG (WS2812)
//     (24)   [ Index = 0 ]
0x0000FFFF,  // 0 Yellow
0x0000CCFF,
0x000099FF,
0x000033FF,
0x000000FF,  // 5  Green
0x006600B3,
0x00990099, 
0x00B30066, 
0x00CC0033,  // 9 Blue
0x00B31919, 
0x00993300, 
0x00994000, 
0x00996600, 
0x00999900, 
0x0099CC00, 
0x0066E600, 
0x0000FF00, // Red
0x0000FF33, 
0x0000FF66, 
0x0000FF80, 
0x0000FF99,  // 20 Orange
0x0000FFB2, 
0x0000FFCC, 
0x0000FFE5,
// #24

//xxBBRRGG  (64)  [ Index = 24 ]
0x00000000, 0x00550000, 0x00AA0000, 0x00FF0000,  // 0, Black,  LtBlue, MBlue, Blue
0x00000055, 0x00550055, 0x00AA0055, 0x00FF0055,  // 4, LtGreen
0x000000AA, 0x005500AA, 0x00AA00AA, 0x00FF00AA,  // 8, MGreen
0x000000FF, 0x005500FF, 0x00AA00FF, 0x00FF00FF,  // 12 Green

0x00005500, 0x00555500, 0x00AA5500, 0x00FF5500,  // 16, LtRed
0x00005555, 0x00555555, 0x00AA5555, 0x00FF5555,  // 20, LtYellow
0x000055AA, 0x005555AA, 0x00AA55AA, 0x00FF55AA,  // 24, 
0x000055FF, 0x005555FF, 0x00AA55FF, 0x00FF55FF,  // 28,

0x0000AA00, 0x0055AA00, 0x00AAAA00, 0x00FFAA00,  // 32, MRed
0x0000AA55, 0x0055AA55, 0x00AAAA55, 0x00FFAA55,  // 36, 
0x0000AAAA, 0x0055AAAA, 0x00AAAAAA, 0x00FFAAAA,  // 55, 
0x0000AAFF, 0x0055AAFF, 0x00AAAAFF, 0x00FFAAFF,  // 44, 

0x0000FF00, 0x0055FF00, 0x00AAFF00, 0x00FFFF00,  // 48, Red, ??, ??, Magenta
0x0000FF55, 0x0055FF55, 0x00AAFF55, 0x00FFFF55,  // 52,
0x0000FFAA, 0x0055FFAA, 0x00AAFFAA, 0x00FFFFAA,  // 56,
0x0000FFFF, 0x0055FFFF, 0x00AAFFFF, 0x00FFFFFF,  // 60, Yellow,??, ??, White

// Misc ( 16 )  [ Index = 88 ]
0x000080FF,  // SPRING_GREEN
0x008000FF,  // TURQUOSE
0x00FF00FF,  // CYAN
0x00FF0080,  // OCEAN
0x00FF8000,  // VIOLET
0x0080FF00,  // RASPBERRY
0x000000FF,  // GREEN
0x00202020,  // DIM WHITE
0x00200000,  // DIM BLUE
0x10000000,  // INVISIBLE
0x0000FF00,  // Fire_Dark
0x0000FF30,  // Fire_Light  // 00FF80
0xFFFFFFFF,  // Full White
0xFF000000,  // LED_WHITE
0xFF808080,  // WHITE_RGB50
0xFF404040,  // WHITE_RGB25

// Temperture Color Blue to Red (16) [ Index = 104 ]
0x00FF0000, 0x00F01000, 0x00E02000, 0x00D03000,
0x00C04000, 0x00B05000, 0x00A06000, 0x00907000,
0x00808000, 0x00709000, 0x0060A000, 0x0050B000,
0x0040C000, 0x0030D000, 0x0020E000, 0x0000FF00
};
#else  //xxBBGGRR (WS2811)
const uint32 `$INSTANCE_NAME`_CLUT[ ] = {
//     (24)   [ Index = 0 ]
0x0000FFFF,  // 0 Yellow
0x0000FFCC,
0x0000FF99,
0x0000FF33,
0x0000FF00,  // 5  Green
0x0066B300,
0x00999900, 
0x00B36600, 
0x00CC3300,  // 9 Blue
0x00B31919, 
0x00990033, 
0x00990040, 
0x00990066, 
0x00990099, 
0x009900CC, 
0x006600E6, 
0x000000FF, 
0x000033FF, 
0x000066FF, 
0x000080FF, 
0x000080FF,  // 20 Orange
0x0000B2FF, 
0x0000CCFF, 
0x0000E5FF,
// #24

//xxBBGGRR  (64)  [ Index = 24 ]
0x00000000, 0x00550000, 0x00AA0000, 0x00FF0000,  // 0, Black,  LtBlue, MBlue, Blue
0x00005500, 0x00555500, 0x00AA5500, 0x00FF5500,  // 4, LtGreen
0x0000AA00, 0x0055AA00, 0x00AAAA00, 0x00FFAA00,  // 8, MGreen
0x0000FF00, 0x0055FF00, 0x00AAFF00, 0x00FFFF00,  // 12 Green

0x00000055, 0x00550055, 0x00AA0055, 0x00FF0055,  // 16, LtRed
0x00005555, 0x00555555, 0x00AA5555, 0x00FF5555,  // 20, LtYellow
0x0000AA55, 0x0055AA55, 0x00AAAA55, 0x00FFAA55,  // 24, 
0x0000FF55, 0x0055FF55, 0x00AAFF55, 0x00FFFF55,  // 28,

0x000000AA, 0x005500AA, 0x00AA00AA, 0x00FF00AA,  // 32, MRed
0x000055AA, 0x005555AA, 0x00AA55AA, 0x00FF55AA,  // 36, 
0x0000AAAA, 0x0055AAAA, 0x00AAAAAA, 0x00FFAAAA,  // 55, 
0x0000FFAA, 0x0055FFAA, 0x00AAFFAA, 0x00FFFFAA,  // 44, 

0x000000FF, 0x005500FF, 0x00AA00FF, 0x00F00FFF,  // 48, Red, ??, ??, Magenta
0x000055FF, 0x005555FF, 0x00AA55FF, 0x00FF55FF,  // 52,
0x0000AAFF, 0x0055AAFF, 0x00AAAAFF, 0x00FFAAFF,  // 56,
0x0000FFFF, 0x0055FFFF, 0x00AAFFFF, 0x00FFFFFF,  // 60, Yellow,??, ??, White

// Misc ( 16 )  [ Index = 88 ]   //xxBBGGRR
0x0000FF80,  // SPRING_GREEN
0x0080FF00,  // TURQUOSE
0x00FFFF00,  // CYAN
0x00FF8000,  // OCEAN
0x00FF0080,  // VIOLET
0x008000FF,  // RASPBERRY
0x0000FF00,  // GREEN
0x00202020,  // DIM WHITE
0x00200000,  // DIM BLUE
0x10000000,  // INVISIBLE
0x000000FF,  // Fire_Dark
0x000030FF,  // Fire_Light  // 0080FF
0xFFFFFFFF,  // Full White
0xFF000000,  // White_LED
0xFF808080,  // WHITE_RGB50
0xFF404040,  // WHITE_RGB25

// Temperture Color Blue to Red (16) [ Index = 104 ]
0x00FF0000, 0x00F00010, 0x00E00020, 0x00D00030,
0x00C00040, 0x00B00050, 0x00A00060, 0x00900070,
0x00800080, 0x00700090, 0x006000A0, 0x005000B0,
0x004000C0, 0x003000D0, 0x002000E0, 0x000000FF
};
#endif



/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_Start
********************************************************************************
* Summary:
*  Initialize the hardware. 
*
* Parameters:  
*  void  
*
* Return: 
*  void
*
*******************************************************************************/
void `$INSTANCE_NAME`_Start()
{
    extern uint8 `$INSTANCE_NAME`_initvar;
    extern uint32 `$INSTANCE_NAME`_refreshComplete;

    #if (CY_PSOC3 || CY_PSOC5LP)
        int32   `$INSTANCE_NAME`_realClock; 
        int16   `$INSTANCE_NAME`_period;
        int16   `$INSTANCE_NAME`_dataZero;
        int16   `$INSTANCE_NAME`_dataOne;
    #endif /* CY_PSOC5A */

    `$INSTANCE_NAME`_ACTL0_REG = `$INSTANCE_NAME`_DISABLE_FIFO;

    #if (CY_PSOC3 || CY_PSOC5LP)
        `$INSTANCE_NAME`_realClock = (int32)`$INSTANCE_NAME`_INPUT_CLOCK / (int32)(`$INSTANCE_NAME`_Clock_DIV_LSB + 1);
        `$INSTANCE_NAME`_period    = (int16)(`$INSTANCE_NAME`_realClock/`$INSTANCE_NAME`_WS281X_CLOCK );
        
        `$INSTANCE_NAME`_dataZero  = ((`$INSTANCE_NAME`_period * `$INSTANCE_NAME`_TIME_T0H)/`$INSTANCE_NAME`_TIME_TL_TH);
        `$INSTANCE_NAME`_dataOne   = ((`$INSTANCE_NAME`_period * `$INSTANCE_NAME`_TIME_T1H)/`$INSTANCE_NAME`_TIME_TL_TH);
        
        `$INSTANCE_NAME`_Period    = (uint8)(`$INSTANCE_NAME`_period - 1);
        `$INSTANCE_NAME`_Compare0  = (uint8)(`$INSTANCE_NAME`_period - `$INSTANCE_NAME`_dataZero);
        `$INSTANCE_NAME`_Compare1  = (uint8)(`$INSTANCE_NAME`_period - `$INSTANCE_NAME`_dataOne);

    #elif (CY_PSOC4)
 
        `$INSTANCE_NAME`_Period   = (uint8)`$INSTANCE_NAME`_PERIOD-1;
        `$INSTANCE_NAME`_Compare0 = (uint8)`$INSTANCE_NAME`_COMPARE_ZERO;
        `$INSTANCE_NAME`_Compare1 = (uint8)`$INSTANCE_NAME`_COMPARE_ONE;        
    #endif /* CY_PSOC5A */

    
    `$INSTANCE_NAME`_CONTROL = `$INSTANCE_NAME`_ENABLE;
    `$INSTANCE_NAME`_MemClear(`$INSTANCE_NAME`_OFF);
    `$INSTANCE_NAME`_CONTROL = `$INSTANCE_NAME`_DISABLE;
    
    `$INSTANCE_NAME`_SetFont( `$INSTANCE_NAME`_FONT_5X7);
    
    /* Set no dimming, full brightness */
    `$INSTANCE_NAME`_Dim(0); 

    if(`$INSTANCE_NAME`_initvar == 0)
    {

        `$INSTANCE_NAME`_initvar = 1;
 
         /* Start and set interrupt vector */

#if(`$INSTANCE_NAME`_TRANSFER == `$INSTANCE_NAME`_TRANSFER_ISR)
       {
           `$INSTANCE_NAME`_cisr_StartEx(`$INSTANCE_NAME`_CISR);
		   `$INSTANCE_NAME`_fisr_StartEx(`$INSTANCE_NAME`_FISR);
        
           /* Set the ISRs to the highest priority */
           `$INSTANCE_NAME`_cisr_SetPriority(1u);
           `$INSTANCE_NAME`_fisr_SetPriority(0u);
       }
#endif       
       if(`$INSTANCE_NAME`_TRANSFER == `$INSTANCE_NAME`_TRANSFER_FIRMWARE)
       {
           `$INSTANCE_NAME`_CONTROL = `$INSTANCE_NAME`_ENABLE;    
       }
    }
    `$INSTANCE_NAME`_refreshComplete = 1;
}

/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_SetCriticalFuncs
********************************************************************************
* Summary:
*   Set the enter and exit critical functions
*
* Parameters:  
*  void  
*
* Return: 
*  void
*
*******************************************************************************/
void `$INSTANCE_NAME`_SetCriticalFuncs(funcPtr EnCriticalFunc, funcPtr ExCriticalFunc)
{
    `$INSTANCE_NAME`_EnterCritical = EnCriticalFunc;
    `$INSTANCE_NAME`_ExitCritical  = ExCriticalFunc;   
}

/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_TriggerWaitDelay
********************************************************************************
* Summary:
*   Update the LEDs then wait for completion.
*
* Parameters:  
*  void  
*
* Return: 
*  void
*
*******************************************************************************/
void `$INSTANCE_NAME`_TriggerWaitDelay(uint32 delay)
{

    uint32 toCount = 0;
    
    // Call Critical section function here
    if(`$INSTANCE_NAME`_EnterCritical != (funcPtr)0)
    {
        `$INSTANCE_NAME`_EnterCritical();    
    }
    
    `$INSTANCE_NAME`_Trigger(1);           // Trigger the LED update
    
    while( `$INSTANCE_NAME`_Ready() == 0)
    {
        if(toCount > (`$INSTANCE_NAME`_READY_TIMEOUT  * 10)) break; 
        CyDelayUs(100);  
        toCount++;
    }
    
    while(toCount < (delay*10)) 
    {
        toCount++;
        CyDelayUs(100); 
    }
    
    
    // Call undo critical function here
      if(`$INSTANCE_NAME`_ExitCritical != (funcPtr)0)
    {
        `$INSTANCE_NAME`_ExitCritical();    
    }
    
    
}

/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_TriggerWait
********************************************************************************
* Summary:
*   Update the LEDs then waits for completion.
*
* Parameters:  
  rst: Resets the row and column parameters.    
*
* Return: 
*  void
*
*******************************************************************************/
void `$INSTANCE_NAME`_TriggerWait(uint32 rst)
{

    
    // Call Critical section function here
    if(`$INSTANCE_NAME`_EnterCritical != (funcPtr)0)
    {
        `$INSTANCE_NAME`_EnterCritical();    
    }
    
    `$INSTANCE_NAME`_Trigger(rst);
    
    while( `$INSTANCE_NAME`_Ready() == 0);
    
    
    // Call undo critical function here
      if(`$INSTANCE_NAME`_ExitCritical != (funcPtr)0)
    {
        `$INSTANCE_NAME`_ExitCritical();    
    }
    
    
}


/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_Trigger
********************************************************************************
* Summary:
*   Update the LEDs with graphics memory.
*
* Parameters:  
*  rst: Resets the row and column parameters.  
*
* Return: 
*  void
*
*******************************************************************************/
void `$INSTANCE_NAME`_Trigger(uint32 rst)
{
    uint32 color;
    
    if(rst) 
    {
        `$INSTANCE_NAME`_row = 0;  /* Reset the row */
        `$INSTANCE_NAME`_Channel = 0;
    }
    
    #if(`$INSTANCE_NAME`_MEMORY_TYPE == `$INSTANCE_NAME`_MEMORY_RGB)
       color = `$INSTANCE_NAME`_ledArray[`$INSTANCE_NAME`_row][0];
    #else  /* Else use lookup table */
       color = `$INSTANCE_NAME`_CLUT[ (`$INSTANCE_NAME`_ledArray[`$INSTANCE_NAME`_row][0]) ];
    #endif
    
     color = (color >> `$INSTANCE_NAME`_DimShift) & `$INSTANCE_NAME`_DimMask;
     
    #if(`$INSTANCE_NAME`_GAMMA_CORRECTION == `$INSTANCE_NAME`_GAMMA_ON)
        `$INSTANCE_NAME`_DATA = `$INSTANCE_NAME`_gamma[color & 0x000000FF];  // Write Green
        color = color >> 8;
        `$INSTANCE_NAME`_DATA = `$INSTANCE_NAME`_gamma[color & 0x000000FF];  // Write Red
        color = color >> 8;
        `$INSTANCE_NAME`_DATA = `$INSTANCE_NAME`_gamma[color & 0x000000FF];  // Write Blue
        #if(`$INSTANCE_NAME`_CHIP == `$INSTANCE_NAME`_CHIP_SF2812RGBW)
            color = color >> 8;
            `$INSTANCE_NAME`_DATA = `$INSTANCE_NAME`_gamma[color & 0x000000FF];  // Write White
        #endif
    #else
        `$INSTANCE_NAME`_DATA = (uint8)(color & 0x000000FF);  // Write Green
        color = color >> 8;
        `$INSTANCE_NAME`_DATA = (uint8)(color & 0x000000FF);  // Write Red
        color = color >> 8;
        `$INSTANCE_NAME`_DATA = (uint8)(color & 0x000000FF);  // Write Blue  
        #if(`$INSTANCE_NAME`_CHIP == `$INSTANCE_NAME`_CHIP_SF2812RGBW)
            color = color >> 8;
        `$INSTANCE_NAME`_DATA = (uint8)(color & 0x000000FF);  // Write White 
        #endif
    #endif
  
    `$INSTANCE_NAME`_ledIndex = 1;
	
	`$INSTANCE_NAME`_CONTROL = `$INSTANCE_NAME`_ENABLE | `$INSTANCE_NAME`_XFRCMPT_IRQ_EN | `$INSTANCE_NAME`_FIFO_IRQ_EN; 
    `$INSTANCE_NAME`_refreshComplete = 0;
}

/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_Ready
********************************************************************************
* Summary:
*  Checks to see if transfer is complete.
*
* Parameters:  
*  none  
*
* Return: 
*  Zero if not complete, non-zero if transfer complete.
*
*******************************************************************************/
uint32 `$INSTANCE_NAME`_Ready(void)
{
    if(`$INSTANCE_NAME`_refreshComplete )
    {
        `$INSTANCE_NAME`_CONTROL = `$INSTANCE_NAME`_DISABLE;
        return((uint32)1);
    }
    else
    {
         return((uint32)0);
    }
}


/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_Stop
********************************************************************************
* Summary:
*  Stop all transfers.
*
* Parameters:  
*  None 
*
* Return: 
*  void
*
*******************************************************************************/
void `$INSTANCE_NAME`_Stop()
{
    `$INSTANCE_NAME`_CONTROL = `$INSTANCE_NAME`_DISABLE;
}

uint32 `$INSTANCE_NAME`_getColor( uint32 color)
{
    #if(`$INSTANCE_NAME`_MEMORY_TYPE == `$INSTANCE_NAME`_MEMORY_RGB)
    color = `$INSTANCE_NAME`_CLUT[color];
    #endif

    return(color);
}

/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_ColorInc
********************************************************************************
* Summary:
*  Increment color throught the color lookup table.
*
* Parameters:  
*  uint32 incValue: Increment through color table by incValue. 
*
* Return: Color at next location.
*  
*
*******************************************************************************/
uint32 `$INSTANCE_NAME`_ColorInc(uint32 incValue)
{
    uint32 color;
    extern const uint32 `$INSTANCE_NAME`_CLUT[];
    static uint32 colorIndex = 0;
    
    colorIndex += incValue;
    colorIndex = colorIndex % `$INSTANCE_NAME`_COLOR_WHEEL_SIZE;

    #if(`$INSTANCE_NAME`_MEMORY_TYPE == `$INSTANCE_NAME`_MEMORY_RGB)
       color = `$INSTANCE_NAME`_CLUT[ colorIndex ];
    #else
        color = colorIndex;
    #endif
    return(color);
}
/*****************************************************************************
* Function Name: `$INSTANCE_NAME`_FISR
******************************************************************************
*
* Summary:
*  Interrupt service handler for data transfer for each LED 
*
* Parameters:  
*  void
*
* Return: 
*  void 
*
* Reentrant: 
*  No
*
*****************************************************************************/
CY_ISR( `$INSTANCE_NAME`_FISR)
{
    extern uint32  `$INSTANCE_NAME`_DimMask;
    extern uint32  `$INSTANCE_NAME`_DimShift;
    uint32 static color;
   
    #if(`$INSTANCE_NAME`_MEMORY_TYPE == `$INSTANCE_NAME`_MEMORY_RGB)
        color = `$INSTANCE_NAME`_ledArray[`$INSTANCE_NAME`_row][`$INSTANCE_NAME`_ledIndex];
    #else  /* Else use lookup table */
        color = `$INSTANCE_NAME`_CLUT[ (`$INSTANCE_NAME`_ledArray[`$INSTANCE_NAME`_row][`$INSTANCE_NAME`_ledIndex]) ];
    #endif

    color = (color >> `$INSTANCE_NAME`_DimShift) & `$INSTANCE_NAME`_DimMask;  

    #if(`$INSTANCE_NAME`_GAMMA_CORRECTION == `$INSTANCE_NAME`_GAMMA_ON)

        `$INSTANCE_NAME`_DATA = `$INSTANCE_NAME`_gamma[color & 0x000000FF];  // Write Green
        color = color >> 8;
        `$INSTANCE_NAME`_DATA = `$INSTANCE_NAME`_gamma[color & 0x000000FF];  // Write Red
        color = color >> 8;
        `$INSTANCE_NAME`_DATA = `$INSTANCE_NAME`_gamma[color & 0x000000FF];  // Write Blue
        #if(`$INSTANCE_NAME`_CHIP == `$INSTANCE_NAME`_CHIP_SF2812RGBW)
            color = color >> 8;
            `$INSTANCE_NAME`_DATA = `$INSTANCE_NAME`_gamma[color & 0x000000FF];  // Write White 
        #endif
    #else

        `$INSTANCE_NAME`_DATA = (uint8)(color & 0x000000FF);  // Write Green
        color = color >> 8;
        `$INSTANCE_NAME`_DATA = (uint8)(color & 0x000000FF);  // Write Red
        color = color >> 8;
        `$INSTANCE_NAME`_DATA = (uint8)(color & 0x000000FF);  // Write Blue    
         #if(`$INSTANCE_NAME`_CHIP == `$INSTANCE_NAME`_CHIP_SF2812RGBW)
            color = color >> 8;
            `$INSTANCE_NAME`_DATA = (uint8)(color & 0x000000FF);  // Write White 
        #endif 
    #endif
    
    `$INSTANCE_NAME`_ledIndex++;
    if(`$INSTANCE_NAME`_ledIndex >= `$INSTANCE_NAME`_ARRAY_COLS)
    {
        `$INSTANCE_NAME`_CONTROL = `$INSTANCE_NAME`_ENABLE | `$INSTANCE_NAME`_XFRCMPT_IRQ_EN; 
    }
}

/*****************************************************************************
* Function Name: `$INSTANCE_NAME`_CISR
******************************************************************************
*
* Summary:
*  Interrupt service handler after each row is complete.
*
* Parameters:  
*  void
*
* Return: 
*  void 
*
* Reentrant: 
*  No
*
*****************************************************************************/
CY_ISR( `$INSTANCE_NAME`_CISR)
{
    extern uint32  `$INSTANCE_NAME`_DimMask;
    extern uint32  `$INSTANCE_NAME`_DimShift;
    uint32 static color;
    extern uint32 `$INSTANCE_NAME`_refreshComplete;

	`$INSTANCE_NAME`_CONTROL = `$INSTANCE_NAME`_ENABLE |`$INSTANCE_NAME`_NEXT_ROW;
    `$INSTANCE_NAME`_row++;
    if( `$INSTANCE_NAME`_row < `$INSTANCE_NAME`_ARRAY_ROWS)  /* More Rows to do  */
    {
        `$INSTANCE_NAME`_Channel = `$INSTANCE_NAME`_row;  

		#if(`$INSTANCE_NAME`_MEMORY_TYPE == `$INSTANCE_NAME`_MEMORY_RGB)
             color = `$INSTANCE_NAME`_ledArray[`$INSTANCE_NAME`_row][0];
        #else  /* Else use lookup table */
             color = `$INSTANCE_NAME`_CLUT[ (`$INSTANCE_NAME`_ledArray[`$INSTANCE_NAME`_row][0]) ];
        #endif

        color = (color >> `$INSTANCE_NAME`_DimShift) & `$INSTANCE_NAME`_DimMask;
 
        #if(`$INSTANCE_NAME`_GAMMA_CORRECTION == `$INSTANCE_NAME`_GAMMA_ON)
            `$INSTANCE_NAME`_DATA = `$INSTANCE_NAME`_gamma[color & 0x000000FF];  // Write Green
            color = color >> 8;
            `$INSTANCE_NAME`_DATA = `$INSTANCE_NAME`_gamma[color & 0x000000FF];  // Write Red
            color = color >> 8;
            `$INSTANCE_NAME`_DATA = `$INSTANCE_NAME`_gamma[color & 0x000000FF];  // Write Blue
             #if(`$INSTANCE_NAME`_CHIP == `$INSTANCE_NAME`_CHIP_SF2812RGBW)
                color = color >> 8;
                `$INSTANCE_NAME`_DATA = `$INSTANCE_NAME`_gamma[color & 0x000000FF];  // Write White 
            #endif 
        #else
            `$INSTANCE_NAME`_DATA = (uint8)(color & 0x000000FF);  // Write Green
            color = color >> 8;
            `$INSTANCE_NAME`_DATA = (uint8)(color & 0x000000FF);  // Write Red
            color = color >> 8;
            `$INSTANCE_NAME`_DATA = (uint8)(color & 0x000000FF);  // Write Blue  
            #if(`$INSTANCE_NAME`_CHIP == `$INSTANCE_NAME`_CHIP_SF2812RGBW)
                color = color >> 8;
                `$INSTANCE_NAME`_DATA = (uint8)(color & 0x000000FF);  // Write White 
            #endif  
        #endif
       
        `$INSTANCE_NAME`_ledIndex = 1;
		`$INSTANCE_NAME`_CONTROL = `$INSTANCE_NAME`_ENABLE | `$INSTANCE_NAME`_XFRCMPT_IRQ_EN | `$INSTANCE_NAME`_FIFO_IRQ_EN; 
    }
    else
    {
        `$INSTANCE_NAME`_refreshComplete = 1u;
    }
}


/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_DisplayClear
********************************************************************************
* Summary:
*   Clear memory with a given value and update the display.
*
* Parameters:  
*  uint32 color: Color to clear display. 
*
* Return: 
*  void
*
*******************************************************************************/
void `$INSTANCE_NAME`_DisplayClear(uint32 color)
{   
    `$INSTANCE_NAME`_MemClear(color);
    `$INSTANCE_NAME`_Trigger(1);
}

/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_MemClear
********************************************************************************
* Summary:
*   Clear LED memory with given color, but do not update display.
*
* Parameters:  
*  uint32 color: Color to clear display.  
*
* Return: 
*  void
*
*******************************************************************************/
void `$INSTANCE_NAME`_MemClear(uint32 color)
{
    uint32  row, col;
    
    for(row=0; row < `$INSTANCE_NAME`_ARRAY_ROWS; row++)
    {
        for(col=0; col < `$INSTANCE_NAME`_ARRAY_COLS; col++)
        {
            #if(`$INSTANCE_NAME`_MEMORY_TYPE == `$INSTANCE_NAME`_MEMORY_RGB)
                `$INSTANCE_NAME`_ledArray[row][col] = color;
            #else  /* Else use lookup table */
                 `$INSTANCE_NAME`_ledArray[row][col] = (uint8)color;
            #endif
        }
    }
}



/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_WriteColor
********************************************************************************
* Summary:
*   Write given color directly to output register.
*
* Parameters:  
*  uint32 color: Color to write to display. 
*
* Return: 
*  void
*
*******************************************************************************/
void `$INSTANCE_NAME`_WriteColor(uint32 color)
{
    while( (`$INSTANCE_NAME`_STATUS & `$INSTANCE_NAME`_FIFO_EMPTY) == 0); 

    `$INSTANCE_NAME`_DATA = (uint8)(color & 0x000000FF);  // Write Green
    color = color >> 8;
    `$INSTANCE_NAME`_DATA = (uint8)(color & 0x000000FF);  // Write Red
    color = color >> 8;
    `$INSTANCE_NAME`_DATA = (uint8)(color & 0x000000FF);  // Write Blue
    
    #if(`$INSTANCE_NAME`_CHIP == `$INSTANCE_NAME`_CHIP_SF2812RGBW)
        color = color >> 8;
        `$INSTANCE_NAME`_DATA = (uint8)(color & 0x000000FF);  // Write White 
    #endif 
}


/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_Pixel
********************************************************************************
*
* Summary:
*  Draw Pixel  
*
* Parameters:  
*  x,y:    Location to draw the pixel
*  color:  Color of the pixel
*
* Return: 
*  None 
*******************************************************************************/
void `$INSTANCE_NAME`_Pixel(int32 x, int32 y, uint32 color)
{

    // Swap X-Y Coordinates
    #if(`$INSTANCE_NAME`_SWAP_XY_COORD == `$INSTANCE_NAME`_XY_SWAPED)
        uint32 xyTmp;
        xyTmp = x;
        x = y;
        y = xyTmp;
    #endif
    
    // X-Wrap
    #if (`$INSTANCE_NAME`_COORD_WRAP & `$INSTANCE_NAME`_COORD_XAXIS )
        x = x % (`$INSTANCE_NAME`_MAX_X+1);
    #endif
    
    // Y-Wrap
    #if (`$INSTANCE_NAME`_COORD_WRAP & `$INSTANCE_NAME`_COORD_YAXIS )
        y = y % (`$INSTANCE_NAME`_MAX_Y+1);
    #endif
    
    // Flip X-Axis
    #if (`$INSTANCE_NAME`_FLIP_X_COORD == `$INSTANCE_NAME`_FLIP_COORD )
        x = `$INSTANCE_NAME`_MAX_X - x;
    #endif
    
    // Flip Y-Axis
    #if (`$INSTANCE_NAME`_FLIP_Y_COORD == `$INSTANCE_NAME`_FLIP_COORD )
        y = `$INSTANCE_NAME`_MAX_Y - y;
    #endif
    
    // Make sure X-Y values are in range
   	if((x >= `$INSTANCE_NAME`_MIN_X) && (y >= `$INSTANCE_NAME`_MIN_Y) && (x <= `$INSTANCE_NAME`_MAX_X) && (y <= `$INSTANCE_NAME`_MAX_Y))
    {
        
        #if (`$INSTANCE_NAME`_LED_LAYOUT == `$INSTANCE_NAME`_LED_LAYOUT_STANDARD)
             // Do nothing special   
        #elif (`$INSTANCE_NAME`_LED_LAYOUT == `$INSTANCE_NAME`_LED_LAYOUT_SPIRAL)
            x = x + (y * `$INSTANCE_NAME`_COLUMNS);
            y = 0;
            
        #elif (`$INSTANCE_NAME`_LED_LAYOUT == `$INSTANCE_NAME`_LED_LAYOUT_GRID16X16)
            	x = (((x/(int32)16)*(int32)256) + (uint32)`$INSTANCE_NAME`_cTrans[y % 16][x % 16]);
		        y = (y/(int32)16);
                
        #elif (`$INSTANCE_NAME`_LED_LAYOUT == `$INSTANCE_NAME`_LED_LAYOUT_GRID8X8)
            	x = (((x/(int32)8)*(int32)64) + (uint32)`$INSTANCE_NAME`_cTrans[y % 8][x % 8]);
		        y = (y/(int32)8);    
        #endif
         
        #if(`$INSTANCE_NAME`_MEMORY_TYPE == `$INSTANCE_NAME`_MEMORY_RGB)
            `$INSTANCE_NAME`_ledArray[y][x] = color;
        #else  /* Else use lookup table */
            `$INSTANCE_NAME`_ledArray[y][x] = (uint8)color;
        #endif    
            

    }
}



/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_GetPixel
********************************************************************************
*
* Summary:
*  Get Pixel Color  
*
* Parameters:  
*  x,y:    Location to get pixel color
*
* Return: 
*  None 
*******************************************************************************/
uint32 `$INSTANCE_NAME`_GetPixel(int32 x, int32 y)
{
    uint32 color = 0;
    #if (`$INSTANCE_NAME`_COORD_WRAP & `$INSTANCE_NAME`_COORD_XAXIS )
        x = x % (`$INSTANCE_NAME`_MAX_X+1);
    #endif
    #if (`$INSTANCE_NAME`_COORD_WRAP & `$INSTANCE_NAME`_COORD_YAXIS )
        y = y % (`$INSTANCE_NAME`_MAX_Y+1);
    #endif
    
    if((x>=0) && (y>=0) && (x < `$INSTANCE_NAME`_ARRAY_COLS) && (y < `$INSTANCE_NAME`_ARRAY_ROWS))
    {
                
        #if (`$INSTANCE_NAME`_LED_LAYOUT == `$INSTANCE_NAME`_LED_LAYOUT_STANDARD)
            
        #elif (`$INSTANCE_NAME`_LED_LAYOUT == `$INSTANCE_NAME`_LED_LAYOUT_SPIRAL)
            x = x + (y * `$INSTANCE_NAME`_COLUMNS);
            y = 0;
            
        #elif (`$INSTANCE_NAME`_LED_LAYOUT == `$INSTANCE_NAME`_LED_LAYOUT_GRID16X16)
            	x = (((x/(int32)16)*(int32)256) + (uint32)`$INSTANCE_NAME`_cTrans[y % 16][x % 16]);
		        y = (y/(int32)16); 
                
        #elif (`$INSTANCE_NAME`_LED_LAYOUT == `$INSTANCE_NAME`_LED_LAYOUT_GRID8X8)
            	x = (((x/(int32)8)*(int32)64) + (uint32)`$INSTANCE_NAME`_cTrans[y % 8][x % 8]);
		        y = (y/(int32)8);  
        #endif
    
        #if(`$INSTANCE_NAME`_MEMORY_TYPE == `$INSTANCE_NAME`_MEMORY_RGB)
            color = `$INSTANCE_NAME`_ledArray[y][x];
        #else  /* Else use lookup table */
            color = (uint32)`$INSTANCE_NAME`_ledArray[y][x];
        #endif

    }
    return(color);
}

/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_DrawCircle
********************************************************************************
*
* Summary:
*  Draw a circle on the display given a start point and radius.  
*
*  This code uses Bresenham's Circle Algorithm. 
*
* Parameters:  
*  x0, y0: Center of circle
*  radius: Radius of circle
*  color:  Color of circle
*
* Return: 
*  None 
*  
*******************************************************************************/
void `$INSTANCE_NAME`_DrawCircle (int32 x0, int32 y0, int32 radius, uint32 color)
{
	int32 f = 1 - radius;
	int32 ddF_x = 0;
	int32 ddF_y = -2 * radius;
	int32 x = 0;
	int32 y = radius;

	`$INSTANCE_NAME`_Pixel(x0, y0 + radius, color);
	`$INSTANCE_NAME`_Pixel(x0, y0 - radius, color);
	`$INSTANCE_NAME`_Pixel( x0 + radius, y0, color);
	`$INSTANCE_NAME`_Pixel( x0 - radius, y0, color);

	while(x < y)
	{
		if(f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x + 1;

		`$INSTANCE_NAME`_Pixel(x0 + x, y0 + y, color);
		`$INSTANCE_NAME`_Pixel(x0 - x, y0 + y, color);
		`$INSTANCE_NAME`_Pixel( x0 + x, y0 - y, color);
		`$INSTANCE_NAME`_Pixel( x0 - x, y0 - y, color);
		`$INSTANCE_NAME`_Pixel( x0 + y, y0 + x, color);
		`$INSTANCE_NAME`_Pixel( x0 - y, y0 + x, color);
		`$INSTANCE_NAME`_Pixel( x0 + y, y0 - x, color);
		`$INSTANCE_NAME`_Pixel( x0 - y, y0 - x, color);
	}
}


/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_DrawLine
********************************************************************************
*
* Summary:
*  Draw a line on the display.  
*
* Parameters:  
*  x0, y0:  The beginning endpoint
*  x1, y1:  The end endpoint.
*  color:   Color of the line.
*
* Return: 
*  None 
*  
*******************************************************************************/
void `$INSTANCE_NAME`_DrawLine(int32 x0, int32 y0, int32 x1, int32 y1, uint32 color)
{
	int32 dy = y1 - y0; /* Difference between y0 and y1 */
	int32 dx = x1 - x0; /* Difference between x0 and x1 */
	int32 stepx, stepy;

	if (dy < 0)
	{
		dy = -dy;
		stepy = -1;
	}
	else
	{
		stepy = 1;
	}

	if (dx < 0)
	{
		dx = -dx;
		stepx = -1;
	}
	else
	{
		stepx = 1;
	}

	dy <<= 1; /* dy is now 2*dy  */
	dx <<= 1; /* dx is now 2*dx  */
	`$INSTANCE_NAME`_Pixel(x0, y0, color);

	if (dx > dy) 
	{
		int fraction = dy - (dx >> 1);
		while (x0 != x1)
		{
			if (fraction >= 0)
			{
				y0 += stepy;
				fraction -= dx;
			}
			x0 += stepx;
			fraction += dy;
			`$INSTANCE_NAME`_Pixel(x0, y0, color);
		}
	}
	else
	{
		int fraction = dx - (dy >> 1);
		while (y0 != y1)
		{
			if (fraction >= 0)
			{
				x0 += stepx;
				fraction -= dy;
			}
			y0 += stepy;
			fraction += dx;
			`$INSTANCE_NAME`_Pixel( x0, y0, color);
		}
	}
}

/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_DrawRect
********************************************************************************
*
* Summary:
*  Draw a rectangle, filled or not.  
*
* Parameters:  
*  x0, y0:  The upper lefthand corner.
*  x1, y1:  The lower right corner.
*  fill:    Non-Zero if retangle is to be filled.
*  color:   Color for rectangle, border and fill.
*
* Return: 
*  None 
*  
*******************************************************************************/
void `$INSTANCE_NAME`_DrawRect(int32 x0, int32 y0, int32 x1, int32 y1, int32 fill, uint32 color)
{	
     int xDiff;
	/* Check if the rectangle is to be filled    */
	if (fill != 0)
	{	
        /* Find the difference between the x vars */
		if(x0 > x1)
		{
			xDiff = x0 - x1; 
		}
		else
		{
			xDiff = x1 - x0;
		}
	
	    /* Fill it with lines  */
		while(xDiff >= 0)
		{
			`$INSTANCE_NAME`_DrawLine(x0, y0, x0, y1, color);
		
			if(x0 > x1)
				x0--;
			else
				x0++;
		
			xDiff--;
		}
	}
	else 
	{
		/* Draw the four sides of the rectangle */
		`$INSTANCE_NAME`_DrawLine(x0, y0, x1, y0, color);
		`$INSTANCE_NAME`_DrawLine(x0, y1, x1, y1, color);
		`$INSTANCE_NAME`_DrawLine(x0, y0, x0, y1, color);
		`$INSTANCE_NAME`_DrawLine(x1, y0, x1, y1, color);
	}
}

/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_Dim
********************************************************************************
*
* Summary:
*  Dim all output by a specific level (0,1,2,3,4)  
*
* Parameters:  
*  dimLevel:  Dim level 1 to 4, 0 => No dimming.
*
* Return: 
*  None 
*  
*******************************************************************************/
void `$INSTANCE_NAME`_Dim(uint32 dimLevel) 
{
extern uint32  `$INSTANCE_NAME`_DimMask;
extern uint32  `$INSTANCE_NAME`_DimShift;

    switch(dimLevel)
    {
       case 1:  // 1/2 bright
           `$INSTANCE_NAME`_DimMask = 0x7F7F7F7F;
           `$INSTANCE_NAME`_DimShift = 1;
           break;
           
       case 2:
           `$INSTANCE_NAME`_DimMask = 0x3F3F3F3F;
           `$INSTANCE_NAME`_DimShift = 2;
           break;
           
       case 3:
           `$INSTANCE_NAME`_DimMask = 0x1F1F1F1F;
           `$INSTANCE_NAME`_DimShift = 3;
           break;
           
       case 4:
           `$INSTANCE_NAME`_DimMask = 0x0F0F0F0F;
           `$INSTANCE_NAME`_DimShift = 4;
           break;
           
       default:
           `$INSTANCE_NAME`_DimMask = 0xFFFFFFFF;
           `$INSTANCE_NAME`_DimShift = 0;
           break; 
    }
}

/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_Bplot8
********************************************************************************
*
* Summary:
*  Draw a bitmap image on the display using 8-bit color. 
*
* Parameters:  
*    x, y:  The upper left corner of the image location
*  bitMap:  A pointer to an image array.  The first two values is the X and Y
*           size of the bitmap.
*  update:  If not 0 update the display after drawing image.
*
* Return: 
*  None 
*  
*******************************************************************************/
void `$INSTANCE_NAME`_Bplot8( int32 x, int32 y, uint8 * bitMap, int32 update)
{
    int32 dx, dy;
    int32 aindex = 0;
    int32 maxX, maxY;

    maxX = x + (int32)bitMap[aindex++];
    maxY = y  + (int32)bitMap[aindex++];

	for(dy = y; dy < maxY; dy++)
    {
		for(dx = x; dx < maxX; dx++)
        {
            `$INSTANCE_NAME`_Pixel(dx, dy, bitMap[aindex++]);
        }
    }
	if(update) `$INSTANCE_NAME`_Trigger(1);
}

/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_Bplot32
********************************************************************************
*
* Summary:
*  Draw a bitmap image on the display using 32-bit color. 
*
* Parameters:  
*    x, y:  The upper left corner of the image location
*  bitMap:  A pointer to an image array.  The first two values is the X and Y
*           size of the bitmap.
*  update:  If not 0 update the display after drawing image.
*
* Return: 
*  None 
*  
*******************************************************************************/
void `$INSTANCE_NAME`_Bplot32( int32 x, int32 y, uint32 * bitMap, int32 update)
{
    int32 dx, dy;
    int32 aindex = 0;
    int32 maxX, maxY;

    maxX = x + (int32)bitMap[aindex++];
    maxY = y  + (int32)bitMap[aindex++];

	for(dy = y; dy < maxY; dy++)
    {
		for(dx = x; dx < maxX; dx++)
        {
            `$INSTANCE_NAME`_Pixel(dx, dy, bitMap[aindex++]);
        }
    }
	if(update) `$INSTANCE_NAME`_Trigger(1);
}

/*******************************************************************************
* Function Name: `$INSTANCE_NAME`_RgbBlend()
********************************************************************************
* Summary:  Blend two colors into one.
*    
*    newColor = (pct * toColor)  + ((100-pct) * fromColor)
*
* Parameters:  
*  void  
*
* Return: 
*  void
*
*******************************************************************************/
uint32 `$INSTANCE_NAME`_RgbBlend(uint32 fromColor, uint32 toColor, uint32 pct)
{
    uint32 newColor = 0;
 
    #if(`$INSTANCE_NAME`_CHIP == `$INSTANCE_NAME`_CHIP_SF2812RGBW)
        uint32 tc, fc;
        
        tc = toColor >> 8;
        fc = fromColor >> 8;
        newColor = ((((pct * (tc & 0x00FF0000)) + ((100-pct) * (fc & 0x00FF0000)))/100) & 0x00FF0000)<<8; 
    #endif
    newColor |= (((pct * (toColor & 0x00FF0000)) + ((100-pct) * (fromColor & 0x00FF0000)))/100) & 0x00FF0000;
    newColor |= (((pct * (toColor & 0x0000FF00)) + ((100-pct) * (fromColor & 0x0000FF00)))/100) & 0x0000FF00;
    newColor |= (((pct * (toColor & 0x000000FF)) + ((100-pct) * (fromColor & 0x000000FF)))/100) & 0x000000FF;
 
   return(newColor);
}

/* [] END OF FILE */
