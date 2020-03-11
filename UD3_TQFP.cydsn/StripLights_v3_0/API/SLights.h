/* ========================================
 *
 * Strip Light Controller
 * By Mark Hastings
 *
 * 05/27/2013  v1.0  Mark Hastings   Initial working version
 * 12/11/2013  v2.6  Mark Hastings   Update for SK2812
 *
 * ========================================
*/

#if (!defined(CY_SLIGHTS_`$INSTANCE_NAME`_H))
#define CY_SLIGHTS_`$INSTANCE_NAME`_H

#include "cytypes.h"
#include "cyfitter.h"
    
typedef void(*funcPtr)(void);

/* Function Prototypes */
void   `$INSTANCE_NAME`_Start(void);
void   `$INSTANCE_NAME`_Stop(void);
void   `$INSTANCE_NAME`_WriteColor(uint32 color);
void   `$INSTANCE_NAME`_DisplayClear(uint32 color);
void   `$INSTANCE_NAME`_MemClear(uint32 color);
void   `$INSTANCE_NAME`_Trigger(uint32 rst);
uint32 `$INSTANCE_NAME`_Ready(void);
void   `$INSTANCE_NAME`_SetCriticalFuncs(funcPtr EnCriticalFunc, funcPtr ExCriticalFunc);
void   `$INSTANCE_NAME`_TriggerWait(uint32 rst);
void   `$INSTANCE_NAME`_TriggerWaitDelay(uint32 delay);

void   `$INSTANCE_NAME`_DrawRect(int32 x0, int32 y0, int32 x1, int32 y1, int32 fill, uint32 color);
void   `$INSTANCE_NAME`_DrawLine(int32 x0, int32 y0, int32 x1, int32 y1, uint32 color);
void   `$INSTANCE_NAME`_DrawCircle (int32 x0, int32 y0, int32 radius, uint32 color);
void   `$INSTANCE_NAME`_Pixel(int32 x, int32 y, uint32 color);
uint32 `$INSTANCE_NAME`_GetPixel(int32 x, int32 y);
uint32 `$INSTANCE_NAME`_ColorInc(uint32 incValue);
void   `$INSTANCE_NAME`_Dim(uint32 dimLevel); 
uint32 `$INSTANCE_NAME`_getColor( uint32 color);
void   `$INSTANCE_NAME`_Bplot8( int32 x, int32 y, uint8 * bitMap, int32 update);
void   `$INSTANCE_NAME`_Bplot32( int32 x, int32 y, uint32 * bitMap, int32 update);
uint32 `$INSTANCE_NAME`_RgbBlend(uint32 fromColor, uint32 toColor, uint32 pct);

#define `$INSTANCE_NAME`_DimLevel_0   0
#define `$INSTANCE_NAME`_DimLevel_1   1
#define `$INSTANCE_NAME`_DimLevel_2   2
#define `$INSTANCE_NAME`_DimLevel_3   3
#define `$INSTANCE_NAME`_DimLevel_4   4

#define `$INSTANCE_NAME`_READY_TIMEOUT 200  // 200 milliseconds (6000 LEDs)

#define `$INSTANCE_NAME`_CIRQ_Enable() CyIntEnable(`$INSTANCE_NAME`_CIRQ__INTC_NUMBER ); 
#define `$INSTANCE_NAME`_CIRQ_Disable() CyIntDisable(`$INSTANCE_NAME`_CIRQ__INTC_NUMBER );
CY_ISR_PROTO(`$INSTANCE_NAME`_CISR);

#define `$INSTANCE_NAME`_FIRQ_Enable() CyIntEnable(`$INSTANCE_NAME`_FIRQ__INTC_NUMBER ); 
#define `$INSTANCE_NAME`_FIRQ_Disable() CyIntDisable(`$INSTANCE_NAME`_FIRQ__INTC_NUMBER );
CY_ISR_PROTO(`$INSTANCE_NAME`_FISR);

/* Register Definitions */
#define `$INSTANCE_NAME`_DATA         (*(reg8 *) `$INSTANCE_NAME`_B_WS2811_dshifter_u0__F0_REG)
#define `$INSTANCE_NAME`_DATA_PTR     ((reg8 *)  `$INSTANCE_NAME`_B_WS2811_dshifter_u0__F0_REG)

