/*
 * UD3
 *
 * Copyright (c) 2018 Jens Kerrinnes
 * Copyright (c) 2015 Steve Ward
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#if !defined(tsk_display_TASK_H)
#define tsk_display_TASK_H

/*
 * Add user task definitions, types, includes and other things in the below
 * merge region to customize the task.
 */
/* `#START USER_TYPES_AND_DEFINES` */
#include <device.h>
#include "TTerm.h"

/* `#END` */
    
#define DISP_MAX_ZONES 10

#define DISP_SRC_OFF            0
#define DISP_SRC_BUS            1
#define DISP_SRC_SYNTH          2
    
#define DISP_SRC_HWG0           9
#define DISP_SRC_HWG1           10
#define DISP_SRC_HWG2           11
#define DISP_SRC_HWG3           12
#define DISP_SRC_HWG4           13
#define DISP_SRC_HWG5           14
    
#define DISP_SRC_WHITE_STATIC   15
    
typedef struct{
    uint8_t firstLed        : 6;
    uint8_t lastLed         : 6;
    uint8_t src             : 4;
} DISP_ZONE_s;

typedef struct{
    uint8_t red : 5;
    uint8_t green : 6;
    uint8_t blue : 5;
} SMALL_COLOR;

typedef union{
    DISP_ZONE_s zone[DISP_MAX_ZONES];
    uint16_t data[DISP_MAX_ZONES];
} DISP_ZONE_t;

extern DISP_ZONE_t DISP_zones;
    
void tsk_display_Start(void);
uint32_t DISP_getZoneColor(uint8_t src);
uint8_t CMD_display(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);

uint32_t smallColorToColor(SMALL_COLOR c);

/*
 * Add user function prototypes in the below merge region to add user
 * functionality to the task definition.
 */
/* `#START USER_TASK_PROTOS` */

/* `#END` */

/* ------------------------------------------------------------------------ */
#endif
/* [] END OF FILE */
