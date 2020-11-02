
#if !defined(TTerm_H)
#define TTerm_H
    
#ifndef _VT100_CURSOR_POS1

#include "FreeRTOS.h"
#include "task.h"
#if PIC32 == 1    
#include "ff.h"
#endif    

#define EXTENDED_PRINTF 1

#if PIC32 == 1 
    #define START_OF_FLASH  0xa0000000
    #define END_OF_FLASH    0xa000ffff
#else
    #define START_OF_FLASH  0x00000000
    #define END_OF_FLASH    0x1FFF8000
#endif

#define _VT100_CURSOR_POS1 3
#define _VT100_CURSOR_END 4
#define _VT100_FOREGROUND_COLOR 5
#define _VT100_BACKGROUND_COLOR 6
#define _VT100_RESET_ATTRIB 7
#define _VT100_BRIGHT 8
#define _VT100_DIM 9
#define _VT100_UNDERSCORE 10
#define _VT100_BLINK 11
#define _VT100_REVERSE 12
#define _VT100_HIDDEN 13
#define _VT100_ERASE_SCREEN 14
#define _VT100_ERASE_LINE 15
#define _VT100_FONT_G0 16
#define _VT100_FONT_G1 17
#define _VT100_WRAP_ON 18
#define _VT100_WRAP_OFF 19
#define _VT100_ERASE_LINE_END 20
#define _VT100_CURSOR_BACK_BY 21
#define _VT100_CURSOR_FORWARD_BY 22
#define _VT100_CURSOR_SAVE_POSITION 23
#define _VT100_CURSOR_RESTORE_POSITION 24

//VT100 cmds given to us by the terminal software (they need to be > 8 bits so the handler can tell them apart from normal characters)
#define _VT100_RESET                0x1000
#define _VT100_KEY_END              0x1001
#define _VT100_KEY_POS1             0x1002
#define _VT100_CURSOR_FORWARD       0x1003
#define _VT100_CURSOR_BACK          0x1004
#define _VT100_CURSOR_UP            0x1005
#define _VT100_CURSOR_DOWN          0x1006
#define _VT100_BACKWARDS_TAB        0x1007

#define _VT100_BLACK 0
#define _VT100_RED 1
#define _VT100_GREEN 2
#define _VT100_YELLOW 3
#define _VT100_BLUE 4
#define _VT100_MAGENTA 5
#define _VT100_CYAN 6
#define _VT100_WHITE 7

#define _VT100_POS_IGNORE 0xffff

#define TERM_DEVICE_NAME "UD3 Fibernet"
#define TERM_VERSION_STRING "V1.0"

#define TERM_HISTORYSIZE 16
#define TERM_INPUTBUFFER_SIZE 128

                        
#define TERM_ARGS_ERROR_STRING_LITERAL 0xffff

#define TERM_CMD_EXIT_ERROR 0
#define TERM_CMD_EXIT_NOT_FOUND 1
#define TERM_CMD_EXIT_SUCCESS 0xff
#define TERM_CMD_EXIT_PROC_STARTED 0xfe
#define TERM_CMD_CONTINUE 0x80

#define TERM_ENABLE_STARTUP_TEXT
//#define TERM_SUPPORT_CWD

#ifdef TERM_ENABLE_STARTUP_TEXT
const extern char TERM_startupText1[];
const extern char TERM_startupText2[];
const extern char TERM_startupText3[];
#endif

#if EXTENDED_PRINTF == 1
#define ttprintf(format, ...) (*handle->print)(handle->port, format, ##__VA_ARGS__)
#else
#define ttprintf(format, ...) (*handle->print)(format, ##__VA_ARGS__)
#endif

typedef struct __TERMINAL_HANDLE__ TERMINAL_HANDLE;
typedef struct __TermCommandDescriptor__ TermCommandDescriptor;

