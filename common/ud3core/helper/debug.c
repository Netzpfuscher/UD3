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

#include <stdlib.h>

#include "debug.h"
#include "printf.h"
#include "min.h"
#include "min_id.h"
#include "tasks/tsk_min.h"
#include "tasks/tsk_cli.h"

TERMINAL_HANDLE * debug_port;
uint8_t debug_id=0xFF;

#define CTRL_C        0x03
#define DEBUG_LOOP_MS 10

uint8_t print_debug(TERMINAL_HANDLE * handle, uint8_t id, uint8_t fibernet){
    debug_port = handle;
    debug_id = id;
    TERM_sendVT100Code(handle, _VT100_CLS, 0);
    char buffer[80];
    if(fibernet){
        ttprintf("Entering fibernet [CTRL+C] for exit\r\n");
        min_send(id,(uint8_t*)ESC_STR "c",strlen(ESC_STR "c"),DEBUG_LOOP_MS /portTICK_RATE_MS);
    }else{
        ttprintf("Entering debug @%u [CTRL+C] for exit\r\n", id);
    }
    uint8_t c=0;
    while(c != CTRL_C){
        uint8_t len = xStreamBufferReceive(portM->rx,buffer,sizeof(buffer),DEBUG_LOOP_MS /portTICK_RATE_MS);
        for(uint32_t i=0;i<len;i++){
            if(buffer[i]==CTRL_C) c = CTRL_C;   
        }
        if(len) {
            min_send(id,(uint8_t*)buffer,len,DEBUG_LOOP_MS /portTICK_RATE_MS);
        }

    }
    debug_port = NULL;
    debug_id = 0xFF;
    ttprintf("\r\n");
    return 1;
}

uint8_t print_min_debug(TERMINAL_HANDLE * handle){
    TERM_sendVT100Code(handle, _VT100_CLS, 0);
    ttprintf("Entering min debug [CTRL+C] for exit\r\n");
    debug_port = handle;
    min_debug = pdTRUE;
    while(Term_check_break(handle,250));
    min_debug = pdFALSE;
    debug_port = NULL;
    ttprintf("\r\n");
    return 1;
}

uint8_t CMD_debug(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf(   "Usage: debug [id]\r\n"
                    "debug min\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    } 

    if(strcmp(args[0], "min") == 0){
        print_min_debug(handle);
        return TERM_CMD_EXIT_SUCCESS;
	}
    if(strcmp(args[0], "fn") == 0){
        print_debug(handle,MIN_ID_DEBUG,pdTRUE);
        return TERM_CMD_EXIT_SUCCESS;
	}
    uint8_t id = atoi(args[0]);
    print_debug(handle,id,pdFALSE);
    return TERM_CMD_EXIT_SUCCESS;
}


