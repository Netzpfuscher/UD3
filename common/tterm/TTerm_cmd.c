/*
 * TTerm
 *
 * Copyright (c) 2020 Thorben Zethoff
 * Copyright (c) 2020 Jens Kerrinnes
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

#define TETRIS_GA_WIDTH 10
#define TETRIS_GA_HEIGHT 20

#define TETRIS_CM_XTERM 1
#define TETRIS_CM_VT100 2

#if PIC32 == 1
#include <xc.h>
#endif  
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "TTerm.h"
#include "TTerm_cmd.h"
#include "semphr.h"
#include "system.h"
#include "tasks/tsk_overlay.h"

char TETRISplaceHolder[] = {[0 ... TETRIS_GA_WIDTH] = ' ', [TETRIS_GA_WIDTH + 1] = 0};

typedef struct{
    uint32_t staticRows[(TETRIS_GA_WIDTH/8)+1][TETRIS_GA_HEIGHT];
    uint32_t score;
    uint32_t gameSpeed;
    uint8_t colorMode;
    enum gameState{TETRIS_MM, TETRIS_PLAYING, TETRIS_END};
} TETRIS_GAMESTATE;

typedef enum{
    cyan, blue, orange, yellow, green, purple, red
} TETRIS_color;

typedef struct{
    uint8_t blockData;
    TETRIS_color blockColor;
} TETRIS_BLOCK;

const TETRIS_BLOCK TETRIS_blocks[] = {{.blockData = 0b00001111, cyan}, {.blockData = 0b10001110, blue}, {.blockData = 0b00101110, orange}, {.blockData = 0b11001100, yellow}, {.blockData = 01101100, green}, {.blockData = 01001110, purple}, {.blockData = 11000110, red}};

uint8_t TETRIS_getBlockWidth(TETRIS_BLOCK block){
    if(block.blockData & 0x11) return 4;
    if(block.blockData & 0x22) return 3;
    if(block.blockData & 0x44) return 2;
    if(block.blockData & 0x88) return 1;
    return 0;
}

uint8_t TETRIS_getBlockHeight(TETRIS_BLOCK block){
    if(block.blockData & 0xf0) return 2;
    if(block.blockData & 0x0f) return 1;
    return 0;
}

void TETRIS_drawBlock(TERMINAL_HANDLE * handle, TETRIS_BLOCK block, uint8_t x, uint8_t y){
    uint8_t currX = 0;
    uint8_t currY = 0;
    if(y + TETRIS_getBlockHeight(block) > TETRIS_GA_HEIGHT) currY = 1;
    
    TERM_sendVT100Code(handle, _VT100_CURSOR_POS1, 0);
    TERM_sendVT100Code(handle, _VT100_CURSOR_DOWN_BY, y);
    
    TERM_sendVT100Code(handle, _VT100_BACKGROUND_COLOR, block.blockColor);
    TERM_sendVT100Code(handle, _VT100_FOREGROUND_COLOR, block.blockColor);
    
    for(; currY < 2; currY ++){
        uint8_t currRow = block.blockData >> ((1 - currY) * 4);
        for(; currX < 4; currX ++){
            if(x + currX > TETRIS_GA_WIDTH) break;
            ttprintf("%c", (((currRow >> currX) & 1) == 1) ? 't' : ' ');
        }
        ttprintf("\r\n");
    }
    TERM_sendVT100Code(handle, _VT100_RESET_ATTRIB, 0);
}

uint8_t CMD_testCommandHandler(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    uint8_t currArg = 0;
    uint8_t returnCode = 0;
    for(;currArg<argCount; currArg++){
        if(strcmp(args[currArg], "-?") == 0){
            ttprintf("This function is intended for testing. it will list all passed arguments\r\n");
            ttprintf("usage:\r\n\ttest [{option} {value}]\r\n\n\t-aa : adds an argument to the ACL\r\n\n\t-ra : removes an argument from the ACL\r\n\n\t-r  : returns with the given code");
            return TERM_CMD_EXIT_SUCCESS;
        }
        if(strcmp(args[currArg], "-r") == 0){
            if(argCount > currArg + 1){
                returnCode = atoi(args[currArg + 1]);
                ttprintf("returning %d (from string \"%s\")\r\n", returnCode, args[currArg + 1]);
                currArg++;
                return returnCode;
            }else{
                ttprintf("usage:\r\ntest -r [return code]\r\n");
                return 0;
            }
        }
        if(strcmp(args[currArg], "-ra") == 0){
            if(++currArg < argCount){
                ACL_remove(head, args[currArg]);
                ttprintf("removed \"%s\" from the ACL\r\n", args[currArg]);
                returnCode = TERM_CMD_EXIT_SUCCESS;
            }else{
                ttprintf("missing ACL element value for option \"-ra\"\r\n");
                returnCode = TERM_CMD_EXIT_ERROR;
            }
        }
        if(strcmp(args[currArg], "-aa") == 0){
            if(++currArg < argCount){
                char * newString = pvPortMalloc(strlen(args[currArg])+1);
                strcpy(newString, args[currArg]);
                ACL_add(head, newString);
                ttprintf("Added \"%s\" to the ACL\r\n", args[currArg]);
                returnCode = TERM_CMD_EXIT_SUCCESS;
            }else{
                ttprintf("missing ACL element value for option \"-aa\"\r\n");
                returnCode = TERM_CMD_EXIT_ERROR;
            }
        }
        if(strcmp(args[currArg], "-tt") == 0){
            uint16_t c = 0;
            for(; c < 7; c++){
                TETRIS_drawBlock(handle, TETRIS_blocks[c], c * 7, 60);
            }
            returnCode = TERM_CMD_EXIT_SUCCESS;
        }
    }
    if(returnCode != 0) return returnCode;
    
    ttprintf("Terminal test function called. ArgCount = %d ; Calling user = \"%s\"%s\r\n", argCount, handle->currUserName, (argCount != 0) ? "; \r\narguments={" : "");
    for(currArg = 0;currArg<argCount; currArg++){
        ttprintf("%d:\"%s\"%s\r\n", currArg, args[currArg], (currArg == argCount - 1) ? "\r\n}" : ",");
    }
    return TERM_CMD_EXIT_SUCCESS;
}

uint8_t TERM_testCommandAutoCompleter(TERMINAL_HANDLE * handle, void * params){
    if(params == 0){ 
        handle->autocompleteBufferLength = 0;
        return 0;
    }
    
    AC_LIST_HEAD * list = (AC_LIST_HEAD *) params;
    
    if(list->elementCount == 0){ 
        handle->autocompleteBufferLength = 0;
        return 0;
    }
    
    char * buff = pvPortMalloc(128);
    uint8_t len;
    memset(buff, 0, 128);
    handle->autocompleteStart = TERM_findLastArg(handle, buff, &len);
    
    //TODO use a reasonable size here
    handle->autocompleteBuffer = pvPortMalloc(list->elementCount * sizeof(char *));
    handle->currAutocompleteCount = 0;
    handle->autocompleteBufferLength = TERM_doListAC(list, buff, len, handle->autocompleteBuffer);

    //UART_print("\r\ncompleting \"%s\" (len = %d, matching = %d) will delete until %d\r\n", buff, len, handle->autocompleteBufferLength, handle->autocompleteStart);
        
    vPortFree(buff);
    return handle->autocompleteBufferLength;
}

uint8_t CMD_help(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    uint8_t currArg = 0;
    for(;currArg<argCount; currArg++){
        if(strcmp(args[currArg], "-?") == 0){
            ttprintf("come on do you really need help with help?\r\n");
            return TERM_CMD_EXIT_SUCCESS;
        }
    }
    ttprintf("\r\nTTerm %s\r\n%d Commands available:\r\n\r\n", TERM_VERSION_STRING, handle->cmdListHead->commandLength);
    ttprintf("\x1b[%dC%s\r\x1b[%dC%s\r\n\r\n", 2, "Command:", 19, "Description:");
    TermCommandDescriptor * currCmd = handle->cmdListHead->nextCmd;
    while(currCmd != 0){
        ttprintf("\x1b[%dC%s\r\x1b[%dC%s\r\n", 3, currCmd->command, 20, currCmd->commandDescription);
        currCmd = currCmd->nextCmd;
    }
    return TERM_CMD_EXIT_SUCCESS;
}

uint8_t CMD_cls(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    uint8_t currArg = 0;
    uint8_t returnCode = TERM_CMD_EXIT_SUCCESS;
    for(;currArg<argCount; currArg++){
        if(strcmp(args[currArg], "-?") == 0){
            ttprintf("clears the screen\r\n");
            return TERM_CMD_EXIT_SUCCESS;
        }
    }
    
    TERM_sendVT100Code(handle, _VT100_RESET, 0); TERM_sendVT100Code(handle, _VT100_CURSOR_POS1, 0);
    TERM_printBootMessage(handle);
    
    return TERM_CMD_EXIT_SUCCESS;
}

#define CMD_TOP_STACK 200

uint8_t CMD_top(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    uint8_t currArg = 0;
    uint8_t returnCode = TERM_CMD_EXIT_SUCCESS;
    for(;currArg<argCount; currArg++){
        if(strcmp(args[currArg], "-?") == 0){
            ttprintf("shows performance stats\r\n");
            return TERM_CMD_EXIT_SUCCESS;
        }
    }
    
    TermProgram * prog = pvPortMalloc(sizeof(TermProgram));
    prog->inputHandler = CMD_top_handleInput;
    TERM_sendVT100Code(handle, _VT100_RESET, 0); TERM_sendVT100Code(handle, _VT100_CURSOR_POS1, 0);
    returnCode = xTaskCreate(CMD_top_task, "top", CMD_TOP_STACK, handle, tskIDLE_PRIORITY + 1, &prog->task) ? TERM_CMD_EXIT_PROC_STARTED : TERM_CMD_EXIT_ERROR;
    if(returnCode == TERM_CMD_EXIT_PROC_STARTED) TERM_attachProgramm(handle, prog);
    return returnCode;
}

void CMD_top_task(void *pvParameters){
    TERMINAL_HANDLE * handle = (TERMINAL_HANDLE*)pvParameters;
    while(1){
        
        TaskStatus_t * taskStats;
        uint32_t taskCount = uxTaskGetNumberOfTasks();
        uint32_t sysTime;
                
        taskStats = pvPortMalloc( taskCount * sizeof( TaskStatus_t ) );
        if(taskStats){
            taskCount = uxTaskGetSystemState(taskStats, taskCount, &sysTime);
            
            TERM_sendVT100Code(handle, _VT100_CURSOR_POS1, 0);
        
            uint32_t cpuLoad = SYS_getCPULoadFine(taskStats, taskCount, sysTime);
            ttprintf("%sbottom - %d\r\n%sTasks: \t%d\r\n%sCPU: \t%d,%d%%\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0), xTaskGetTickCount(), TERM_getVT100Code(_VT100_ERASE_LINE_END, 0), taskCount, TERM_getVT100Code(_VT100_ERASE_LINE_END, 0), cpuLoad / 10, cpuLoad % 10);
            
            uint32_t heapRemaining = xPortGetFreeHeapSize();
            ttprintf("%sMem: \t%db total,\t %db free,\t %db used (%d%%)\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0), configTOTAL_HEAP_SIZE, heapRemaining, configTOTAL_HEAP_SIZE - heapRemaining, ((configTOTAL_HEAP_SIZE - heapRemaining) * 100) / configTOTAL_HEAP_SIZE);
            //taskStats[0].
            ttprintf("%s%s%s", TERM_getVT100Code(_VT100_BACKGROUND_COLOR, _VT100_WHITE), TERM_getVT100Code(_VT100_ERASE_LINE_END, 0), TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_BLACK));
            ttprintf("PID \r\x1b[%dCName \r\x1b[%dCstate \r\x1b[%dC%%Cpu \r\x1b[%dCtime  \r\x1b[%dCStack \r\x1b[%dCHeap\r\n", 6, 7 + configMAX_TASK_NAME_LEN, 20 + configMAX_TASK_NAME_LEN, 27 + configMAX_TASK_NAME_LEN, 38 + configMAX_TASK_NAME_LEN, 45 + configMAX_TASK_NAME_LEN);
            ttprintf("%s", TERM_getVT100Code(_VT100_RESET_ATTRIB, 0));
            
            uint32_t currTask = 0;
            for(;currTask < taskCount; currTask++){
                if(strlen(taskStats[currTask].pcTaskName) != 4 || strcmp(taskStats[currTask].pcTaskName, "IDLE") != 0){
                    char name[configMAX_TASK_NAME_LEN+1];
                    strncpy(name, taskStats[currTask].pcTaskName, configMAX_TASK_NAME_LEN);
                    uint32_t load = (taskStats[currTask].ulRunTimeCounter) / (sysTime/1000);
                    ttprintf("%s%d\r\x1b[%dC%s\r\x1b[%dC%s\r\x1b[%dC%d,%d\r\x1b[%dC%d\r\x1b[%dC%u\r\x1b[%dC%d\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0), taskStats[currTask].xTaskNumber, 6, name, 7 + configMAX_TASK_NAME_LEN
                            , SYS_getTaskStateString(taskStats[currTask].eCurrentState), 20 + configMAX_TASK_NAME_LEN, load / 10, load % 10, 27 + configMAX_TASK_NAME_LEN, taskStats[currTask].ulRunTimeCounter
                            , 38 + configMAX_TASK_NAME_LEN, taskStats[currTask].usStackHighWaterMark, 45 + configMAX_TASK_NAME_LEN, taskStats[currTask].usedHeap);
                }
            }
            vPortFree(taskStats);
        }else{
            ttprintf("Malloc failed\r\n");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

uint8_t CMD_top_handleInput(TERMINAL_HANDLE * handle, uint16_t c){
    switch(c){
        case 'q':
        case 0x03:
            vTaskDelete(handle->currProgram->task);
            vPortFree(handle->currProgram);
            TERM_removeProgramm(handle);
            return TERM_CMD_EXIT_SUCCESS;
        default:
            return TERM_CMD_CONTINUE;
    }
}