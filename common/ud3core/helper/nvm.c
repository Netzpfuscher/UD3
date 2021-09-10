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

#include "nvm.h"
#include "stdlib.h"


#define NVM_START_ADDR 0x38000
#define NVM_ARRAY 3
#define NVM_START_PAGE 128
#define NVM_PAGE_SIZE 256

#define NVM_DEBUG 0


#ifndef SIMULATOR
    const volatile uint8_t* NVM_mapMem = (uint8_t*)NVM_START_ADDR; 
    const volatile uint8_t * NVM_blockMem = (uint8_t*)(NVM_START_ADDR + MAPMEM_SIZE);
    const VMS_BLOCK* VMS_BLKS = (VMS_BLOCK*)(NVM_START_ADDR + MAPMEM_SIZE);
#else
    
    uint8_t flash_pages[128][256];
    
    uint8_t * NVM_mapMem = (uint8_t*)flash_pages;
    uint8_t * NVM_blockMem = (uint8_t*)(((uint8_t*)flash_pages) + MAPMEM_SIZE);
    VMS_BLOCK* VMS_BLKS = (VMS_BLOCK*)(((uint8_t*)flash_pages) + MAPMEM_SIZE);
    
    uint8_t CyWriteRowData(uint8_t arr_n, uint16_t page, uint8_t* page_content){
        page -= NVM_START_PAGE;
        memcpy(&flash_pages[page][0], page_content, NVM_PAGE_SIZE);
        return 0;
    }
    void CyFlushCache(){
        
    }
    void CySetTemp(){
        
    }
#endif

void nvm_init(){
   
    TERM_addCommand(CMD_nvm, "nvm","NVM-Test Func",0,&TERM_cmdListHead);  
}

static uint8_t *page_content=NULL;
static uint16_t last_page=0xFFFF;
static uint16_t page;


uint8_t nvm_flush(){
    CySetTemp();
    uint32_t rc = CyWriteRowData((uint8)NVM_ARRAY, page, page_content);
    CyFlushCache();
    vPortFree(page_content);
    page_content = NULL;
    last_page=0xFFFF;
    
    return rc ? pdFAIL : pdTRUE;
}

uint8_t nvm_write_buffer(uint16_t index, uint8_t* buffer, int32_t len){

    uint16_t n_pages = 0;


    if(page_content==NULL){
        page_content = pvPortMalloc(NVM_PAGE_SIZE);
    }
    if(page_content == NULL) return pdFAIL;
    
    uint32_t write_offset = index % NVM_PAGE_SIZE;

    while(len>0){
        uint8_t * page_addr = (uint8_t*)NVM_mapMem + ((index / NVM_PAGE_SIZE)*NVM_PAGE_SIZE); 
        page = NVM_START_PAGE + (index / NVM_PAGE_SIZE);
        if(last_page==0xFFFF){
            last_page = page;
            memcpy(page_content, page_addr , NVM_PAGE_SIZE);
        }
        if(page > 256) return pdFAIL;
        
        uint32_t write_len = len;
        if(write_len+write_offset > NVM_PAGE_SIZE){
            write_len = NVM_PAGE_SIZE-write_offset;   
        }
        
        
        
        if(last_page!=page){
            CySetTemp();
            uint32_t rc = CyWriteRowData((uint8)NVM_ARRAY, last_page, page_content);
            CyFlushCache();
            if(rc) return pdFAIL;
            memcpy(page_content, page_addr , NVM_PAGE_SIZE);
        }
        memcpy(page_content + write_offset, buffer , write_len);
        last_page=page;
        
        buffer+=write_len;
        
        if(write_offset){
            len-=(NVM_PAGE_SIZE-write_offset);
            index+=NVM_PAGE_SIZE;
            write_offset = 0;
        }else{
            len-=NVM_PAGE_SIZE;
            index+=NVM_PAGE_SIZE;
        }
        n_pages++;
        
        #if NVM_DEBUG
            for(int i=0; i<NVM_PAGE_SIZE; i++){
                ttprintf("%02X:%c ", page_content[i], page_content[i]);   
            }
            ttprintf("\r\n");
            ttprintf("page: %u addr: %04X n_pages %u write_len: %u\r\n", page, page_addr, n_pages, write_len);
        #endif

    }
    
    return pdPASS;
}

