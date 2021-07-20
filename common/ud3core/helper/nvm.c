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
    const volatile uint8_t * NVM_blockMem = (uint8_t*)NVM_START_ADDR;
    const VMS_BLOCK* VMS_BLKS = (VMS_BLOCK*)(NVM_START_ADDR + BLOCKMEM_SIZE);
#else
    uint8_t mem[0xFFFF];
    uint8_t * NVM_mapMem = (uint8_t*)mem;
    uint8_t * NVM_blockMem = (uint8_t*)(mem + BLOCKMEM_SIZE);
    VMS_BLOCK* VMS_BLKS = (VMS_BLOCK*)(mem + BLOCKMEM_SIZE);
#endif

void nvm_init(){
   
    TERM_addCommand(CMD_nvm, "nvm","NVM-Test Func",0,&TERM_cmdListHead);  
}

#ifndef SIMULATOR
uint8_t nvm_write_buffer(TERMINAL_HANDLE * handle, uint16_t index, uint8_t* buffer, int32_t len){

    uint16_t n_pages = 0;

    uint8_t *page_content;
    page_content = pvPortMalloc(NVM_PAGE_SIZE);
    if(page_content == NULL) return pdFAIL;
    
    uint32_t write_offset = index % NVM_PAGE_SIZE;

    while(len>0){
        uint8_t * page_addr = (uint8_t*)NVM_mapMem + ((index / NVM_PAGE_SIZE)*NVM_PAGE_SIZE); 
        uint16_t page = NVM_START_PAGE + (index / NVM_PAGE_SIZE);
        if(page > 256) return pdFAIL;
        
        memcpy(page_content, page_addr , NVM_PAGE_SIZE);

        uint32_t write_len = len;
        if(write_len+write_offset > NVM_PAGE_SIZE){
            write_len = NVM_PAGE_SIZE-write_offset;   
        }
        
        memcpy(page_content + write_offset, buffer , write_len);
        
        CySetTemp();
        uint32_t rc = CyWriteRowData((uint8)NVM_ARRAY, page, page_content);
        CyFlushCache();
        
        if(rc) return pdFAIL;
        
        
        
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
    
    vPortFree(page_content);
    return pdPASS;
}
#else
uint8_t nvm_write_buffer(TERMINAL_HANDLE * handle, uint16_t index, uint8_t* buffer, int32_t len){
    
    memcpy(&NVM_mapMem[index], buffer, len);
    
    return pdPASS;
}   
#endif

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
    
    ttprintf("\r\n%*sTarget factor: %u\r\n", indent, "", blk->targetFactor);
    ttprintf("%*sParam 1: %u\r\n", indent, "", blk->param1);
    ttprintf("%*sParam 2: %u\r\n", indent, "", blk->param2);
    ttprintf("%*sParam 3: %u\r\n", indent, "", blk->param3);
    ttprintf("%*sPeriod: %u\r\n", indent, "", blk->period);
    ttprintf("%*sFlags: 0x%08X\r\n\r\n", indent, "", blk->flags);
 
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
    
    //if(argCount == 0) return TERM_CMD_EXIT_SUCCESS;
    uint32_t index = BLOCKMEM_SIZE;
    
    VMS_BLOCK* temp = pvPortMalloc(sizeof(VMS_BLOCK)*4);
    
    ttprintf("Size of %u\r\n", sizeof(VMS_BLOCK));
    
    VMS_init_blk(&temp[0]);
    VMS_init_blk(&temp[1]);
    VMS_init_blk(&temp[2]);
    VMS_init_blk(&temp[3]);
    
    temp[0].uid = 1;
    temp[0].nextBlocks[0] = HARD_BLK(1);
    temp[0].nextBlocks[1] = HARD_BLK(2);
    temp[0].behavior = NORMAL;
    temp[0].type = VMS_LIN;
    temp[0].target = maxOnTime;
    temp[0].thresholdDirection = RISING;
    temp[0].targetFactor = 1;
    temp[0].param1 = 1;
    temp[0].param2 = 2;
    temp[0].param3 = 3;
    temp[0].period = 4;
    temp[0].flags = 0;
    
    temp[1].uid = 2;
    temp[1].nextBlocks[0] = &temp[0];
    temp[1].nextBlocks[1] = &temp[2];
    temp[1].behavior = NORMAL;
    temp[1].type = VMS_SIN;
    temp[1].target = maxOnTime;
    temp[1].thresholdDirection = FALLING;
    temp[1].targetFactor = 1;
    temp[1].param1 = 5;
    temp[1].param2 = 6;
    temp[1].param3 = 7;
    temp[1].period = 8;
    temp[1].flags = 0;
    
    temp[2].uid = 3;
    temp[2].behavior = NORMAL;
    temp[2].type = VMS_LIN;
    temp[2].target = maxOnTime;
    temp[2].thresholdDirection = RISING;
    temp[2].targetFactor = 1;
    temp[2].param1 = 6;
    temp[2].param2 = 7;
    temp[2].param3 = 8;
    temp[2].period = 9;
    temp[2].flags = 0;
    
    temp[3].uid = 4;
    temp[3].behavior = INVERTED;
    temp[3].type = VMS_EXP;
    temp[3].target = maxOnTime;
    temp[3].thresholdDirection = FALLING;
    temp[3].targetFactor = 1;
    temp[3].param1 = 6;
    temp[3].param2 = 7;
    temp[3].param3 = 8;
    temp[3].period = 9;
    temp[3].flags = 0xFF;
    
    VMS_print_blk(handle, &temp[0], 0);
    VMS_print_blk(handle, &temp[1], 0);
    VMS_print_blk(handle, &temp[2], 0);
    VMS_print_blk(handle, &temp[3], 0);
    

    nvm_write_buffer(handle, index, (uint8_t*)temp, sizeof(VMS_BLOCK)*4);
    
    uint32_t n_blocks = NVM_get_blk_cnt(VMS_BLKS);
    ttprintf("NVM MEM --> %u blocks:\r\n", n_blocks);
    
    for(uint32_t i=0;i<n_blocks;i++){
        VMS_print_blk(handle, &VMS_BLKS[i], 4);
    }
    vPortFree(temp);
    
    return TERM_CMD_EXIT_SUCCESS; 
}