#define `$INSTANCE_NAME`_CONTROL      (*(reg8 *) `$INSTANCE_NAME`_B_WS2811_ctrl__CONTROL_REG)
#define `$INSTANCE_NAME`_STATUS       (*(reg8 *) `$INSTANCE_NAME`_B_WS2811_StatusReg__STATUS_REG)

#define `$INSTANCE_NAME`_Period       (*(reg8 *) `$INSTANCE_NAME`_B_WS2811_pwm8_u0__F0_REG)
#define `$INSTANCE_NAME`_Period_PTR   ((reg8 *)  `$INSTANCE_NAME`_B_WS2811_pwm8_u0__F0_REG)

#define `$INSTANCE_NAME`_Compare0     (*(reg8 *) `$INSTANCE_NAME`_B_WS2811_pwm8_u0__D0_REG)
#define `$INSTANCE_NAME`_Compare1     (*(reg8 *) `$INSTANCE_NAME`_B_WS2811_pwm8_u0__D1_REG)

#define `$INSTANCE_NAME`_Period2      (*(reg8 *) `$INSTANCE_NAME`_B_WS2811_pwm8_u0__F1_REG)
#define `$INSTANCE_NAME`_Period2_PTR  ((reg8 *)  `$INSTANCE_NAME`_B_WS2811_pwm8_u0__F1_REG)

#define `$INSTANCE_NAME`_ACTL0_REG    (*(reg8 *) `$INSTANCE_NAME`_B_WS2811_pwm8_u0__DP_AUX_CTL_REG)
#define `$INSTANCE_NAME`_DISABLE_FIFO  0x03

#define `$INSTANCE_NAME`_Channel      (*(reg8 *) `$INSTANCE_NAME`_StringSel_Sync_ctrl_reg__CONTROL_REG)
#define `$INSTANCE_NAME`_Channel_PTR  ((reg8 *)  `$INSTANCE_NAME`_StringSel_Sync_ctrl_reg__CONTROL_REG)

/* Status Register Constants  */
#define `$INSTANCE_NAME`_FIFO_EMPTY       0x01
#define `$INSTANCE_NAME`_FIFO_NOT_FULL    0x02
#define `$INSTANCE_NAME`_STATUS_ENABLE    0x80
#define `$INSTANCE_NAME`_STATUS_XFER_CMPT 0x40

/* Control Register Constants */
#define `$INSTANCE_NAME`_ENABLE         0x01
#define `$INSTANCE_NAME`_DISABLE        0x00
#define `$INSTANCE_NAME`_RESTART        0x02
#define `$INSTANCE_NAME`_CNTL           0x04
#define `$INSTANCE_NAME`_FIFO_IRQ_EN    0x08
#define `$INSTANCE_NAME`_XFRCMPT_IRQ_EN 0x10
#define `$INSTANCE_NAME`_ALL_IRQ_EN     0x18
#define `$INSTANCE_NAME`_NEXT_ROW       0x20

#define `$INSTANCE_NAME`_TRANSFER           `$Transfer_Method`
#define `$INSTANCE_NAME`_TRANSFER_FIRMWARE  0
#define `$INSTANCE_NAME`_TRANSFER_ISR       1
#define `$INSTANCE_NAME`_TRANSFER_DMA       2

#define `$INSTANCE_NAME`_SPEED        `$Speed`
#define `$INSTANCE_NAME`_SPEED_400KHZ 0
#define `$INSTANCE_NAME`_SPEED_800KHZ 1

#define `$INSTANCE_NAME`_MEMORY_TYPE  `$Display_Memory`
#define `$INSTANCE_NAME`_MEMORY_RGB   0
#define `$INSTANCE_NAME`_MEMORY_LUT   1

#define `$INSTANCE_NAME`_GAMMA_CORRECTION   `$Gamma_Correction`
#define `$INSTANCE_NAME`_GAMMA_ON            1
#define `$INSTANCE_NAME`_GAMMA_OFF           0

#define `$INSTANCE_NAME`_COORD_WRAP        `$Coord_Wrap`
#define `$INSTANCE_NAME`_COORD_NONE        (0u)
#define `$INSTANCE_NAME`_COORD_XAXIS       (1u)
#define `$INSTANCE_NAME`_COORD_YAXIS       (2u)
#define `$INSTANCE_NAME`_COORD_XYAXIS      (3u)

