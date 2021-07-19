#if PIC32
#include <xc.h>
#endif
#include <stdint.h>
#include "vms_types.h"

#ifndef DLL
#define DLL

/*
 * Creates a new doubly linked list
 *  
 * returns the new list's head object
 */
DLLObject * createNewDll();
uint32_t DLL_add(void * data, DLLObject * listHead);
void * findListEntryUID(uint32_t uid, DLLObject * listHead);
unsigned DLL_isEmpty(DLLObject * listHead);
void removeListEntryUID(uint32_t targetUID, DLLObject * listHead);
void DLL_remove(DLLObject * target);
void DLL_pop(DLLObject * listHead);
/*
void DLL_dump(DLLObject * listHead);
*/
void DLL_moveToEnd(DLLObject * currObject, DLLObject * listHead);

#endif