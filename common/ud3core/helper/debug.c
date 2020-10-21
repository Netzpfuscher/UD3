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

port_str *debug_port;

uint8_t print_debug(port_str *ptr){
    debug_port = ptr;
    Term_Erase_Screen(ptr);
    SEND_CONST_STRING("Entering debug [q] for exit\r\n",ptr);
    while(Term_check_break(ptr,250)){

    }
    debug_port = NULL;
    SEND_CONST_STRING("\r\n",ptr);
    return 1;
}

uint8_t command_debug(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline); 
    CHECK_NULL(commandline);
    if (ntlibc_stricmp(commandline, "start") == 0) {
        print_debug(ptr);
        return 0;
	}
    HELP_TEXT("Usage: debug [start]\r\n");
}