#define `$INSTANCE_NAME`_LED_LAYOUT       `$LED_Config`
#define `$INSTANCE_NAME`_LED_LAYOUT_STANDARD  (0u)
#define `$INSTANCE_NAME`_LED_LAYOUT_SPIRAL    (1u)
#define `$INSTANCE_NAME`_LED_LAYOUT_GRID16X16 (2u)
#define `$INSTANCE_NAME`_LED_LAYOUT_GRID8X8   (3u)

#define `$INSTANCE_NAME`_FLIP_X_COORD      `$Flip_X_Coord`
#define `$INSTANCE_NAME`_FLIP_Y_COORD      `$Flip_Y_Coord`

#define `$INSTANCE_NAME`_FLIP_COORD         (1u)
#define `$INSTANCE_NAME`_NOT_FLIP_COORD      (0u)

#define `$INSTANCE_NAME`_SWAP_XY_COORD      `$Swap_X_Y`
#define `$INSTANCE_NAME`_XY_SWAPED          (1u)

#if (`$INSTANCE_NAME`_SPEED_800KHZ)
    #define `$INSTANCE_NAME`_BYTE_TIME_US 10u
    #define `$INSTANCE_NAME`_WORD_TIME_US 30u
#else
    #define `$INSTANCE_NAME`_BYTE_TIME_US 20u
    #define `$INSTANCE_NAME`_WORD_TIME_US 60u
#endif

#define `$INSTANCE_NAME`_GRIDMODE_NORMAL  1
#define `$INSTANCE_NAME`_GRIDMODE_WRAP    0


#if (`$INSTANCE_NAME`_LED_LAYOUT == `$INSTANCE_NAME`_LED_LAYOUT_STANDARD)
    #define `$INSTANCE_NAME`_COLUMNS      `$LEDs_per_Strip`
    #define `$INSTANCE_NAME`_ROWS         `$LedChannels`
    #define `$INSTANCE_NAME`_TOTAL_LEDS   (`$INSTANCE_NAME`_COLUMNS*`$INSTANCE_NAME`_ROWS)

    #define `$INSTANCE_NAME`_ARRAY_COLS   (int32)(`$LEDs_per_Strip`)
    #define `$INSTANCE_NAME`_ARRAY_ROWS   (int32)(`$LedChannels`)    
    
    #define `$INSTANCE_NAME`_MIN_X        (int32)0u
    #define `$INSTANCE_NAME`_MAX_X        (int32)(`$INSTANCE_NAME`_COLUMNS - 1)
    #define `$INSTANCE_NAME`_MIN_Y        (int32)0u
    #define `$INSTANCE_NAME`_MAX_Y        (int32)(`$INSTANCE_NAME`_ROWS - 1)
    
#elif (`$INSTANCE_NAME`_LED_LAYOUT == `$INSTANCE_NAME`_LED_LAYOUT_SPIRAL)

    #define `$INSTANCE_NAME`_COLUMNS      `$String_Wrap_Length`  
    #define `$INSTANCE_NAME`_ROWS         ((`$LEDs_per_Strip` +  (`$String_Wrap_Length` - 1 ))/(`$String_Wrap_Length` ) )
    #define `$INSTANCE_NAME`_TOTAL_LEDS   `$LEDs_per_Strip`

    #define `$INSTANCE_NAME`_ARRAY_COLS   (int32)(`$LEDs_per_Strip` + `$String_Wrap_Length`)
    #define `$INSTANCE_NAME`_ARRAY_ROWS   (int32)(1u)  
    
    #define `$INSTANCE_NAME`_MIN_X        (int32)0u
    #define `$INSTANCE_NAME`_MAX_X        (int32)(`$INSTANCE_NAME`_COLUMNS - 1)
    #define `$INSTANCE_NAME`_MIN_Y        (int32)0u
    #define `$INSTANCE_NAME`_MAX_Y        (int32)(`$INSTANCE_NAME`_ROWS - 1)
    
