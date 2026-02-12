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

/**
 * @file nvm.h
 * @brief Non-volatile memory (flash) interface for VMS maps and blocks
 *
 * Provides flash memory storage for Voice Music System (VMS) note mappings and
 * modulation blocks. Supports both PSoC hardware flash and file-based simulation.
 */

#if !defined(nvm_H)
#define nvm_H

    #include "TTerm.h"
    #include "VMS.h"
    
    #ifndef SIMULATOR
        extern const volatile uint8_t* NVM_mapMem;      //!< Pointer to MIDI map table in flash
        extern const volatile uint8_t * NVM_blockMem;   //!< Pointer to VMS block memory in flash
        extern const volatile VMS_Block_t * NVM_blocks; //!< Pointer to VMS blocks array in flash
    #else
        extern uint8_t * NVM_mapMem;      //!< Pointer to MIDI map table (simulator)
        extern uint8_t * NVM_blockMem;   //!< Pointer to VMS block memory (simulator)
        extern VMS_Block_t * NVM_blocks; //!< Pointer to VMS blocks array (simulator)

        /**
         * @brief Load flash data from file (simulator only)
         *
         * Loads flash.data file into simulated flash memory.
         */
        void load_flash();
    #endif
    
    #define MAPMEM_SIZE   16384  //!< Size of MIDI map table area in bytes
    #define BLOCKMEM_SIZE 16384  //!< Size of VMS block area in bytes
    
    /**
     * @brief Initialize NVM subsystem
     *
     * Sets up flash memory access (hardware) or loads flash.data (simulator).
     */
    void nvm_init();
    /**
     * @brief NVM command handler
     *
     * CLI command for inspecting flash contents:
     * - `nvm maps` - Display all MIDI map tables
     * - `nvm blocks` - Display all VMS modulation blocks
     * - `nvm block [id]` - Display specific block by ID
     *
     * @param handle Terminal handle
     * @param argCount Number of arguments
     * @param args Argument array
     * @return TERM_CMD_EXIT_SUCCESS
     */
    uint8_t CMD_nvm(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args);
    /**
     * @brief Write buffer to flash memory
     *
     * Performs buffered flash write operation. Call nvm_flush() to commit.
     *
     * @param index Byte offset in flash memory
     * @param buffer Data to write
     * @param len Number of bytes to write
     * @return pdTRUE on success, pdFAIL on error
     */
    uint8_t nvm_write_buffer(uint16_t index, uint8_t* buffer, int32_t len);
    /**
     * @brief Flush pending flash writes
     *
     * Commits buffered flash page to non-volatile storage.
     *
     * @return pdTRUE on success, pdFAIL on error
     */
    uint8_t nvm_flush();
    /**
     * @brief Count valid VMS blocks in flash
     *
     * @param blk Pointer to VMS block array
     * @return Number of valid blocks
     */
    uint32_t nvm_get_blk_cnt(const VMS_Block_t * blk);
    
#endif
