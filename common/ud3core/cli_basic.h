
#include <stdint.h>

#define PARAM_SIZE(param) sizeof(param) / sizeof(parameter_entry)

#define SIZEP(x) ((char*)(&(x) + 1) - (char*)&(x))
#define ADD_PARAM(para_type,text, value_var, type, min, max, update_func, help_text) {para_type,text, &value_var, SIZEP(value_var), type, min, max, update_func, help_text},
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

#define SERIAL 0
#define USB 1

typedef struct parameter_entry_struct parameter_entry;
struct parameter_entry_struct {
    const uint8_t parameter_type;
	const char *name;
	void *value;
	const uint8_t size;
	const uint8_t type;
	int32_t min;
	int32_t max;
	uint8_t (*callback_function)(parameter_entry * params, uint8_t index, uint8_t port);
	const char *help;
};
uint8_t updateDefaultFunction(parameter_entry * params, char * newValue, uint8_t index, uint8_t port);
void EEPROM_write_conf(parameter_entry * params, uint8_t param_size, uint16_t eeprom_offset ,uint8_t port);
void EEPROM_read_conf(parameter_entry * params, uint8_t param_size, uint16_t eeprom_offset ,uint8_t port);
void print_param_help(parameter_entry * params, uint8_t param_size, uint8_t port);
void print_param(parameter_entry * params, uint8_t index, uint8_t port);

void input_interpret(uint8_t port);
void input_restart(void);
void Term_Move_cursor_right(uint8_t column, uint8_t port);
void Term_Move_Cursor(uint8_t row, uint8_t column, uint8_t port);
void Term_Erase_Screen(uint8_t port);
void Term_Color_Green(uint8_t port);
void Term_Color_Red(uint8_t port);
void Term_Color_White(uint8_t port);
void Term_Color_Cyan(uint8_t port);
void Term_BGColor_Blue(uint8_t port);
void Term_Save_Cursor(uint8_t port);
void Term_Restore_Cursor(uint8_t port);
void Term_Box(uint8_t row1, uint8_t col1, uint8_t row2, uint8_t col2, uint8_t port);
void send_char(uint8_t c, uint8_t port);
void send_string(char *data, uint8_t port);
void send_buffer(uint8_t *data, uint16_t len, uint8_t port);
uint8_t term_config_changed(void);
