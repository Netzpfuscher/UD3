/*
 * UD3 - NVM
 *
 * Copyright (c) 2021 Jens Kerrinnes
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

#if !defined(nvm_H)
#define nvm_H

    #include "TTerm.h"
    #include "vms_types.h"
    
    #ifndef SIMULATOR
        extern const volatile uint8_t* NVM_mapMem;
        extern const volatile uint8_t * NVM_blockMem;
        extern const VMS_BLOCK* VMS_BLKS;
    #else
        extern uint8_t * NVM_mapMem;
        extern uint8_t * NVM_blockMem;
        extern VMS_BLOCK* VMS_BLKS;
    #endif
    
    #define MAPMEM_SIZE   16384
    #define BLOCKMEM_SIZE 16384
    
    void nvm_init();
    uint8_t CMD_nvm(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
    uint8_t nvm_write_buffer(uint16_t index, uint8_t* buffer, int32_t len);
    
#endif