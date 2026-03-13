/**
 * @file cli_basic.h
 * @brief Core parameter system and CLI utilities
 * 
 * Provides the parameter registration system, EEPROM configuration storage,
 * terminal I/O helpers, and type-generic parameter handling for the UD3
 * command-line interface.
 */

#ifndef CLI_BASIC_H
#define CLI_BASIC_H

#include <stdint.h>
#include "config.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "stream_buffer.h" 
#include "task.h"
#include "TTerm.h"
    
/** @brief Escape sequence string for VT100 commands */
#define ESC_STR "\x1b"
   
    
/** @defgroup bit_macros Bit Manipulation Macros
 * @{
 */
/** @brief Set a bit in a variable */
#define set_bit(var, bit) ((var) |= (1 << (bit)))

/** @brief Clear a bit in a variable (Bit löschen) */
#define clear_bit(var, bit) ((var) &= (unsigned)~(1 << (bit)))

/** @brief Toggle a bit in a variable (Bit togglen) */
#define toggle_bit(var,bit) ((var) ^= (1 << (bit)))

/** @brief Check if a bit is set (Bit abfragen) */
#define bit_is_set(var, bit) ((var) & (1 << (bit)))
/** @brief Check if a bit is clear */
#define bit_is_clear(var, bit) !bit_is_set(var, bit)
/** @} */

/** @brief Calculate number of elements in parameter array */
#define PARAM_SIZE(param) sizeof(param) / sizeof(parameter_entry)

/**
 * @brief CLI parameter data types
 */
enum cli_types{
	TYPE_UNSIGNED,  /**< Unsigned integer (8/16/32 bit) */
	TYPE_SIGNED,    /**< Signed integer (8/16/32 bit) */
	TYPE_FLOAT,     /**< Floating point (32 bit) */
	TYPE_CHAR,      /**< Single character */
	TYPE_STRING,    /**< Null-terminated string */
	TYPE_BUFFER     /**< Binary buffer (uint16_t*) */
};

/**
 * @brief Port and terminal type identifiers
 */
enum port{
	PORT_TYPE_NULL,    /**< Null port (no I/O) */
	PORT_TYPE_SERIAL,  /**< Serial UART port */
	PORT_TYPE_USB,     /**< USB CDC port */
	PORT_TYPE_MIN,     /**< MIN protocol port */
	PORT_TERM_VT100,   /**< VT100 terminal mode */
	PORT_TERM_TT,      /**< Teslaterm terminal mode */
	PORT_TERM_MQTT     /**< MQTT terminal mode */
};
    
/**
 * @brief Type detection macro using C11 _Generic
 * 
 * Automatically determines the CLI type enum value for a variable
 * based on its C type.
 */
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

/**
 * @brief Size calculation macro (differs between hardware and simulator)
 * 
 * On hardware, calculates actual memory size. On simulator, uses sizeof().
 */
#ifndef SIMULATOR
	#define SIZEP(x) ((char*)(&(x) + 1) - (char*)&(x))
#else
	#define SIZEP(x) sizeof(x)
#endif

/**
 * @brief Add a parameter to parameter array
 * @param para_type PARAM_CONFIG (saved to EEPROM) or PARAM_DEFAULT (runtime only)
 * @param visible Non-zero to show in help, 0 to hide
 * @param text Parameter name (used in get/set commands)
 * @param value_var Variable to bind to this parameter
 * @param min Minimum allowed value
 * @param max Maximum allowed value
 * @param div Division factor for fixed-point display (0 for integers)
 * @param update_func Callback function when value changes (can be NULL)
 * @param help_text Help text shown in parameter listing
 */
#define ADD_PARAM(para_type, visible,text, value_var, min, max, div, update_func, help_text) {para_type, visible,text, &value_var, SIZEP(value_var), typename(value_var),min, max, div, update_func, help_text},

/**
 * @brief Add a command to command array (deprecated - use TERM_addCommand)
 */
#define ADD_COMMAND(command, command_func, help_text) {command, command_func, help_text},

    
/** @defgroup param_types Parameter Type Flags
 * @{
 */
#define PARAM_DEFAULT   0  /**< Runtime parameter (not saved to EEPROM) */
#define PARAM_CONFIG    1  /**< Configuration parameter (saved to EEPROM) */
/** @} */

