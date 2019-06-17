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

#if !defined(cli_commands_H)
#define cli_commands_H
    
#include <device.h>
#include "cli_basic.h"

uint8_t command_help(char *commandline, port_str *ptr);
uint8_t command_get(char *commandline, port_str *ptr);
uint8_t command_set(char *commandline, port_str *ptr);
uint8_t command_tr(char *commandline, port_str *ptr);
uint8_t command_udkill(char *commandline, port_str *ptr);
uint8_t command_eprom(char *commandline, port_str *ptr);
uint8_t command_status(char *commandline, port_str *ptr);
uint8_t command_tune_p(char *commandline, port_str *ptr);
uint8_t command_tune_s(char *commandline, port_str *ptr);
uint8_t command_tasks(char *commandline, port_str *ptr);
uint8_t command_bootloader(char *commandline, port_str *ptr);
uint8_t command_qcw(char *commandline, port_str *ptr);
uint8_t command_bus(char *commandline, port_str *ptr);
uint8_t command_load_default(char *commandline, port_str *ptr);
uint8_t command_tterm(char *commandline, port_str *ptr);
uint8_t command_reset(char *commandline, port_str *ptr);
uint8_t command_minstat(char *commandline, port_str *ptr);
uint8_t command_config_get(char *commandline, port_str *ptr);
uint8_t command_exit(char *commandline, port_str *ptr);
uint8_t command_ethcon(char *commandline, port_str *ptr);
uint8_t command_fuse(char *commandline, port_str *ptr);
uint8_t command_signals(char *commandline, port_str *ptr);
uint8_t command_alarms(char *commandline, port_str *ptr);


#endif