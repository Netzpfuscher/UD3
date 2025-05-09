
#include "teslaterm.h"

#include "cli_basic.h"
#include "cli_common.h"
#include "printf.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

uint8_t gaugebuf[] = {0xFF,0x04, TT_GAUGE,0x00,0x00,0x00};
uint8_t buf32[] = {0xFF,0x06, TT_GAUGE32,0x00,0x00,0x00,0x00,0x00};
uint8_t chartbuf[] = {0xFF,0x04, TT_CHART,0x00,0x00,0x00};
const uint8_t chartdraw[] = {0xFF,0x02, TT_CHART_DRAW,0x00};

const char *units[]={
    "",
    "V",
    "A",
    "W",
    "Hz",
    "C",
    "kW",
    "RPM",
    "%",
    "mV"
    
};

void send_gauge(uint8_t gauge, int16_t val, TERMINAL_HANDLE * handle){
    gaugebuf[3]=gauge;
    gaugebuf[4]=(uint8_t)val;
    gaugebuf[5]=(uint8_t)(val>>8);
    ttprintb(gaugebuf,sizeof(gaugebuf));
}

void send_gauge32(uint8_t gauge, int32_t val, TERMINAL_HANDLE * handle){
    buf32[2]=TT_GAUGE32;
    buf32[3]=gauge;
    buf32[4]=(uint8_t)val;
    buf32[5]=(uint8_t)(val>>8);
    buf32[6]=(uint8_t)(val>>16);
    buf32[7]=(uint8_t)(val>>24);
    ttprintb(buf32,sizeof(buf32));
}

void send_chart(uint8_t chart, int16_t val, TERMINAL_HANDLE * handle){
    chartbuf[3]=chart;
    chartbuf[4]=(uint8_t)val;
    chartbuf[5]=(uint8_t)(val>>8);
    ttprintb(chartbuf,sizeof(chartbuf));
}

void send_chart32(uint8_t chart, int32_t val, TERMINAL_HANDLE * handle){
    buf32[2]=TT_CHART32;
    buf32[3]=chart;
    buf32[4]=(uint8_t)val;
    buf32[5]=(uint8_t)(val>>8);
    buf32[6]=(uint8_t)(val>>16);
    buf32[7]=(uint8_t)(val>>24);
    ttprintb(buf32,sizeof(buf32));
}


void send_chart_draw(TERMINAL_HANDLE * handle){
    ttprintb(chartdraw,sizeof(chartdraw));
}

void send_chart_config(uint8_t chart, int16_t min, int16_t max, int16_t offset,uint8_t unit, char * text, TERMINAL_HANDLE * handle){
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
    ttprintb(buf,sizeof(buf));
    ttprintb(text,bytes);
}

void send_chart_config32(uint8_t chart, int32_t min, int32_t max, int32_t offset, int32_t div, uint8_t unit, char * text, TERMINAL_HANDLE * handle){
    uint8_t bytes = strlen(text);
    uint8_t buf[21];
    buf[0] = 0xFF;
    buf[1] = bytes+sizeof(buf)-2;
    buf[2] = TT_CHART32_CONF;
    buf[3] = chart;
    buf[4] = (uint8_t)min;
    buf[5] = (uint8_t)(min>>8);
    buf[6] = (uint8_t)(min>>16);
    buf[7] = (uint8_t)(min>>24);
    buf[8] = (uint8_t)max;
    buf[9] = (uint8_t)(max>>8);
    buf[10] = (uint8_t)(max>>16);
    buf[11] = (uint8_t)(max>>24);
    buf[12] = (uint8_t)offset;
    buf[13] = (uint8_t)(offset>>8);
    buf[14] = (uint8_t)(offset>>16);
    buf[15] = (uint8_t)(offset>>24);
    buf[16] = (uint8_t)div;
    buf[17] = (uint8_t)(div>>8);
    buf[18] = (uint8_t)(div>>16);
    buf[19] = (uint8_t)(div>>24);
    buf[20] = unit;
    ttprintb(buf,sizeof(buf));
    ttprintb(text,bytes);
}

void send_gauge_config(uint8_t gauge, int16_t min, int16_t max, char * text, TERMINAL_HANDLE * handle){
    uint8_t bytes = strlen(text);
    uint8_t buf[8];
    buf[0] = 0xFF;
    buf[1] = bytes+sizeof(buf)-2;
    buf[2] = TT_GAUGE_CONF;
    buf[3] = gauge;
    buf[4] = min;
    buf[5] = (min>>8);
    buf[6] = max;
    buf[7] = (max>>8);
    ttprintb(buf,sizeof(buf));
    ttprintb(text,bytes);
}

void send_gauge_config32(uint8_t gauge, int32_t min, int32_t max, int32_t div ,char * text, TERMINAL_HANDLE * handle){
    uint8_t bytes = strlen(text);
    uint8_t buf[16];
    buf[0] = 0xFF;
    buf[1] = bytes+sizeof(buf)-2;
    buf[2] = TT_GAUGE_CONF32;
    buf[3] = gauge;
    buf[4] = min;
    buf[5] = (min>>8);
    buf[6] = (min>>16);
    buf[7] = (min>>24);
    buf[8] = max;
    buf[9] = (max>>8);
    buf[10] = (max>>16);
    buf[11] = (max>>24);
    buf[12] = div;
    buf[13] = (div>>8);
    buf[14] = (div>>16);
    buf[15] = (div>>24);
    ttprintb(buf,sizeof(buf));
    ttprintb(text,bytes);
}

