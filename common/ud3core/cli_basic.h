#ifndef CLI_BASIC_H
#define CLI_BASIC_H

#include <stdint.h>
#include "config.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "stream_buffer.h" 
#include "task.h"
#include "TTerm.h"
    
#define ESC_STR "\x1b"
   
    
#define set_bit(var, bit) ((var) |= (1 << (bit)))

/* Bit löschen */
#define clear_bit(var, bit) ((var) &= (unsigned)~(1 << (bit)))

/* Bit togglen */
#define toggle_bit(var,bit) ((var) ^= (1 << (bit)))

/* Bit abfragen */
#define bit_is_set(var, bit) ((var) & (1 << (bit)))
#define bit_is_clear(var, bit) !bit_is_set(var, bit)

#define PARAM_SIZE(param) sizeof(param) / sizeof(parameter_entry)

enum cli_types{
    TYPE_UNSIGNED,
    TYPE_SIGNED,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_STRING,
    TYPE_BUFFER
};

enum port{
    PORT_TYPE_NULL,
    PORT_TYPE_SERIAL,
    PORT_TYPE_USB,
    PORT_TYPE_MIN,
    PORT_TERM_VT100,
    PORT_TERM_TT,
    PORT_TERM_MQTT
};
    
#define typename(x) _Generic((x), \
    uint8_t:    TYPE_UNSIGNED, \
    uint16_t:   TYPE_UNSIGNED, \
    uint32_t:   TYPE_UNSIGNED, \
    int8_t:     TYPE_SIGNED, \
    int16_t:    TYPE_SIGNED, \
    int32_t:    TYPE_SIGNED, \
    float:      TYPE_FLOAT, \
    uint16_t*:  TYPE_BUFFER, \
    char:       TYPE_CHAR, \
    char*:      TYPE_STRING)

/*
#define send_buffer(data,len,ptr) _Generic((data), \
                uint8_t*        : send_buffer_u8, \
                char*           : send_buffer_c,  \
                const char*     : send_buffer_c,  \
                const uint8_t*  : send_buffer_u8  \
)(data,len,ptr)*/

#define SIZEP(x) ((char*)(&(x) + 1) - (char*)&(x))
#define ADD_PARAM(para_type, visible,text, value_var, min, max, div, update_func, help_text) {para_type, visible,text, &value_var, SIZEP(value_var), typename(value_var),min, max, div, update_func, help_text},
#define ADD_COMMAND(command, command_func, help_text) {command, command_func, help_text},

    
#define PARAM_DEFAULT   0
#define PARAM_CONFIG    1

#define ROW_SIZE 16
#define DATASET_BYTES 5

#define CYDEV_EEPROM_ROW_SIZE 0x00000010u
#define CY_EEPROM_SIZEOF_ROW        (CYDEV_EEPROM_ROW_SIZE)

#define CYDEV_EE_SIZE 0x00000800u
#define CY_EEPROM_SIZE              (CYDEV_EE_SIZE)
    
//#define SEND_CONST_STRING(string,port) send_buffer(string,strlen(string),port);

   
#define CT2_TYPE_CURRENT      0
#define CT2_TYPE_VOLTAGE      1
    
/*    
#define SKIP_SPACE(ptr) 	if (*ptr == 0x20 && ptr != 0) ptr++ //skip space
#define CHECK_NULL(ptr)     if (*ptr == 0 || ptr == 0) goto helptext;
#define HELP_TEXT(text)        \
	helptext:;                  \
	Term_Color_Red(ptr);       \
	send_string(text, ptr);  \
	Term_Color_White(ptr);     \
	return 1;                  \
   */
typedef struct port_struct port_str;
struct port_struct {
    uint8_t type;
    uint8_t num;
    uint8_t term_mode;
    StreamBufferHandle_t tx;
    StreamBufferHandle_t rx;
    xSemaphoreHandle term_block;
    xTaskHandle telemetry_handle;
};

   
typedef struct parameter_entry_struct parameter_entry;
struct parameter_entry_struct {
    const uint8_t parameter_type;
    uint8_t visible;
	const char *name;
	void *value;
	const uint8_t size;
	const uint8_t type;
	const int32_t min;
	const int32_t max;
    const uint16_t div;
	uint8_t (*callback_function)(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
	const char *help;
};

uint8_t updateDefaultFunction(parameter_entry * params, char * newValue, uint8_t index, TERMINAL_HANDLE * handle);
void EEPROM_check_hash(parameter_entry * params, uint8_t param_size, TERMINAL_HANDLE * handle);
void EEPROM_write_conf(parameter_entry * params, uint8_t param_size, uint16_t eeprom_offset ,TERMINAL_HANDLE * handle);
void EEPROM_read_conf(parameter_entry * params, uint8_t param_size, uint16_t eeprom_offset ,TERMINAL_HANDLE * handle);
void print_param_help(parameter_entry * params, uint8_t param_size, TERMINAL_HANDLE * handle);
void print_param(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);
void print_param_buffer(char * buffer, parameter_entry * params, uint8_t index);

void input_interpret(port_str *ptr);
void input_restart(void);
//void Term_Move_Cursor_right(uint8_t column, TERMINAL_HANDLE * handle);
//void Term_Move_Cursor_left(uint8_t column, TERMINAL_HANDLE * handle);
void Term_Box(uint8_t row1, uint8_t col1, uint8_t row2, uint8_t col2, TERMINAL_HANDLE * handle);
uint8_t getch(TERMINAL_HANDLE * handle, TickType_t xTicksToWait);
uint8_t getche(TERMINAL_HANDLE * handle, TickType_t xTicksToWait);
uint8_t kbhit(TERMINAL_HANDLE * handle);
uint8_t Term_check_break(TERMINAL_HANDLE * handle, uint32_t ms_to_wait);
/*void send_char(uint8_t c, port_str *ptr);
void send_string(char *data, port_str *ptr);
void send_buffer_u8(uint8_t *data, uint16_t len, port_str *ptr);
void send_buffer_c(char *data, uint16_t len, port_str *ptr);
uint8_t split(char *ptr, char *ret[], uint8_t size_ret, char split_char);*/

uint8_t term_config_changed(void);
uint32_t djb_hash(const char* cp);
uint8_t set_visibility(parameter_entry * params, uint8_t param_size, char* text, uint8_t visible);

#endif