#elif (`$INSTANCE_NAME`_LED_LAYOUT == `$INSTANCE_NAME`_LED_LAYOUT_GRID16X16)

    #define `$INSTANCE_NAME`_COLUMNS      (int32)((`$Grid_16x16_Columns`) * (uint32)(16u) )
    #define `$INSTANCE_NAME`_ROWS         (int32)((`$Grid_16x16_Rows`) * (uint32)(16u) )
    #define `$INSTANCE_NAME`_TOTAL_LEDS   (`$INSTANCE_NAME`_COLUMNS*`$INSTANCE_NAME`_ROWS)

    #define `$INSTANCE_NAME`_ARRAY_COLS   (int32)(`$Grid_16x16_Columns`*(int32)256)
    #define `$INSTANCE_NAME`_ARRAY_ROWS   (int32)(`$Grid_16x16_Rows`)
    
    #define `$INSTANCE_NAME`_MIN_X        (int32)0
    #define `$INSTANCE_NAME`_MAX_X        (int32)((`$Grid_16x16_Columns`*(int32)16) - (int32)1)
    #define `$INSTANCE_NAME`_MIN_Y        (int32)0
    #define `$INSTANCE_NAME`_MAX_Y        (int32)((`$Grid_16x16_Rows`*(int32)16) - (int32)1)

#elif (`$INSTANCE_NAME`_LED_LAYOUT == `$INSTANCE_NAME`_LED_LAYOUT_GRID8X8)

    #define `$INSTANCE_NAME`_COLUMNS      (int32)((`$Grid_16x16_Columns`) * (uint32)(8u) )
    #define `$INSTANCE_NAME`_ROWS         (int32)((`$Grid_16x16_Rows`) * (uint32)(8u) )
    #define `$INSTANCE_NAME`_TOTAL_LEDS   (`$INSTANCE_NAME`_COLUMNS*`$INSTANCE_NAME`_ROWS)

    #define `$INSTANCE_NAME`_ARRAY_COLS   (int32)(`$Grid_16x16_Columns`*(int32)64)
    #define `$INSTANCE_NAME`_ARRAY_ROWS   (int32)(`$Grid_16x16_Rows`)
    
    #define `$INSTANCE_NAME`_MIN_X        (int32)0
    #define `$INSTANCE_NAME`_MAX_X        (int32)((`$Grid_16x16_Columns`*(int32)8) - (int32)1)
    #define `$INSTANCE_NAME`_MIN_Y        (int32)0
    #define `$INSTANCE_NAME`_MAX_Y        (int32)((`$Grid_16x16_Rows`*(int32)8) - (int32)1)
    
#endif
    
    
#define `$INSTANCE_NAME`_CHIP        (`$WS281x_Type`)
#define `$INSTANCE_NAME`_CHIP_WS2811     1
#define `$INSTANCE_NAME`_CHIP_WS2812     2
#define `$INSTANCE_NAME`_CHIP_SF2812RGBW 3

#define `$INSTANCE_NAME`_TIME_TL_TH    1250U  // 1250 us period
#if((`$INSTANCE_NAME`_CHIP == `$INSTANCE_NAME`_CHIP_WS2812) || (`$INSTANCE_NAME`_CHIP == `$INSTANCE_NAME`_CHIP_WS2811))
    #define `$INSTANCE_NAME`_TIME_T0H  300U     // Logic 0 high, 300 us 
    #define `$INSTANCE_NAME`_TIME_T1H  700U     // Logic 1 low, 700 us
#else
    #define `$INSTANCE_NAME`_TIME_T0H  300U     // Logic 0 high, 300 us 
    #define `$INSTANCE_NAME`_TIME_T1H  650U     // Logic 1 low, 700 us
#endif
 

#if (CY_PSOC3 || CY_PSOC5LP)
    #define  `$INSTANCE_NAME`_INPUT_CLOCK    BCLK__BUS_CLK__KHZ
    #define  `$INSTANCE_NAME`_WS281X_CLOCK   `$ClockSpeedKhz`
    #define  `$INSTANCE_NAME`_PERIOD         ((`$INSTANCE_NAME`_INPUT_CLOCK)/`$ClockSpeedKhz`)
