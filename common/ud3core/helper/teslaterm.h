/**
 * @file teslaterm.h
 * @brief Teslaterm protocol implementation for telemetry and visualization
 *
 * Implements binary protocol for sending gauge/chart updates, events, and
 * configuration to Teslaterm GUI application. Supports 16-bit and 32-bit
 * data types with configurable units and ranges.
 */

#if !defined(tt_H)
#define tt_H
    
#include <stdint.h>
#include "cli_basic.h"
#include "alarmevent.h"
#include "TTerm.h"

/** @name Teslaterm Message Types
 * Binary protocol message type identifiers
 * @{
 */
#define TT_GAUGE        1    //!< 16-bit gauge value update
#define TT_GAUGE_CONF   2    //!< 16-bit gauge configuration
#define TT_CHART        3    //!< 16-bit chart data point
#define TT_CHART_DRAW   4    //!< Trigger chart redraw
#define TT_CHART_CONFIG 5    //!< 16-bit chart configuration
#define TT_CHART_CLEAR  6    //!< Clear chart and set title
#define TT_CHART_LINE   7    //!< Draw line on chart
#define TT_CHART_TEXT   8    //!< Draw text on chart
#define TT_CHART_TEXT_CENTER 9 //!< Draw centered text on chart
#define TT_STATUS       10   //!< System status update
#define TT_CONFIG_GET   11   //!< Configuration parameter transfer
#define TT_EVENT        12   //!< Alarm/event notification
#define TT_GAUGE32      13   //!< 32-bit gauge value update
#define TT_GAUGE_CONF32 14   //!< 32-bit gauge configuration
#define TT_FEATURE_GET  15   //!< Feature list transfer
#define TT_CHART32      16   //!< 32-bit chart data point
#define TT_CHART32_CONF 17   //!< 32-bit chart configuration
/** @} */

/** @name Unit Type Identifiers
 * Unit codes for gauge and chart display
 * @{
 */
#define TT_UNIT_NONE    0    //!< No unit
#define TT_UNIT_V       1    //!< Volts
#define TT_UNIT_A       2    //!< Amperes
#define TT_UNIT_W       3    //!< Watts
#define TT_UNIT_Hz      4    //!< Hertz
#define TT_UNIT_C       5    //!< Celsius
#define TT_UNIT_kW      6    //!< Kilowatts
#define TT_UNIT_RPM     7    //!< Revolutions per minute
#define TT_UNIT_PERCENT 8    //!< Percentage
#define TT_UNIT_mV      9    //!< Millivolts
/** @} */

extern const char *units[]; //!< Unit string lookup table

/** @name Color Identifiers
 * Color codes for chart drawing
 * @{
 */
#define TT_COLOR_WHITE 0  //!< White
#define TT_COLOR_RED 1    //!< Red
#define TT_COLOR_BLUE 2   //!< Blue
#define TT_COLOR_GREEN 3  //!< Green
#define TT_COLOR_GRAY 8   //!< Gray
/** @} */

/**
 * @brief Chart geometry configuration
 *
 * Defines display dimensions and grid spacing for chart rendering
 */
typedef struct __chart__ {
    uint16_t height;   //!< Chart height in pixels
    uint16_t width;    //!< Chart width in pixels
    uint16_t offset_x; //!< X offset from screen edge
    uint16_t offset_y; //!< Y offset from screen edge
    uint16_t div_x;    //!< X-axis grid division spacing
    uint16_t div_y;    //!< Y-axis grid division spacing
} CHART;



/**
 * @brief Send 16-bit gauge value update
 * @param gauge Gauge ID (0-255)
 * @param val Gauge value
 * @param handle Terminal handle (unused, for API compatibility)
 */
void send_gauge(uint8_t gauge, int16_t val, TERMINAL_HANDLE * handle);

/**
 * @brief Send 32-bit gauge value update
 * @param gauge Gauge ID (0-255)
 * @param val Gauge value
 * @param handle Terminal handle (unused, for API compatibility)
 */
void send_gauge32(uint8_t gauge, int32_t val, TERMINAL_HANDLE * handle);

/**
 * @brief Send 16-bit chart data point
 * @param chart Chart ID (0-255)
 * @param val Data value
 * @param handle Terminal handle (unused)
 */
void send_chart(uint8_t chart, int16_t val, TERMINAL_HANDLE * handle);

/**
 * @brief Send 32-bit chart data point
 * @param chart Chart ID (0-255)
 * @param val Data value
 * @param handle Terminal handle (unused)
 */
void send_chart32(uint8_t chart, int32_t val, TERMINAL_HANDLE * handle);

