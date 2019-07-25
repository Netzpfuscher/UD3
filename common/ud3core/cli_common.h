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

#ifndef CLI_COMMON_H
#define CLI_COMMON_H

#include <device.h>
#include "cli_basic.h"

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "tasks/tsk_eth.h"


void nt_interpret(char *text, port_str *ptr);
void init_config();
void eeprom_load(port_str *ptr);

uint8_t command_cls(char *commandline, port_str *ptr);
void stop_overlay_task(port_str *ptr);
void uart_baudrate(uint32_t baudrate);
void update_visibilty(void);

volatile uint8_t qcw_reg;
extern parameter_entry confparam[];

struct config_struct{
    uint8_t watchdog;
    uint16_t max_tr_pw;
    uint16_t max_tr_prf;
    uint16_t max_qcw_pw;
    uint16_t max_tr_current;
    uint16_t min_tr_current;
    uint16_t max_qcw_current;
    uint8_t temp1_max;
    uint8_t temp2_max;
    uint16_t ct1_ratio;
    uint16_t ct2_ratio;
    uint16_t ct3_ratio;
    uint16_t ct1_burden;
    uint16_t ct2_burden;
    uint16_t ct3_burden;
    uint16_t lead_time;
    uint16_t start_freq;
    uint8_t  start_cycles;
    uint16_t max_tr_duty;
    uint16_t max_qcw_duty;
    uint16_t temp1_setpoint;
    uint8_t ps_scheme;
    uint8_t autotune_s;
    char ud_name[16];
    char ip_addr[16];
    char ip_gw[16];
    char ip_mac[18];
    char ip_subnet[16];
    uint8_t minprot;
    uint16_t max_const_i;
    uint16_t max_fault_i;
    uint8_t eth_hw;
    char ssid[16];
    char passwd[20];
    uint32_t baudrate;
    uint8_t ct2_type;
    uint16_t ct2_current;
    uint16_t ct2_voltage;
    uint16_t ct2_offset;
    uint32_t r_top;
    uint16_t chargedelay;
    uint8_t ivo_uart;
};
typedef struct config_struct cli_config;

struct parameter_struct{
    uint16_t    pw;
	uint16_t    pwd;
    uint16_t    tune_start;
    uint16_t    tune_end;
    uint16_t    tune_pw;
    uint16_t    tune_delay;
    uint16_t    offtime;
    uint8_t     qcw_ramp;
    uint16_t    qcw_repeat;
    uint16_t    burst_on;
    uint16_t    burst_off;
    int8_t      transpose;
    uint8_t     synth;
    uint16_t    temp_duty;
    uint8_t     mch;
};
typedef struct parameter_struct cli_parameter;

extern cli_config configuration;
extern cli_parameter param;

#define CONF_SIZE sizeof(confparam) / sizeof(parameter_entry)

#endif