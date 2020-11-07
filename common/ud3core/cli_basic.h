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

   
#define CT2_TYPE_CURRENT      0
#define CT2_TYPE_VOLTAGE      1
    
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

uint8_t getch(TERMINAL_HANDLE * handle, TickType_t xTicksToWait);
uint8_t getche(TERMINAL_HANDLE * handle, TickType_t xTicksToWait);
uint8_t kbhit(TERMINAL_HANDLE * handle);
uint8_t Term_check_break(TERMINAL_HANDLE * handle, uint32_t ms_to_wait);

uint8_t term_config_changed(void);
uint32_t djb_hash(const char* cp);
uint8_t set_visibility(parameter_entry * params, uint8_t param_size, char* text, uint8_t visible);

#endif
