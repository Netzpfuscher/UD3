/*
 * VMS - Versatile Modulation System
 *
 * Copyright (c) 2020 Thorben Zethoff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
    
#if !defined(DLL_H)
#define DLL_H
    
  
#if PIC32
#include <xc.h>
#endif
#include <stdint.h>
#include "helper/vms_types.h"


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