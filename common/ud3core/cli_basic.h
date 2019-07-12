#ifndef CLI_BASIC_H
#define CLI_BASIC_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "stream_buffer.h" 

#define PARAM_SIZE(param) sizeof(param) / sizeof(parameter_entry)

#define SIZEP(x) ((char*)(&(x) + 1) - (char*)&(x))
#define ADD_PARAM(para_type, visible,text, value_var, type, min, max, div, update_func, help_text) {para_type, visible,text, &value_var, SIZEP(value_var), type,min, max, div, update_func, help_text},
#define ADD_COMMAND(command, command_func, help_text) {command, command_func, help_text},
#define TYPE_UNSIGNED   0
#define TYPE_SIGNED     1
#define TYPE_FLOAT      2
#define TYPE_CHAR       3
#define TYPE_STRING     4
    
#define VISIBLE_TRUE    1
#define VISIBLE_FALSE   0

#define PARAM_DEFAULT   0
#define PARAM_CONFIG    1

#define ROW_SIZE 16
#define DATASET_BYTES 5

#define CYDEV_EEPROM_ROW_SIZE 0x00000010u
#define CY_EEPROM_SIZEOF_ROW        (CYDEV_EEPROM_ROW_SIZE)

#define CYDEV_EE_SIZE 0x00000800u
#define CY_EEPROM_SIZE              (CYDEV_EE_SIZE)
    
#define SEND_CONST_STRING(string,port) send_buffer((uint8_t*)string,strlen(string),port);


#define NUM_CON (NUM_ETH_CON+3)

#define PORT_TYPE_NULL      0    
#define PORT_TYPE_SERIAL    1
#define PORT_TYPE_USB       2
#define PORT_TYPE_ETH       3
    
#define PORT_TERM_VT100  0
#define PORT_TERM_TT     1
#define PORT_TERM_MQTT   2
    
#define ETH_HW_DISABLED  0
#define ETH_HW_W5500     1
#define ETH_HW_ESP32     2
    
#define CT2_TYPE_CURRENT      0
#define CT2_TYPE_VOLTAGE      1
    
    
#define SKIP_SPACE(ptr) 	if (*ptr == 0x20 && ptr != 0) ptr++ //skip space
#define CHECK_NULL(ptr)     if (*ptr == 0 || ptr == 0) goto helptext;
#define HELP_TEXT(text)        \
	helptext:;                  \
	Term_Color_Red(ptr);       \
	send_string(text, ptr);  \
	Term_Color_White(ptr);     \
	return 1;                  \
   
typedef struct port_struct port_str;
struct port_struct {
    uint8_t type;
    uint8_t num;
    uint8_t term_mode;
    StreamBufferHandle_t tx;
    StreamBufferHandle_t rx;
    xSemaphoreHandle term_block;
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
void Term_Disable_Cursor(port_str *ptr);
void Term_Enable_Cursor(port_str *ptr);
void Term_Box(uint8_t row1, uint8_t col1, uint8_t row2, uint8_t col2, port_str *ptr);
uint8_t getch(port_str *ptr, TickType_t xTicksToWait);
uint8_t getche(port_str *ptr, TickType_t xTicksToWait);
uint8_t kbhit(port_str *ptr);
void send_char(uint8_t c, port_str *ptr);
void send_string(char *data, port_str *ptr);
void send_buffer(uint8_t *data, uint16_t len, port_str *ptr);
uint8_t term_config_changed(void);
uint32_t djb_hash(const char* cp);
uint8_t set_visibility(parameter_entry * params, uint8_t param_size, char* text, uint8_t visible);

#endif