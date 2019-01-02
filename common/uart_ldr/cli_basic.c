#include "cli_basic.h"

#include <project.h>

uint16_t byte_cnt;


#define EEPROM_READ_BYTE(x) EEPROM_1_ReadByte(x)
#define EEPROM_WRITE_ROW(x,y) EEPROM_1_Write(y,x)

uint32_t djb_hash(const char* cp)
{
    uint32_t hash = 5381;
    while (*cp)
        hash = 33 * hash ^ (unsigned char) *cp++;
    return hash;
}

void EEPROM_read_conf(parameter_entry * params, uint8_t param_size, uint16_t eeprom_offset ,uint8_t port){
    uint16_t addr=eeprom_offset;
    uint32_t temp_hash=0;
    uint8_t data[DATASET_BYTES];
    uint16_t change_count=0;
    uint8_t change_flag=0;
        for(int i=0;i<DATASET_BYTES;i++){
            data[i] = EEPROM_READ_BYTE(addr);
            addr++;
        }
        if(!(data[0]== 0x00 && data[1] == 0xC0 && data[2] == 0xFF && data[3] == 0xEE)) {
            return;
        }

    while(addr<CY_EEPROM_SIZE){
        change_flag=0;
        for(int i=0;i<DATASET_BYTES;i++){
            data[i] = EEPROM_READ_BYTE(addr);
            addr++;
        }
        if(data[0]== 0xDE && data[1] == 0xAD && data[2] == 0xBE && data[3] == 0xEF) break;
        uint8_t current_parameter=0;
        for ( current_parameter = 0; current_parameter < param_size; current_parameter++) {
            if(params[current_parameter].parameter_type==PARAM_CONFIG){
                temp_hash=djb_hash(params[current_parameter].name);
                if((uint8_t)temp_hash == data[0] && (uint8_t)(temp_hash>>8) == data[1]&& (uint8_t)(temp_hash>>16) == data[2]&& (uint8_t)(temp_hash>>24) == data[3]){
                    for(uint8_t i=0;i<data[4];i++){
                        *(i+(uint8_t * )params[current_parameter].value) = EEPROM_READ_BYTE(addr);
                        addr++;
                    }
                    change_count++;
                    change_flag=1;
                    break;
                }
            }
        }
        if(!change_flag) addr+=data[4];
       

    }
 }