#elif (CY_PSOC4)

    #define  `$INSTANCE_NAME`_INPUT_CLOCK    ( CYDEV_BCLK__HFCLK__KHZ/2 )
    #define  `$INSTANCE_NAME`_PERIOD         ((`$INSTANCE_NAME`_INPUT_CLOCK)/`$ClockSpeedKhz`)
    #define  `$INSTANCE_NAME`_DATA_ZERO      ((`$INSTANCE_NAME`_PERIOD * `$INSTANCE_NAME`_TIME_T0H)/`$INSTANCE_NAME`_TIME_TL_TH) 
    #define  `$INSTANCE_NAME`_DATA_ONE       ((`$INSTANCE_NAME`_PERIOD * `$INSTANCE_NAME`_TIME_T1H)/`$INSTANCE_NAME`_TIME_TL_TH)
    
    #define  `$INSTANCE_NAME`_COMPARE_ZERO   (`$INSTANCE_NAME`_PERIOD - `$INSTANCE_NAME`_DATA_ZERO)
    #define  `$INSTANCE_NAME`_COMPARE_ONE    (`$INSTANCE_NAME`_PERIOD - `$INSTANCE_NAME`_DATA_ONE)
#endif /* CY_PSOC5A */

#define `$INSTANCE_NAME`_COLOR_WHEEL_SIZE  24

#if(`$INSTANCE_NAME`_CHIP == `$INSTANCE_NAME`_CHIP_WS2812)
#define `$INSTANCE_NAME`_RED_MASK   0x0000FF00
#define `$INSTANCE_NAME`_GREEN_MASK 0x000000FF
#define `$INSTANCE_NAME`_BLUE_MASK  0x00FF0000
#else
#define `$INSTANCE_NAME`_RED_MASK   0x000000FF
#define `$INSTANCE_NAME`_GREEN_MASK 0x0000FF00
#define `$INSTANCE_NAME`_BLUE_MASK  0x00FF0000	
#endif

#define `$INSTANCE_NAME`_CWHEEL_SIZE 24
#define `$INSTANCE_NAME`_YELLOW      `$INSTANCE_NAME`_getColor(1)
#define `$INSTANCE_NAME`_GREEN       `$INSTANCE_NAME`_getColor((70 + `$INSTANCE_NAME`_CWHEEL_SIZE))
#define `$INSTANCE_NAME`_ORANGE      `$INSTANCE_NAME`_getColor(20)
#define `$INSTANCE_NAME`_BLACK       `$INSTANCE_NAME`_getColor((0 + `$INSTANCE_NAME`_CWHEEL_SIZE))
#define `$INSTANCE_NAME`_OFF         `$INSTANCE_NAME`_getColor((0 + `$INSTANCE_NAME`_CWHEEL_SIZE))
#define `$INSTANCE_NAME`_LTBLUE      `$INSTANCE_NAME`_getColor((1 + `$INSTANCE_NAME`_CWHEEL_SIZE))
#define `$INSTANCE_NAME`_MBLUE       `$INSTANCE_NAME`_getColor((2 + `$INSTANCE_NAME`_CWHEEL_SIZE))
#define `$INSTANCE_NAME`_BLUE        `$INSTANCE_NAME`_getColor((3 + `$INSTANCE_NAME`_CWHEEL_SIZE))
#define `$INSTANCE_NAME`_LTGREEN     `$INSTANCE_NAME`_getColor((4 + `$INSTANCE_NAME`_CWHEEL_SIZE))
#define `$INSTANCE_NAME`_MGREEN      `$INSTANCE_NAME`_getColor((8 + `$INSTANCE_NAME`_CWHEEL_SIZE))
//#define `$INSTANCE_NAME`_GREEN       (12 + `$INSTANCE_NAME`_CWHEEL_SIZE) 
#define `$INSTANCE_NAME`_LTRED       `$INSTANCE_NAME`_getColor((16 + `$INSTANCE_NAME`_CWHEEL_SIZE)) 
#define `$INSTANCE_NAME`_LTYELLOW    `$INSTANCE_NAME`_getColor((20 + `$INSTANCE_NAME`_CWHEEL_SIZE))
#define `$INSTANCE_NAME`_MGRED       `$INSTANCE_NAME`_getColor((32 + `$INSTANCE_NAME`_CWHEEL_SIZE)) 
#define `$INSTANCE_NAME`_RED         `$INSTANCE_NAME`_getColor((48 + `$INSTANCE_NAME`_CWHEEL_SIZE)) 
#define `$INSTANCE_NAME`_MAGENTA     `$INSTANCE_NAME`_getColor((51 + `$INSTANCE_NAME`_CWHEEL_SIZE))
#define `$INSTANCE_NAME`_RGB_WHITE   `$INSTANCE_NAME`_getColor((63 + `$INSTANCE_NAME`_CWHEEL_SIZE)) 

