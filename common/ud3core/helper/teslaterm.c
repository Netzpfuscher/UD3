
#include "teslaterm.h"
#include "cli_basic.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

uint8_t gaugebuf[] = {0xFF,0x04, TT_GAUGE,0x00,0x00,0x00};
uint8_t chartbuf[] = {0xFF,0x04, TT_CHART,0x00,0x00,0x00};
uint8_t statusbuf[] = {0xFF,0x02, TT_STATUS,0x00};
const uint8_t chartdraw[] = {0xFF,0x02, TT_CHART_DRAW,0x00};
const uint8_t chartclear[] = {0xFF,0x02, TT_CHART_CLEAR,0x00};

void send_gauge(uint8_t gauge, int16_t val, uint8_t port){
    gaugebuf[3]=gauge;
    gaugebuf[4]=(uint8_t)val;
    gaugebuf[5]=(uint8_t)(val>>8);
    send_buffer(gaugebuf,sizeof(gaugebuf),port);
}

void send_chart(uint8_t chart, int16_t val, uint8_t port){
    chartbuf[3]=chart;
    chartbuf[4]=(uint8_t)val;
    chartbuf[5]=(uint8_t)(val>>8);
    send_buffer(chartbuf,sizeof(chartbuf),port);
}
void send_chart_draw(uint8_t port){
    send_buffer((uint8_t*)chartdraw,sizeof(chartdraw),port);
}

void send_chart_config(uint8_t chart, int16_t min, int16_t max, int16_t offset,uint8_t unit, char * text, uint8_t port){
    uint8_t bytes = strlen(text);
    uint8_t buf[11];
    buf[0] = 0xFF;
    buf[1] = bytes+sizeof(buf)-2;
    buf[2] = TT_CHART_CONFIG;
    buf[3] = chart;
    buf[4] = min;
    buf[5] = (min>>8);
    buf[6] = max;
    buf[7] = (max>>8);
    buf[8] = offset;
    buf[9] = (offset>>8);
    buf[10] = unit;
    send_buffer(buf,sizeof(buf),port);
    send_string(text, port);
}

void send_gauge_config(uint8_t gauge, int16_t min, int16_t max, char * text, uint8_t port){
    uint8_t bytes = strlen(text);
    uint8_t buf[8];
    buf[0] = 0xFF;
    buf[1] = bytes+6;
    buf[2] = TT_GAUGE_CONF;
    buf[3] = gauge;
    buf[4] = min;
    buf[5] = (min>>8);
    buf[6] = max;
    buf[7] = (max>>8);
    send_buffer(buf,sizeof(buf),port);
    send_string(text, port);
}

void send_chart_clear(uint8_t port){
    send_buffer((uint8_t*)chartclear,sizeof(chartdraw),port); 
}

void send_chart_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t port){
    uint8_t buf[12];
    buf[0] = 0xFF;
    buf[1] = sizeof(buf)-2;
    buf[2] = TT_CHART_LINE;
    buf[3] = x1;
    buf[4] = (x1>>8);
    buf[5] = y1;
    buf[6] = (y1>>8);
    buf[7] = x2;
    buf[8] = (x2>>8);
    buf[9] = y2;
    buf[10] = (y2>>8);
    buf[11] = color;
    send_buffer(buf,sizeof(buf),port); 
}

void send_chart_text(int16_t x, int16_t y, uint8_t color, uint8_t size, char * text, uint8_t port){
    uint8_t bytes = strlen(text);
    uint8_t buf[9];
    buf[0] = 0xFF;
    buf[1] = bytes+sizeof(buf)-2;
    buf[2] = TT_CHART_TEXT;
    buf[3] = x;
    buf[4] = (x>>8);
    buf[5] = y;
    buf[6] = (y>>8);
    buf[7] = color;
    buf[8] = size;
    send_buffer(buf,sizeof(buf),port);
    send_string(text, port);
}

void send_chart_text_center(int16_t x, int16_t y, uint8_t color, uint8_t size, char * text, uint8_t port){
    uint8_t bytes = strlen(text);
    uint8_t buf[9];
    buf[0] = 0xFF;
    buf[1] = bytes+sizeof(buf)-2;
    buf[2] = TT_CHART_TEXT_CENTER;
    buf[3] = x;
    buf[4] = (x>>8);
    buf[5] = y;
    buf[6] = (y>>8);
    buf[7] = color;
    buf[8] = size;
    send_buffer(buf,sizeof(buf),port);
    send_string(text, port);
}

void send_status(uint8_t bus_active, uint8_t transient_active, uint8_t bus_controlled, uint8_t port) {
	statusbuf[3] = bus_active|(transient_active<<1)|(bus_controlled<<2);
    send_buffer(statusbuf, sizeof(statusbuf), port);
}
