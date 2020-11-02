
 #if PIC32 == 1
#include <xc.h>
#endif  
#include <stdint.h>

#include "TTerm.h"
#include "TTerm_AC.h"

#ifndef TTERM_CMD
#define TTERM_CMD

AC_LIST_HEAD * head;

uint8_t CMD_testCommandHandler(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
uint8_t TERM_testCommandAutoCompleter(TERMINAL_HANDLE * handle, void * params);
uint8_t CMD_help(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
uint8_t CMD_cls(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
uint8_t CMD_top(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
void CMD_top_task(void * handle);
uint8_t CMD_top_handleInput(TERMINAL_HANDLE * handle, uint16_t c);

#endif