
#include "teslaterm.h"
#include "cli_basic.h"
#include <string.h>

uint8_t gaugebuf[] = {0xFF,0x04, TT_GAUGE,0x00,0x00,0x00};
uint8_t chartbuf[] = {0xFF,0x04, TT_CHART,0x00,0x00,0x00};
uint8_t chartdraw[] = {0xFF,0x02, TT_CHART_DRAW,0x00};

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
    send_buffer(chartdraw,sizeof(chartdraw),port);
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
    send_buffer(buf,sizeof(buf),SERIAL);
    send_string(text, SERIAL);
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
    send_buffer(buf,8,SERIAL);
    send_string(text, SERIAL);
}