/**
 * @brief Trigger chart redraw
 * @param handle Terminal handle (unused)
 */
void send_chart_draw(TERMINAL_HANDLE * handle);

/**
 * @brief Configure 16-bit chart parameters
 * @param chart Chart ID
 * @param min Minimum value
 * @param max Maximum value
 * @param offset Y-axis offset
 * @param unit Unit type (TT_UNIT_*)
 * @param text Chart label text
 * @param handle Terminal handle (unused)
 */
void send_chart_config(uint8_t chart, int16_t min, int16_t max, int16_t offset, uint8_t unit,char * text, TERMINAL_HANDLE * handle);

/**
 * @brief Configure 32-bit chart parameters
 * @param chart Chart ID
 * @param min Minimum value
 * @param max Maximum value
 * @param offset Y-axis offset
 * @param div Division factor for display scaling
 * @param unit Unit type (TT_UNIT_*)
 * @param text Chart label text
 * @param handle Terminal handle (unused)
 */
void send_chart_config32(uint8_t chart, int32_t min, int32_t max, int32_t offset, int32_t div, uint8_t unit, char * text, TERMINAL_HANDLE * handle);

/**
 * @brief Configure 16-bit gauge parameters
 * @param gauge Gauge ID
 * @param min Minimum value
 * @param max Maximum value
 * @param text Gauge label text
 * @param handle Terminal handle (unused)
 */
void send_gauge_config(uint8_t gauge, int16_t min, int16_t max, char * text, TERMINAL_HANDLE * handle);

/**
 * @brief Configure 32-bit gauge parameters
 * @param gauge Gauge ID
 * @param min Minimum value
 * @param max Maximum value
 * @param div Division factor for display scaling
 * @param text Gauge label text
 * @param handle Terminal handle (unused)
 */
void send_gauge_config32(uint8_t gauge, int32_t min, int32_t max, int32_t div, char * text, TERMINAL_HANDLE * handle);

/**
 * @brief Draw text on chart
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Color (TT_COLOR_*)
 * @param size Font size
 * @param text Text string
 * @param handle Terminal handle (unused)
 */
void send_chart_text(int16_t x, int16_t y, uint8_t color, uint8_t size, char * text, TERMINAL_HANDLE * handle);

/**
 * @brief Draw centered text on chart
 * @param x Center X coordinate
 * @param y Center Y coordinate
 * @param color Color (TT_COLOR_*)
 * @param size Font size
 * @param text Text string
 * @param handle Terminal handle (unused)
 */
void send_chart_text_center(int16_t x, int16_t y, uint8_t color, uint8_t size, char * text, TERMINAL_HANDLE * handle);

/**
 * @brief Draw line on chart
 * @param x1 Start X coordinate
 * @param y1 Start Y coordinate
 * @param x2 End X coordinate
 * @param y2 End Y coordinate
 * @param color Line color (TT_COLOR_*)
 * @param handle Terminal handle (unused)
 */
void send_chart_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, TERMINAL_HANDLE * handle);

/**
 * @brief Clear chart and set title
 * @param handle Terminal handle (unused)
 * @param title Chart title string
 */
void send_chart_clear(TERMINAL_HANDLE * handle, char * title);
/**
 * @brief Send system status update
 * @param bus_active Bus voltage active flag
 * @param transient_active Transient mode active flag
 * @param bus_controlled Bus voltage controlled flag
 * @param killbit Interrupter kill status
 * @param handle Terminal handle (unused)
 */
void send_status(uint8_t bus_active, uint8_t transient_active, uint8_t bus_controlled,uint8_t killbit ,TERMINAL_HANDLE * handle);

/**
 * @brief Send configuration parameter info
 * @param param Parameter name
 * @param help_text Parameter help text
 * @param handle Terminal handle (unused)
 */
void send_config(char* param, const char* help_text, TERMINAL_HANDLE * handle);

/**
 * @brief Send alarm/event notification
 * @param alm Alarm structure pointer
 * @param handle Terminal handle (unused)
 */
void send_event(ALARMS *alm, TERMINAL_HANDLE * handle);

/**
 * @brief Send feature list
 * @param text Feature description string
 * @param handle Terminal handle (unused)
 */
void send_features(const char* text, TERMINAL_HANDLE * handle);

/**
 * @brief Initialize chart with grid and axes
 * @param chart Chart configuration structure
 * @param handle Terminal handle (unused)
 *
 * Draws X/Y axes, grid lines, and labels based on chart geometry
 */
void tt_chart_init(CHART *chart, TERMINAL_HANDLE * handle);

#endif
