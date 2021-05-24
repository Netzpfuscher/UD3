#ifndef Term_ACH
#define Term_ACH

#if PIC32
#include <xc.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

typedef struct __ACL__ AC_LIST_ELEMENT;
typedef struct __ACL_HEAD__ AC_LIST_HEAD;

struct __ACL__{
    AC_LIST_ELEMENT * next;
    char * string;
};

struct __ACL_HEAD__{
    unsigned isConst;
    uint32_t elementCount;
    AC_LIST_ELEMENT * first;
};

AC_LIST_HEAD * ACL_create();
AC_LIST_HEAD * ACL_createConst(char ** strings, uint32_t count);
AC_LIST_ELEMENT * ACL_getNext(AC_LIST_ELEMENT * currElement);
void ACL_add(AC_LIST_HEAD * head, char * string);
void ACL_remove(AC_LIST_HEAD * head, char * string);
uint8_t TERM_doListAC(AC_LIST_HEAD * head, char * currInput, uint8_t length, char ** buff);
uint8_t ACL_defaultCompleter(TERMINAL_HANDLE * handle, void * params);
unsigned ACL_isSorted(char * a, char * b);
void ACL_remove(AC_LIST_HEAD * head, char * string);
void ACL_add(AC_LIST_HEAD * head, char * string);

#endif