/** @defgroup eeprom_constants EEPROM Size Constants
 * @{
 */
#define ROW_SIZE 16        /**< EEPROM row size in bytes */
#define DATASET_BYTES 5    /**< Bytes per EEPROM dataset header */

#define CYDEV_EEPROM_ROW_SIZE 0x00000010u  /**< Cypress EEPROM row size */
#define CY_EEPROM_SIZEOF_ROW        (CYDEV_EEPROM_ROW_SIZE)

#define CYDEV_EE_SIZE 0x00000800u          /**< Total EEPROM size (2048 bytes) */
#define CY_EEPROM_SIZE              (CYDEV_EE_SIZE)

/** @brief EEPROM read byte macro */
#define EEPROM_READ_BYTE(x) EEPROM_1_ReadByte(x)
/** @brief EEPROM write row macro */
#define EEPROM_WRITE_ROW(x,y) EEPROM_1_Write(y,x)
/** @} */

   
/** @defgroup ct2_types CT2 Current Transformer Types
 * @{
 */   
#define CT2_TYPE_CURRENT      0  /**< CT2 measures current directly */
#define CT2_TYPE_VOLTAGE      1  /**< CT2 measures voltage (for voltage-mode CTs) */
/** @} */
    
/**
 * @brief Port communication structure
 * 
 * Describes a bidirectional communication port with stream buffers,
 * terminal mode, and FreeRTOS synchronization.
 */
typedef struct port_struct port_str;
struct port_struct {
	uint8_t type;                      /**< Port type (PORT_TYPE_*) */
	uint8_t num;                       /**< Port number/instance */
	uint8_t term_mode;                 /**< Terminal mode (PORT_TERM_*) */
	uint8_t term_send_alarms;          /**< Send alarms to this terminal */
	StreamBufferHandle_t tx;           /**< Transmit stream buffer */
	StreamBufferHandle_t rx;           /**< Receive stream buffer */
	xSemaphoreHandle term_block;       /**< Terminal blocking semaphore */
	xTaskHandle telemetry_handle;      /**< Telemetry task handle */
};

   
/**
 * @brief Parameter entry descriptor
 * 
 * Defines a configurable parameter with type information, value range,
 * storage location, and optional callback function.
 */
typedef struct parameter_entry_struct parameter_entry;
struct parameter_entry_struct {
	const uint8_t parameter_type;      /**< PARAM_CONFIG or PARAM_DEFAULT */
	uint8_t visible;                   /**< Show in help (non-zero) or hide (0) */
	const char *name;                  /**< Parameter name for get/set commands */
	void *value;                       /**< Pointer to parameter variable */
	const uint8_t size;                /**< Size in bytes (1/2/4) */
	const uint8_t type;                /**< Data type (TYPE_*) */
	const int32_t min;                 /**< Minimum allowed value */
	const int32_t max;                 /**< Maximum allowed value */
	const uint16_t div;                /**< Division factor for fixed-point display */
	uint8_t (*callback_function)(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);  /**< Change callback */
	const char *help;                  /**< Help text */
};

/**
 * @brief Update parameter value from string
 * @param params Parameter array
 * @param newValue New value as string
 * @param index Parameter index in array
 * @param handle Terminal handle for error messages
 * @return 1 if successful, 0 if failed
 * 
 * Parses newValue string and updates parameter if within range.
 * Handles type conversion for unsigned, signed, float, char, and string types.
 */
uint8_t updateDefaultFunction(parameter_entry * params, char * newValue, uint8_t index, TERMINAL_HANDLE * handle);

uint8_t n_number(uint32_t n);

/**
 * @brief Check parameter names for hash collisions
 * @param params Parameter array
 * @param param_size Number of parameters
 * @param handle Terminal handle for output
 * 
 * Validates that all PARAM_CONFIG parameters have unique djb2 hashes
 * to prevent EEPROM storage conflicts.
 */
void EEPROM_check_hash(parameter_entry * params, uint8_t param_size, TERMINAL_HANDLE * handle);

