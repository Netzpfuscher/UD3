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

#include "alarmevent.h"


/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

xQueueHandle qAlarms;


void alarm_push(uint8_t level, const char* message, int32_t value){
    static uint16_t num=0;
    ALARMS temp;
    if(uxQueueSpacesAvailable(qAlarms)==0){
        xQueueReceive(qAlarms,&temp,0);
    }
    temp.alarm_level = level;
    temp.message = (char*)message;
    temp.num = num;
    temp.value = value;
    temp.timestamp = xTaskGetTickCount() * portTICK_RATE_MS;
    xQueueSend(qAlarms,&temp,0);   
    num++;
}

uint32_t alarm_get_num(){
    return uxQueueMessagesWaiting(qAlarms);    
}

uint32_t alarm_get(uint32_t index, ALARMS * alm){
    if(uxQueueMessagesWaiting(qAlarms)){
       return xQueuePeekIndex(qAlarms, alm, index,0);
    }else{
        return pdFAIL;
    }
}

uint32_t alarm_pop(ALARMS * alm){
    if(uxQueueMessagesWaiting(qAlarms)){
        return xQueueReceive(qAlarms, alm,0);
    }else{
        return pdFAIL;
    }
}

void alarm_clear(){
    xQueueReset(qAlarms);    
}
void alarm_init(){
    qAlarms = xQueueCreate(50, sizeof(ALARMS));
}