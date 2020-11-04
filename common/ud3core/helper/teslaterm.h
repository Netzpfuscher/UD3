#if !defined(tt_H)
#define tt_H
    
#include <stdint.h>
#include "cli_basic.h"
#include "alarmevent.h"
#include "TTerm.h"

#define TT_GAUGE        1
#define TT_GAUGE_CONF   2
#define TT_CHART        3
#define TT_CHART_DRAW   4
#define TT_CHART_CONFIG 5
#define TT_CHART_CLEAR  6
#define TT_CHART_LINE   7
#define TT_CHART_TEXT   8
#define TT_CHART_TEXT_CENTER 9
#define TT_STATUS       10
#define TT_CONFIG_GET   11
#define TT_EVENT        12
#define TT_GAUGE32      13
#define TT_GAUGE_CONF32 14
#define TT_FEATURE_GET  15

#define TT_UNIT_NONE    0
#define TT_UNIT_V       1
#define TT_UNIT_A       2
#define TT_UNIT_W       3
#define TT_UNIT_Hz      4
#define TT_UNIT_C       5
#define TT_UNIT_kW      6
#define TT_UNIT_RPM     7
#define TT_UNIT_PERCENT 8
#define TT_UNIT_mV      9

extern const char *units[];

#define TT_COLOR_WHITE 0
#define TT_COLOR_RED 1
#define TT_COLOR_BLUE 2
#define TT_COLOR_GREEN 3
#define TT_COLOR_GRAY 8

typedef struct __chart__ {
    uint16_t height;
    uint16_t width;
    uint16_t offset_x;
    uint16_t offset_y;
    uint16_t div_x;
    uint16_t div_y;
} CHART;



void send_gauge(uint8_t gauge, int16_t val, TERMINAL_HANDLE * handle);
void send_gauge32(uint8_t gauge, int32_t val, TERMINAL_HANDLE * handle);

void send_chart(uint8_t chart, int16_t val, TERMINAL_HANDLE * handle);
void send_chart_draw(TERMINAL_HANDLE * handle);
void send_chart_config(uint8_t chart, int16_t min, int16_t max, int16_t offset, uint8_t unit,char * text, TERMINAL_HANDLE * handle);
void send_gauge_config(uint8_t gauge, int16_t min, int16_t max, char * text, TERMINAL_HANDLE * handle);
void send_gauge_config32(uint8_t gauge, int32_t min, int32_t max, int32 div, char * text, TERMINAL_HANDLE * handle);
void send_chart_text(int16_t x, int16_t y, uint8_t color, uint8_t size, char * text, TERMINAL_HANDLE * handle);
void send_chart_text_center(int16_t x, int16_t y, uint8_t color, uint8_t size, char * text, TERMINAL_HANDLE * handle);
void send_chart_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, TERMINAL_HANDLE * handle);
void send_chart_clear(TERMINAL_HANDLE * handle);
void send_status(uint8_t bus_active, uint8_t transient_active, uint8_t bus_controlled,uint8_t killbit ,TERMINAL_HANDLE * handle);
void send_config(char* param, const char* help_text, TERMINAL_HANDLE * handle);
void send_event(ALARMS *alm, TERMINAL_HANDLE * handle);
void send_features(const char* text, TERMINAL_HANDLE * handle);

void tt_chart_init(CHART *chart, TERMINAL_HANDLE * handle);

#endif