/**
 * @brief Write configuration parameters to EEPROM
 * @param params Parameter array
 * @param param_size Number of parameters
 * @param eeprom_offset Starting EEPROM address
 * @param handle Terminal handle for output
 * 
 * Saves all PARAM_CONFIG parameters to EEPROM with magic header and trailer.
 * Only writes changed values to minimize EEPROM wear.
 */
void EEPROM_write_conf(parameter_entry * params, uint8_t param_size, uint16_t eeprom_offset ,TERMINAL_HANDLE * handle);

/**
 * @brief Read configuration parameters from EEPROM
 * @param params Parameter array
 * @param param_size Number of parameters
 * @param eeprom_offset Starting EEPROM address
 * @param handle Terminal handle for output
 * 
 * Loads PARAM_CONFIG parameters from EEPROM. Validates magic header,
 * checks parameter sizes, and reports errors for missing or mismatched parameters.
 */
void EEPROM_read_conf(parameter_entry * params, uint8_t param_size, uint16_t eeprom_offset ,TERMINAL_HANDLE * handle);

/**
 * @brief Check if EEPROM dataset is valid
 * @return pdTRUE if dataset has errors, pdFALSE if valid
 */
uint8_t EEPROM_not_valid();

/**
 * @brief Print all parameters with help text
 * @param params Parameter array
 * @param param_size Number of parameters
 * @param handle Terminal handle
 * 
 * Displays formatted table of all visible parameters with current values
 * and help text. Separates runtime parameters from configuration parameters.
 */
void print_param_help(parameter_entry * params, uint8_t param_size, TERMINAL_HANDLE * handle);

/**
 * @brief Print single parameter value
 * @param params Parameter array
 * @param index Parameter index
 * @param handle Terminal handle
 */
void print_param(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle);

/**
 * @brief Format parameter to buffer for serial transmission
 * @param buffer Output buffer
 * @param params Parameter array
 * @param index Parameter index
 * 
 * Formats parameter in semicolon-separated format:
 * name;value;type;size;min;max
 */
void print_param_buffer(char * buffer, parameter_entry * params, uint8_t index);

/**
 * @brief Get character from terminal without echo
 * @param handle Terminal handle
 * @param xTicksToWait Timeout in FreeRTOS ticks
 * @return Character received, or 0 if timeout
 * 
 * Blocks until character available or timeout. Releases terminal
 * semaphore during wait to allow other operations.
 */
uint8_t getch(TERMINAL_HANDLE * handle, TickType_t xTicksToWait);

/**
 * @brief Get character from terminal with echo
 * @param handle Terminal handle
 * @param xTicksToWait Timeout in FreeRTOS ticks
 * @return Character received, or 0 if timeout
 * 
 * Like getch() but echoes character back to terminal.
 */
uint8_t getche(TERMINAL_HANDLE * handle, TickType_t xTicksToWait);

/**
 * @brief Check if character available in receive buffer
 * @param handle Terminal handle
 * @return pdTRUE if character available, pdFALSE if empty
 */
uint8_t kbhit(TERMINAL_HANDLE * handle);

/**
 * @brief Wait for break character (Ctrl+C or 'q')
 * @param handle Terminal handle
 * @param ms_to_wait Wait time in milliseconds
 * @return pdTRUE to continue, pdFALSE if break detected
 * 
 * Used in loops to allow user to interrupt long-running commands.
 */
uint8_t Term_check_break(TERMINAL_HANDLE * handle, uint32_t ms_to_wait);

/**
 * @brief Check if terminal configuration has changed
 * @return Non-zero if changed, 0 if unchanged
 */
uint8_t term_config_changed(void);

/**
 * @brief Calculate djb2 hash of string
 * @param cp String to hash
 * @return 32-bit hash value
 * 
 * Used for parameter name hashing in EEPROM storage.
 */
uint32_t djb_hash(const char* cp);

/**
 * @brief Set parameter visibility by name
 * @param params Parameter array
 * @param param_size Number of parameters
 * @param text Parameter name (or prefix)
 * @param visible New visibility (0=hidden, non-zero=visible)
 * @return pdTRUE if parameter found, pdFALSE if not found
 * 
 * Finds longest matching parameter name and sets its visibility.
 */
uint8_t set_visibility(parameter_entry * params, uint8_t param_size, char* text, uint8_t visible);

#endif
