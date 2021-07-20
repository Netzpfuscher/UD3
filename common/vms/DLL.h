/*
    Copyright (C) 2020,2021 TMax-Electronics.de
   
    This file is part of the MidiStick Firmware.

    the MidiStick Firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    the MidiStick Firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the MidiStick Firmware.  If not, see <https://www.gnu.org/licenses/>.
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
//void DLL_dump(DLLObject * listHead);
void DLL_moveToEnd(DLLObject * currObject, DLLObject * listHead);

#endif