void VMS_init_blk(VMS_BLOCK* blk){
    blk->uid = 0;
    blk->nextBlocks[0] = NO_BLK;
    blk->nextBlocks[1] = NO_BLK;
    blk->nextBlocks[2] = NO_BLK;
    blk->nextBlocks[3] = NO_BLK;
    blk->behavior = NORMAL;
    blk->type = VMS_LIN;
    blk->target = maxOnTime;
    blk->thresholdDirection = RISING;
    blk->targetFactor = 0;
    blk->param1 = 0;
    blk->param2 = 0;
    blk->param3 = 0;
    blk->period = 0;
    blk->flags = 0; 
}

void VMS_print_blk(TERMINAL_HANDLE* handle, VMS_BLOCK* blk, uint8_t indent){
    ttprintf("%*sBlock ID: %u @ 0x%08X\r\n", indent, "", blk->uid, blk);
    indent++;
    for(int i=0;i<VMS_MAX_BRANCHES;i++){
        if(blk->nextBlocks[i] == NO_BLK){
            ttprintf("%*sNext %i: No Block\r\n", indent, "", i);   
        }else if(blk->nextBlocks[i] < (VMS_BLOCK*)4096){
            ttprintf("%*sNext %i: ID %u\r\n", indent, "", i, blk->nextBlocks[i]);
        }else{
            ttprintf("%*sNext %i: 0x%08X\r\n", indent, "", i, blk->nextBlocks[i]);
        }
    }
    
    if(blk->offBlock == NO_BLK){
        ttprintf("%*soffBlock: No Block\r\n", indent, "");   
    }else if(blk->offBlock < (VMS_BLOCK*)4096){
        ttprintf("%*soffBlock: ID %u\r\n", indent, "", blk->offBlock);
    }else{
        ttprintf("%*soffBlock: 0x%08X\r\n", indent, "", blk->offBlock);
    }
    

    ttprintf("%*sBehavior: ", indent, "");
    switch(blk->behavior){
        case NORMAL:
            ttprintf("NORMAL");
        break;
        case INVERTED:
            ttprintf("INVERTED");
        break;    
    }
    
    ttprintf("\r\n%*sType: ", indent, "");
    switch(blk->type){
        case VMS_LIN:
            ttprintf("LIN");
        break;
        case VMS_EXP_INV:
            ttprintf("EXP INV");
        break;  
        case VMS_EXP:
            ttprintf("EXP");
        break; 
        case VMS_SIN:
            ttprintf("SIN");
        break;
        case VMS_JUMP:
            ttprintf("JUMP");
        break;
    }
    
    ttprintf("\r\n%*sTarget: ", indent, "");
    switch(blk->target){
        case onTime:
            ttprintf("onTime");
        break;
        case maxOnTime:
            ttprintf("maxOnTime");
        break;  
        case minOnTime:
            ttprintf("minOnTime");
        break;  
        case otCurrent:
            ttprintf("otCurrent");
        break;
        case otTarget:
            ttprintf("otTarget");
        break;
        case otFactor:
            ttprintf("otFactor");
        break;
        case frequency:
            ttprintf("frequency");
        break;
        case freqCurrent:
            ttprintf("freqCurrent");
        break;
        case freqTarget:
            ttprintf("freqTarget");
        break;
        case freqFactor:
            ttprintf("freqFactor");
        break;
        case noise:
            ttprintf("noise");
        break;
        case pTime:
            ttprintf("pTime");
        break;
        case circ1:
            ttprintf("circ1");
        break;
        case circ2:
            ttprintf("circ2");
        break;
        case circ3:
            ttprintf("circ3");
        break;
        case circ4:
            ttprintf("circ4");
        break;
        case CC_102 ... CC_119:
            ttprintf("CC_102 ... CC_119");
        break;
        case KNOWNVAL_MAX:
            ttprintf("KNOWNVAL_MAX");
        break;
        case HyperVoice_Count:
            ttprintf("Hypervoice_Count");
        break;
        case HyperVoice_Phase:
            ttprintf("Hypervoice_Phase");
        break;
    }
    
    ttprintf("\r\n%*sThreshold Direction: ", indent, "");
    switch(blk->thresholdDirection){
        case RISING:
            ttprintf("RISING");
        break;
        case FALLING:
            ttprintf("FALLING");
        break;
        case ANY:
            ttprintf("ANY");
        break;
        case NONE:
            ttprintf("NONE");
        break;
    }
    
    ttprintf("\r\n%*sTarget factor: %i\r\n", indent, "", blk->targetFactor);
    ttprintf("%*sParam 1: %i\r\n", indent, "", blk->param1);
    ttprintf("%*sParam 2: %i\r\n", indent, "", blk->param2);
    ttprintf("%*sParam 3: %i\r\n", indent, "", blk->param3);
    ttprintf("%*sPeriod: %u\r\n", indent, "", blk->period);
    ttprintf("%*sFlags: 0x%08X\r\n\r\n", indent, "", blk->flags);
 
}

