/*
 * TTerm
 *
 * Copyright (c) 2020 Thorben Zethoff, Jens Kerrinnes
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

#include "top.h"
#include "string.h"
#include "system.h"

#define APP_NAME "top"
#define APP_DESCRIPTION "shows performance stats"
#define APP_STACK 250

typedef enum {
	SORT_PID = 0,
	SORT_NAME,
	SORT_STATE,
	SORT_CPU,
	SORT_TIME,
	SORT_STACK,
	SORT_HEAP
} sort_column_t;

typedef struct {
	sort_column_t sort_column;
	uint8_t sort_ascending;
} top_state_t;

uint8_t CMD_main(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
void TASK_main(void *pvParameters);
uint8_t INPUT_handler(TERMINAL_HANDLE * handle, uint16_t c);
int task_compare(const void *a, const void *b, void *state);

uint8_t REGISTER_top(TermCommandDescriptor * desc){
    TERM_addCommand(CMD_main, APP_NAME, APP_DESCRIPTION, 0, desc); 
    return pdTRUE;
}

uint8_t CMD_main(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    
    uint8_t currArg = 0;
    uint8_t returnCode = TERM_CMD_EXIT_SUCCESS;
    char ** cpy_args=NULL;
    argCount++;
    if(argCount){
        cpy_args = pvPortMalloc(sizeof(char*)*argCount);
        cpy_args[0] = pvPortMalloc(sizeof(APP_NAME));
        cpy_args[0]=memcpy(cpy_args[0], APP_NAME, sizeof(APP_NAME));
        for(;currArg<argCount-1; currArg++){
            uint16_t len = strlen(args[currArg])+1;
            cpy_args[currArg+1] = pvPortMalloc(len);
            memcpy(cpy_args[currArg+1], args[currArg], len);
        }
    }
    TermProgram * prog = pvPortMalloc(sizeof(TermProgram));
    prog->inputHandler = INPUT_handler;
    prog->args = cpy_args;
    prog->argCount = argCount;
    
    top_state_t * state = pvPortMalloc(sizeof(top_state_t));
    state->sort_column = SORT_CPU;
    state->sort_ascending = 0;
    prog->userData = state;
    TERM_sendVT100Code(handle, _VT100_RESET, 0); TERM_sendVT100Code(handle, _VT100_CURSOR_POS1, 0);
    returnCode = xTaskCreate(TASK_main, APP_NAME, APP_STACK, handle, tskIDLE_PRIORITY + 1, &prog->task) ? TERM_CMD_EXIT_PROC_STARTED : TERM_CMD_EXIT_ERROR;
    if(returnCode == TERM_CMD_EXIT_PROC_STARTED) TERM_attachProgramm(handle, prog);
    return returnCode;
}

int task_compare(const void *a, const void *b, void *state) {
	const TaskStatus_t *task_a = (const TaskStatus_t *)a;
	const TaskStatus_t *task_b = (const TaskStatus_t *)b;
	top_state_t *sort_state = (top_state_t *)state;
	int result = 0;
	
	switch(sort_state->sort_column) {
		case SORT_PID:
			result = task_a->xTaskNumber - task_b->xTaskNumber;
			break;
		case SORT_NAME:
			result = strcmp(task_a->pcTaskName, task_b->pcTaskName);
			break;
		case SORT_STATE:
			result = task_a->eCurrentState - task_b->eCurrentState;
			break;
		case SORT_CPU:
			result = task_b->ulRunTimeCounter - task_a->ulRunTimeCounter;
			break;
		case SORT_TIME:
			result = task_b->ulRunTimeCounter - task_a->ulRunTimeCounter;
			break;
		case SORT_STACK:
			result = task_b->usStackHighWaterMark - task_a->usStackHighWaterMark;
			break;
		case SORT_HEAP:
			result = task_b->usedHeap - task_a->usedHeap;
			break;
	}
	
	if (!sort_state->sort_ascending) {
		result = -result;
	}
	
	return result;
}

void qsort_r_impl(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *, void *), void *arg) {
	TaskStatus_t *arr = (TaskStatus_t *)base;
	for (size_t i = 0; i < nmemb - 1; i++) {
		for (size_t j = 0; j < nmemb - i - 1; j++) {
			if (compar(&arr[j], &arr[j + 1], arg) > 0) {
				TaskStatus_t temp = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = temp;
			}
		}
	}
}

void TASK_main(void *pvParameters){
    TERMINAL_HANDLE * handle = (TERMINAL_HANDLE*)pvParameters;
    top_state_t * state = (top_state_t *)handle->currProgram->userData;
    char c=0;
    do{
        
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
            if(configTOTAL_HEAP_SIZE > 0){
                ttprintf("%sMem: \t%db total,\t %db free,\t %db used (%d%%)\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0), configTOTAL_HEAP_SIZE, heapRemaining, configTOTAL_HEAP_SIZE - heapRemaining, ((configTOTAL_HEAP_SIZE - heapRemaining) * 100) / configTOTAL_HEAP_SIZE);
            }else{
                ttprintf("%sMem: \t%db total,\t %db free,\t %db used\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0), configTOTAL_HEAP_SIZE, heapRemaining, configTOTAL_HEAP_SIZE - heapRemaining);
            }
            ttprintf("%sSort: %s %s  (p/n/s/c/t/k/h to change, r to reverse)\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0),
                state->sort_column == SORT_PID ? "PID" :
                state->sort_column == SORT_NAME ? "Name" :
                state->sort_column == SORT_STATE ? "State" :
                state->sort_column == SORT_CPU ? "CPU" :
                state->sort_column == SORT_TIME ? "Time" :
                state->sort_column == SORT_STACK ? "Stack" : "Heap",
                state->sort_ascending ? "(asc)" : "(desc)");
            ttprintf("%s%s%s", TERM_getVT100Code(_VT100_BACKGROUND_COLOR, _VT100_WHITE), TERM_getVT100Code(_VT100_ERASE_LINE_END, 0), TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_BLACK));
            ttprintf("PID \r\x1b[%dCName \r\x1b[%dCstate \r\x1b[%dC%%Cpu \r\x1b[%dCtime  \r\x1b[%dCStack \r\x1b[%dCHeap\r\n", 6, 7 + configMAX_TASK_NAME_LEN, 20 + configMAX_TASK_NAME_LEN, 27 + configMAX_TASK_NAME_LEN, 38 + configMAX_TASK_NAME_LEN, 45 + configMAX_TASK_NAME_LEN);
            ttprintf("%s", TERM_getVT100Code(_VT100_RESET_ATTRIB, 0));
            
            qsort_r_impl(taskStats, taskCount, sizeof(TaskStatus_t), task_compare, state);
            
            uint32_t currTask = 0;
            for(;currTask < taskCount; currTask++){
                if(strlen(taskStats[currTask].pcTaskName) != 4 || strcmp(taskStats[currTask].pcTaskName, "IDLE") != 0){
                    char name[configMAX_TASK_NAME_LEN+1];
                    strncpy(name, taskStats[currTask].pcTaskName, configMAX_TASK_NAME_LEN);
                     uint32_t load=0;
                    if(sysTime>1000){
                        uint32_t sysTimeSec = sysTime/configTICK_RATE_HZ;
                        if(sysTimeSec > 0){
                            load = (taskStats[currTask].ulRunTimeCounter) / sysTimeSec;
                        }
                    }
                    ttprintf("%s%d\r\x1b[%dC%s\r\x1b[%dC%s\r\x1b[%dC%d,%d\r\x1b[%dC%d\r\x1b[%dC%u\r\x1b[%dC%d\r\n", TERM_getVT100Code(_VT100_ERASE_LINE_END, 0), taskStats[currTask].xTaskNumber, 6, name, 7 + configMAX_TASK_NAME_LEN
                            , SYS_getTaskStateString(taskStats[currTask].eCurrentState), 20 + configMAX_TASK_NAME_LEN, load / 10, load % 10, 27 + configMAX_TASK_NAME_LEN, taskStats[currTask].ulRunTimeCounter
                            , 38 + configMAX_TASK_NAME_LEN, taskStats[currTask].usStackHighWaterMark, 45 + configMAX_TASK_NAME_LEN, taskStats[currTask].usedHeap);
                }
            }
            vPortFree(taskStats);
        }else{
            ttprintf("Malloc failed\r\n");
        }
        
        xStreamBufferReceive(handle->currProgram->inputStream,&c,sizeof(c),pdMS_TO_TICKS(1000));
    }while(c!=CTRL_C);
    TERM_killProgramm(handle);
}

uint8_t INPUT_handler(TERMINAL_HANDLE * handle, uint16_t c){
    if(handle->currProgram->inputStream==NULL) return TERM_CMD_EXIT_SUCCESS;
    top_state_t * state = (top_state_t *)handle->currProgram->userData;
    switch(c){
        case 'q':
        case CTRL_C:
            c=CTRL_C;
            xStreamBufferSend(handle->currProgram->inputStream,&c,1,20);
            vPortFree(state);
            return TERM_CMD_EXIT_SUCCESS;
        case 'p':
            state->sort_column = SORT_PID;
            return TERM_CMD_CONTINUE;
        case 'n':
            state->sort_column = SORT_NAME;
            return TERM_CMD_CONTINUE;
        case 's':
            state->sort_column = SORT_STATE;
            return TERM_CMD_CONTINUE;
        case 'c':
            state->sort_column = SORT_CPU;
            return TERM_CMD_CONTINUE;
        case 't':
            state->sort_column = SORT_TIME;
            return TERM_CMD_CONTINUE;
        case 'k':
            state->sort_column = SORT_STACK;
            return TERM_CMD_CONTINUE;
        case 'h':
            state->sort_column = SORT_HEAP;
            return TERM_CMD_CONTINUE;
        case 'r':
            state->sort_ascending = !state->sort_ascending;
            return TERM_CMD_CONTINUE;
        default:
            return TERM_CMD_CONTINUE;
    }
}