typedef uint8_t (* TermCommandFunction)(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
typedef uint8_t (* TermCommandInputHandler)(TERMINAL_HANDLE * handle, uint16_t c);

#if EXTENDED_PRINTF == 1
typedef void (* TermPrintHandler)(void * port, char * format, ...);
#else
typedef void (* TermPrintHandler)(char * format, ...);
#endif
typedef uint8_t (* TermAutoCompHandler)(TERMINAL_HANDLE * handle, void * params);

typedef struct{
    TaskHandle_t task;
    TermCommandInputHandler inputHandler;
} TermProgram;

struct __TermCommandDescriptor__{
    TermCommandFunction function;
    const char * command;
    const char * commandDescription;
    uint8_t commandLength;
    uint8_t minPermissionLevel;
    TermAutoCompHandler ACHandler;
    void * ACParams;
};

struct __TERMINAL_HANDLE__{
    char * inputBuffer;
    #if EXTENDED_PRINTF == 1
    void * port;
    #endif        
    uint32_t currBufferPosition;
    uint32_t currBufferLength;
    uint32_t currAutocompleteCount;
    TermProgram * currProgram;
    char ** autocompleteBuffer;
    uint32_t autocompleteBufferLength;
    uint32_t autocompleteStart;    
    TermPrintHandler * print;
    char * currUserName;
    char * historyBuffer[TERM_HISTORYSIZE];
    uint32_t currHistoryWritePosition;
    uint32_t currHistoryReadPosition;
    uint8_t currEscSeqPos;
    uint8_t escSeqBuff[16];
    
//TODO actually finish implementing this...
#ifdef TERM_SUPPORT_CWD
    DIR cwd;
#endif
};

typedef enum{
    TERM_CHECK_COMP_AND_HIST = 0b11, TERM_CHECK_COMP = 0b01, TERM_CHECK_HIST = 0b10, 
} COPYCHECK_MODE;

extern TermCommandDescriptor ** TERM_cmdList;
extern uint8_t TERM_cmdCount;

#if EXTENDED_PRINTF == 1
TERMINAL_HANDLE * TERM_createNewHandle(TermPrintHandler printFunction, void * port, const char * usr);
#else
TERMINAL_HANDLE * TERM_createNewHandle(TermPrintHandler printFunction, const char * usr);    
#endif    
void TERM_destroyHandle(TERMINAL_HANDLE * handle);
uint8_t TERM_processBuffer(uint8_t * data, uint16_t length, TERMINAL_HANDLE * handle);
unsigned isACIILetter(char c);
uint8_t TERM_handleInput(uint16_t c, TERMINAL_HANDLE * handle);
char * strnchr(char * str, char c, uint32_t length);
void strsft(char * src, int32_t startByte, int32_t offset);
void TERM_printBootMessage(TERMINAL_HANDLE * handle);
uint8_t TERM_findMatchingCMDs(char * currInput, uint8_t length, char ** buff);
void TERM_freeCommandList(TermCommandDescriptor ** cl, uint16_t length);
uint8_t TERM_buildCMDList();
TermCommandDescriptor * TERM_addCommand(TermCommandFunction function, const char * command, const char * description, uint8_t minPermissionLevel);
void TERM_addCommandAC(TermCommandDescriptor * cmd, TermAutoCompHandler ACH, void * ACParams);
unsigned TERM_isSorted(TermCommandDescriptor * a, TermCommandDescriptor * b);
char toLowerCase(char c);
void TERM_setCursorPos(TERMINAL_HANDLE * handle, uint16_t x, uint16_t y);
void TERM_sendVT100Code(TERMINAL_HANDLE * handle, uint16_t cmd, uint8_t var);
const char * TERM_getVT100Code(uint16_t cmd, uint8_t var);
uint16_t TERM_countArgs(const char * data, uint16_t dataLength);
uint8_t TERM_interpretCMD(char * data, uint16_t dataLength, TERMINAL_HANDLE * handle);
uint8_t TERM_seperateArgs(char * data, uint16_t dataLength, char ** buff);
void TERM_checkForCopy(TERMINAL_HANDLE * handle, COPYCHECK_MODE mode);
void TERM_printDebug(TERMINAL_HANDLE * handle, char * format, ...);
void TERM_removeProgramm(TERMINAL_HANDLE * handle);
void TERM_attachProgramm(TERMINAL_HANDLE * handle, TermProgram * prog);
uint8_t TERM_doAutoComplete(TERMINAL_HANDLE * handle);
TermCommandDescriptor * TERM_findCMD(TERMINAL_HANDLE * handle);
uint8_t TERM_findLastArg(TERMINAL_HANDLE * handle, char * buff, uint8_t * lenBuff);
BaseType_t ptr_is_in_ram(void* ptr);

#endif
#endif