void send_chart_clear(TERMINAL_HANDLE * handle, char * title){
    uint8_t chartclear[3] = {0xFF,0x00, TT_CHART_CLEAR};
    chartclear[1] = strlen(title)+sizeof(chartclear)-2;
    ttprintb(chartclear,sizeof(chartclear)); 
    ttprintf("%s", title);
}

void send_chart_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, TERMINAL_HANDLE * handle){
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
    ttprintb(buf,sizeof(buf)); 
}

void send_chart_text(int16_t x, int16_t y, uint8_t color, uint8_t size, char * text, TERMINAL_HANDLE * handle){
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
    ttprintb(buf,sizeof(buf));
    ttprintf("%s",text);
}

void send_config(char* param,const char* help_text, TERMINAL_HANDLE * handle){
    uint8_t len = strlen(param);
    len += strlen(help_text);
    len++;
    uint8_t buf[3];
    buf[0] = 0xFF;
    buf[1] = len+sizeof(buf)-2;
    buf[2] = TT_CONFIG_GET;
    ttprintb(buf,sizeof(buf));
    ttprintf("%s;%s",param,(char*)help_text);
}

void send_features(const char* text, TERMINAL_HANDLE * handle){
    uint8_t len = strlen(text);
    uint8_t buf[3];
    buf[0] = 0xFF;
    buf[1] = len+sizeof(buf)-2;
    buf[2] = TT_FEATURE_GET;
    ttprintb(buf,sizeof(buf));
    ttprintf("%s",text);
}

void send_event(ALARMS *alm, TERMINAL_HANDLE * handle){
    uint8_t buf[100];
    buf[0] = 0xFF;
    buf[2] = TT_EVENT;
    if(alm->value==ALM_NO_VALUE){
        buf[1] = snprintf((char*)&buf[3],sizeof(buf)-3,"%u;%u;%s;NULL",alm->alarm_level,alm->timestamp,alm->message)+1;
    }else{
        buf[1] = snprintf((char*)&buf[3],sizeof(buf)-3,"%u;%u;%s;%u",alm->alarm_level,alm->timestamp,alm->message,alm->value)+1;
    }
    ttprintb(buf,buf[1]+2);
}

void send_chart_text_center(int16_t x, int16_t y, uint8_t color, uint8_t size, char * text, TERMINAL_HANDLE * handle){
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
    ttprintb(buf,sizeof(buf));
    ttprintf("%s",text);
}



void send_status(uint8_t bus_active, uint8_t transient_active, uint8_t bus_controlled,uint8_t killbit, TERMINAL_HANDLE * handle) {
    uint8_t buf[8];
    buf[0] = 0xFF;
    buf[1] = sizeof(buf)-2;
    buf[2] = TT_STATUS;
	buf[3] = bus_active|(transient_active<<1)|(bus_controlled<<2)|(killbit<<3)|(configuration.is_qcw<<4);
	buf[4] = configuration.max_tr_pw;
	buf[5] = configuration.max_tr_pw >> 8;
	buf[6] = configuration.max_tr_prf;
	buf[7] = configuration.max_tr_prf >> 8;
    ttprintb(buf, sizeof(buf));
}



void tt_chart_init(CHART *chart, TERMINAL_HANDLE * handle){
    
    int16_t f;
    char buffer[10];
    send_chart_line(chart->offset_x, chart->height+chart->offset_y, chart->width+chart->offset_x, chart->height+chart->offset_y, TT_COLOR_WHITE,handle);
    send_chart_line(chart->offset_x, chart->offset_y, chart->offset_x, chart->height+chart->offset_y, TT_COLOR_WHITE, handle);

    //Y grid
	for (f = chart->height; f >= 0; f -= chart->div_y) {
		send_chart_line(chart->offset_x, f+chart->offset_y, chart->offset_x-10, f+chart->offset_y, TT_COLOR_WHITE, handle);
        send_chart_line(chart->offset_x, f+chart->offset_y, chart->width+chart->offset_x, f+chart->offset_y, TT_COLOR_GRAY, handle);
        
        snprintf(buffer, sizeof(buffer), "%i", chart->height-f);
    	send_chart_text_center(chart->offset_x-20,f+chart->offset_y+4,TT_COLOR_WHITE,8,buffer, handle);
	}
    
    //X grid
	for (f = 0; f <= chart->width; f += chart->div_x) {
		send_chart_line(f+chart->offset_x, chart->height+chart->offset_y, f+chart->offset_x, chart->height+chart->offset_y+10, TT_COLOR_WHITE, handle);
        send_chart_line(f+chart->offset_x, chart->offset_y, f+chart->offset_x, chart->offset_y+chart->height, TT_COLOR_GRAY, handle);
        
        snprintf(buffer, sizeof(buffer), "%i", f);
    	send_chart_text_center(f+chart->offset_x,chart->offset_y+chart->height+20,TT_COLOR_WHITE,8,buffer, handle);
	}
   
    

}