#define `$INSTANCE_NAME`_SPRING_GREEN `$INSTANCE_NAME`_getColor((64 + `$INSTANCE_NAME`_CWHEEL_SIZE)) 
#define `$INSTANCE_NAME`_TURQUOSE    `$INSTANCE_NAME`_getColor((65 + `$INSTANCE_NAME`_CWHEEL_SIZE)) 
#define `$INSTANCE_NAME`_CYAN        `$INSTANCE_NAME`_getColor((66 + `$INSTANCE_NAME`_CWHEEL_SIZE)) 
#define `$INSTANCE_NAME`_OCEAN       `$INSTANCE_NAME`_getColor((67 + `$INSTANCE_NAME`_CWHEEL_SIZE)) 
#define `$INSTANCE_NAME`_VIOLET      `$INSTANCE_NAME`_getColor((68 + `$INSTANCE_NAME`_CWHEEL_SIZE)) 
#define `$INSTANCE_NAME`_RASPBERRY   `$INSTANCE_NAME`_getColor((69 + `$INSTANCE_NAME`_CWHEEL_SIZE)) 
#define `$INSTANCE_NAME`_DIM_WHITE   `$INSTANCE_NAME`_getColor((71 + `$INSTANCE_NAME`_CWHEEL_SIZE)) 
#define `$INSTANCE_NAME`_DIM_BLUE    `$INSTANCE_NAME`_getColor((72 + `$INSTANCE_NAME`_CWHEEL_SIZE)) 
#define `$INSTANCE_NAME`_INVISIBLE   `$INSTANCE_NAME`_getColor((73 + `$INSTANCE_NAME`_CWHEEL_SIZE))

#define `$INSTANCE_NAME`_COLD_TEMP   `$INSTANCE_NAME`_getColor((80 + `$INSTANCE_NAME`_CWHEEL_SIZE)) 
#define `$INSTANCE_NAME`_HOT_TEMP    `$INSTANCE_NAME`_getColor((95 + `$INSTANCE_NAME`_CWHEEL_SIZE)) 

#define `$INSTANCE_NAME`_FIRE_DARK   `$INSTANCE_NAME`_getColor((74 + `$INSTANCE_NAME`_CWHEEL_SIZE))
#define `$INSTANCE_NAME`_FIRE_LIGHT  `$INSTANCE_NAME`_getColor((75 + `$INSTANCE_NAME`_CWHEEL_SIZE))

#define `$INSTANCE_NAME`_FULL_WHITE  `$INSTANCE_NAME`_getColor((76 + `$INSTANCE_NAME`_CWHEEL_SIZE))
#define `$INSTANCE_NAME`_WHITE_LED   `$INSTANCE_NAME`_getColor((77 + `$INSTANCE_NAME`_CWHEEL_SIZE))
#define `$INSTANCE_NAME`_WHITE_RGB50 `$INSTANCE_NAME`_getColor((78 + `$INSTANCE_NAME`_CWHEEL_SIZE))
#define `$INSTANCE_NAME`_WHITE_RGB25 `$INSTANCE_NAME`_getColor((79 + `$INSTANCE_NAME`_CWHEEL_SIZE))

#define `$INSTANCE_NAME`_CLUT_SIZE  (96 + `$INSTANCE_NAME`_CWHEEL_SIZE)

#if(`$INSTANCE_NAME`_CHIP == `$INSTANCE_NAME`_CHIP_SF2812RGBW)
    #define `$INSTANCE_NAME`_WHITE   `$INSTANCE_NAME`_getColor((77 + `$INSTANCE_NAME`_CWHEEL_SIZE))   
#else
    #define `$INSTANCE_NAME`_WHITE   `$INSTANCE_NAME`_getColor((63 + `$INSTANCE_NAME`_CWHEEL_SIZE))       
#endif

#define `$INSTANCE_NAME`_RESET_DELAY_US  55

#endif  /* CY_SLIGHTS_`$INSTANCE_NAME`_H */

//[] END OF FILE
