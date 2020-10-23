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

#include "debug.h"
#include "ntlibc.h"
#include "printf.h"
#include "min.h"
#include "tasks/tsk_min.h"

port_str *debug_port;
uint8_t debug_id=0xFF;

#define CTRL_C        0x03
#define DEBUG_LOOP_MS 10

uint8_t print_debug(port_str *ptr, uint8_t id){
    debug_port = ptr;
    debug_id = id;
    Term_Erase_Screen(ptr);
    char buffer[80];
    uint8_t ret = snprintf(buffer,sizeof(buffer),"Entering debug @%u [CTRL+C] for exit\r\n", id);
    send_buffer(buffer,ret,ptr);
    uint8_t c=0;
    while(c != CTRL_C){
        xSemaphoreGive(ptr->term_block);
        uint8_t len = xStreamBufferReceive(ptr->rx,buffer,sizeof(buffer),DEBUG_LOOP_MS /portTICK_RATE_MS);
        xSemaphoreTake(ptr->term_block, portMAX_DELAY);
        for(uint32_t i=0;i<len;i++){
            if(buffer[i]==CTRL_C) c = CTRL_C;   
        }
        min_send_frame(&min_ctx,id,(uint8_t*)buffer,len);

    }
    debug_port = NULL;
    debug_id = 0xFF;
    SEND_CONST_STRING("\r\n",ptr);
    return 1;
}

uint8_t print_min_debug(port_str *ptr){
    Term_Erase_Screen(ptr);
    SEND_CONST_STRING("Entering min debug [CTRL+C] for exit\r\n",ptr)
    debug_port = ptr;
    min_debug = pdTRUE;
    while(Term_check_break(ptr,250));
    min_debug = pdFALSE;
    debug_port = NULL;
    SEND_CONST_STRING("\r\n",ptr);
    return 1;
}

uint8_t command_debug(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline); 
    CHECK_NULL(commandline);
    if (ntlibc_stricmp(commandline, "min") == 0) {
        print_min_debug(ptr);
        return 0;
	}
    uint8_t id = ntlibc_atoi(commandline);
    print_debug(ptr,id);
    return 0;
    
    HELP_TEXT("Usage: debug [id]\r\n"
              "debug min\r\n");
}