MAPTABLE_HEADER* VMS_print_map(TERMINAL_HANDLE* handle, MAPTABLE_HEADER* map){
    ttprintf("\r\nProgram: %u - %s\r\n", map->programNumber, map->name);
    MAPTABLE_ENTRY* ptr = (MAPTABLE_ENTRY*)(map+1);
    for(uint32_t i=0;i<map->listEntries;i++){
        ttprintf("   Note range: %u - %u\r\n", ptr->startNote, ptr->endNote);
        ttprintf("   Note frequency: %u\r\n", ptr->data.noteFreq);
        ttprintf("   Target ontime : %u\r\n", ptr->data.targetOT);
        ttprintf("   Flags         : 0x%04x\r\n", ptr->data.flags);
        ttprintf("   Start block   : 0x%04x\r\n\r\n", ptr->data.VMS_Startblock);
        ptr++;
    }
    
    map = (MAPTABLE_HEADER*)ptr;
    if(map->listEntries){
        return map;
    }else{
        return NULL;
    }
}

uint32_t NVM_get_blk_cnt(const VMS_BLOCK* blk){
    uint32_t cnt=0;
    while(1){
        if(blk->uid==0) break;
        blk++;
        cnt++;
    }
    return cnt;
}


uint8_t CMD_nvm(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args) {
    if(argCount==0 || strcmp(args[0], "-?") == 0){
        ttprintf("nvm [maps|blocks]\r\n");
        return TERM_CMD_EXIT_SUCCESS;
    }
    
    if(strcmp(args[0], "maps") == 0){
        MAPTABLE_HEADER* map = (MAPTABLE_HEADER*) NVM_mapMem;
    
        while(map){
            map = VMS_print_map(handle, map);
        }
        return TERM_CMD_EXIT_SUCCESS;
    }
    
    if(strcmp(args[0], "blocks") == 0){
        uint32_t n_blocks = NVM_get_blk_cnt(VMS_BLKS);
        ttprintf("NVM block count: %u\r\n", n_blocks);
        
        for(uint32_t i=0;i<n_blocks;i++){
            VMS_print_blk(handle, (VMS_BLOCK*)&VMS_BLKS[i], 4);
        }
        return TERM_CMD_EXIT_SUCCESS;
    }
    
    return TERM_CMD_EXIT_SUCCESS; 
}