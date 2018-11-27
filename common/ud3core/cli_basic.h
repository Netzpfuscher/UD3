#ifndef CLI_BASIC_H
#define CLI_BASIC_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"

#define PARAM_SIZE(param) sizeof(param) / sizeof(parameter_entry)

#define SIZEP(x) ((char*)(&(x) + 1) - (char*)&(x))
#define ADD_PARAM(para_type,text, value_var, type, min, max, div, update_func, help_text) {para_type,text, &value_var, SIZEP(value_var), type,min, max, div, update_func, help_text},
#define ADD_COMMAND(command, command_func, help_text) {command, command_func, help_text},
#define TYPE_UNSIGNED   0
#define TYPE_SIGNED     1
#define TYPE_FLOAT      2
#define TYPE_CHAR       3
#define TYPE_STRING     4

#define PARAM_DEFAULT   0
#define PARAM_CONFIG    1

#define ROW_SIZE 16
#define DATASET_BYTES 5

#define CYDEV_EEPROM_ROW_SIZE 0x00000010u
#define CY_EEPROM_SIZEOF_ROW        (CYDEV_EEPROM_ROW_SIZE)

#define CYDEV_EE_SIZE 0x00000800u
#define CY_EEPROM_SIZE              (CYDEV_EE_SIZE)


#define NUM_CON (NUM_ETH_CON+3)

#define PORT_TYPE_NULL      0    
#define PORT_TYPE_SERIAL    1
#define PORT_TYPE_USB       2
#define PORT_TYPE_ETH       3
    
#define PORT_TERM_VT100  0
#define PORT_TERM_TT     1
    
    
typedef struct port_struct port_str;
struct port_struct {
    uint8_t type;
    uint8_t num;
    uint8_t term_mode;
    xSemaphoreHandle term_block;
};

   
typedef struct parameter_entry_struct parameter_entry;
struct parameter_entry_struct {
    const uint8_t parameter_type;
	const char *name;
	void *value;
	const uint8_t size;
	const uint8_t type;
	const int32_t min;
	const int32_t max;
    const uint16_t div;
	uint8_t (*callback_function)(parameter_entry * params, uint8_t index, port_str *ptr);
	const char *help;
};



uint8_t updateDefaultFunction(parameter_entry * params, char * newValue, uint8_t index, port_str *ptr);
void EEPROM_write_conf(parameter_entry * params, uint8_t param_size, uint16_t eeprom_offset ,port_str *ptr);
void EEPROM_read_conf(parameter_entry * params, uint8_t param_size, uint16_t eeprom_offset ,port_str *ptr);
void print_param_help(parameter_entry * params, uint8_t param_size, port_str *ptr);
void print_param(parameter_entry * params, uint8_t index, port_str *ptr);
void print_param_buffer(char * buffer, parameter_entry * params, uint8_t index);

void input_interpret(port_str *ptr);
void input_restart(void);
void Term_Move_cursor_right(uint8_t column, port_str *ptr);
void Term_Move_Cursor(uint8_t row, uint8_t column, port_str *ptr);
void Term_Erase_Screen(port_str *ptr);
void Term_Color_Green(port_str *ptr);
void Term_Color_Red(port_str *ptr);
void Term_Color_White(port_str *ptr);
void Term_Color_Cyan(port_str *ptr);
void Term_BGColor_Blue(port_str *ptr);
void Term_Save_Cursor(port_str *ptr);
void Term_Restore_Cursor(port_str *ptr);
void Term_Box(uint8_t row1, uint8_t col1, uint8_t row2, uint8_t col2, port_str *ptr);
void send_char(uint8_t c, port_str *ptr);
void send_string(char *data, port_str *ptr);
void send_buffer(uint8_t *data, uint16_t len, port_str *ptr);
uint8_t term_config_changed(void);
uint32_t djb_hash(const char* cp);

#endif