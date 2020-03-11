/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

/*  FONTS  */

#define `$INSTANCE_NAME`_FONT_5X7  0

/* 5x7 Font */
//#define `$INSTANCE_NAME`_Font5x7_COLS  5
//#define `$INSTANCE_NAME`_Font5x7_ROWS  7
#define `$INSTANCE_NAME`_Font5x7_XSIZE  5
#define `$INSTANCE_NAME`_Font5x7_YSIZE  7
#define `$INSTANCE_NAME`_Font5x7_OFFSET 32u
#define `$INSTANCE_NAME`_COLORWHEEL_FONT  0x80000000
#define `$INSTANCE_NAME`_TRANS_BG         0xFF000000


void `$INSTANCE_NAME`_SetFont( uint32 font);
void `$INSTANCE_NAME`_PutChar(int32 row, int32 col, uint8 theChar, uint32 fg, uint32 bg);
void `$INSTANCE_NAME`_PrintString(int32 row, int32 col, char * theString, uint32 fg, uint32 bg);

//[] END OF FILE
