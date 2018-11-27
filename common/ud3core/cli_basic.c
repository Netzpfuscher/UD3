#include "cli_basic.h"
#include <stdio.h>
#include <stdlib.h>
#include <device.h>
#ifndef BOOT
#include "tasks/tsk_uart.h"
#include "tasks/tsk_usb.h"
#include "tasks/tsk_eth.h"
#endif

uint8_t EEPROM_Read_Row(uint8_t row, uint8_t * buffer);
uint8_t EEPROM_1_Write_Row(uint8_t row, uint8_t * buffer);

uint16_t byte_cnt;

#define EEPROM_READ_BYTE(x) EEPROM_1_ReadByte(x)
#define EEPROM_WRITE_ROW(x,y) EEPROM_1_Write(y,x)

uint8_t updateDefaultFunction(parameter_entry * params, char * newValue, uint8_t index, port_str *ptr) {
    char buffer[60];
    int32_t value;
    float fvalue;
    char* ch_ptr;
    switch (params[index].type){
    case TYPE_UNSIGNED:
        if(params[index].div){
            fvalue=strtof(newValue,&ch_ptr);
            value = fvalue*params[index].div;
        }else{
            value = strtoul(newValue,&ch_ptr,10);
        }
        if (value >= params[index].min && value <= params[index].max){
            if(params[index].size==1){
                *(uint8_t*)params[index].value = value;
                return 1;
            }
            if(params[index].size==2){
                *(uint16_t*)params[index].value = value;
                return 1;
            }
            if(params[index].size==4){
                *(uint32_t*)params[index].value = value;
                return 1;
            }
        }else{
            goto error;
        }
        break;
    case TYPE_SIGNED:
        if(params[index].div){
            fvalue=strtof(newValue,&ch_ptr);
            value = fvalue*params[index].div;
        }else{
            value = strtol(newValue,&ch_ptr,10);
        }
        if (value >= params[index].min && value <= params[index].max){
            if(params[index].size==1){
                *(int8_t*)params[index].value = value;
                return 1;
            }
            if(params[index].size==2){
                *(int16_t*)params[index].value = value;
                return 1;
            }
            if(params[index].size==4){
                *(int32_t*)params[index].value = value;
                return 1;
            }
        }else{
            goto error;
        }
        break;
    case TYPE_FLOAT:
            if(params[index].size==4){
                fvalue=strtof(newValue,&ch_ptr);
                if (fvalue >= (float)params[index].min && fvalue <= (float)params[index].max){
                    *(float*)params[index].value = fvalue;
                    return 1;
                }else{
                    goto error;
                }
            }
        break;
    case TYPE_CHAR:
        *(char*)params[index].value = *newValue;
        break;
    case TYPE_STRING:
        for(int i=0;i<params[index].size;i++){
            *(i+(char*)params[index].value) = newValue[i];
            if(!newValue[i]) break;
        }
        return 1;
        break;

    }
    return 0;
    error:
        if(params[index].div){
            sprintf(buffer, "E:Range %i.%u-%i.%u\r\n",
                params[index].min/params[index].div,
                params[index].min%params[index].div,                
                params[index].max/params[index].div,
                params[index].max%params[index].div);
        }else{
            sprintf(buffer, "E:Range %i-%i\r\n", params[index].min, params[index].max);
        }
        send_string(buffer,ptr);

    return 0;
}

void print_param_helperfunc(parameter_entry * params, uint8_t param_size, port_str *ptr, uint8_t param_type){
    char buffer[100];
    #define COL_A 9
    #define COL_B 33
    #define COL_C 64
    uint8_t current_parameter;
    uint32_t u_temp_buffer=0;
    int32_t i_temp_buffer=0;
    Term_Move_cursor_right(COL_A,ptr);
    send_string("Parameter", ptr);
    Term_Move_cursor_right(COL_B,ptr);
    send_string("| Value", ptr);
    Term_Move_cursor_right(COL_C,ptr);
    send_string("| Text\r\n", ptr);
    for (current_parameter = 0; current_parameter < param_size; current_parameter++) {
        if(params[current_parameter].parameter_type==param_type){
            Term_Move_cursor_right(COL_A,ptr);
            sprintf(buffer, "\033[36m%s", params[current_parameter].name);
            send_string(buffer, ptr);

            switch (params[current_parameter].type){
            case TYPE_UNSIGNED:
                switch (params[current_parameter].size){
                case 1:
                    u_temp_buffer = *(uint8_t*)params[current_parameter].value;
                    break;
                case 2:
                    u_temp_buffer = *(uint16_t*)params[current_parameter].value;
                    break;
                case 4:
                    u_temp_buffer = *(uint32_t*)params[current_parameter].value;
                    break;
                }

                Term_Move_cursor_right(COL_B,ptr);
                if(params[current_parameter].div){
                    sprintf(buffer, "\033[37m| \033[32m%u.%u", (u_temp_buffer/params[current_parameter].div),(u_temp_buffer%params[current_parameter].div));
                }else{
                    sprintf(buffer, "\033[37m| \033[32m%u", u_temp_buffer);
                }
                send_string(buffer, ptr);

                break;
            case TYPE_SIGNED:
                switch (params[current_parameter].size){
                case 1:
                    i_temp_buffer = *(int8_t*)params[current_parameter].value;
                    break;
                case 2:
                    i_temp_buffer = *(int16_t*)params[current_parameter].value;
                    break;
                case 4:
                    i_temp_buffer = *(int32_t*)params[current_parameter].value;
                    break;
                }

                Term_Move_cursor_right(COL_B,ptr);
                if(params[current_parameter].div){
                    uint32_t mod;
                    if(i_temp_buffer<0){
                        mod=(i_temp_buffer*-1)%params[current_parameter].div;
                    }else{
                        mod=i_temp_buffer%params[current_parameter].div;
                    }
                    sprintf(buffer, "\033[37m| \033[32m%i.%u", (i_temp_buffer/params[current_parameter].div),mod);
                }else{
                    sprintf(buffer, "\033[37m| \033[32m%i", i_temp_buffer);
                }
                send_string(buffer, ptr);

                break;
            case TYPE_FLOAT:

                Term_Move_cursor_right(COL_B,ptr);
                sprintf(buffer, "\033[37m| \033[32m%f", *(float*)params[current_parameter].value);
                send_string(buffer, ptr);

                break;
            case TYPE_CHAR:

                Term_Move_cursor_right(COL_B,ptr);
                sprintf(buffer, "\033[37m| \033[32m%c", *(char*)params[current_parameter].value);
                send_string(buffer, ptr);

                break;
            case TYPE_STRING:

                Term_Move_cursor_right(COL_B,ptr);
                sprintf(buffer, "\033[37m| \033[32m%s", (char*)params[current_parameter].value);
                send_string(buffer, ptr);

                break;

            }
            Term_Move_cursor_right(COL_C,ptr);
            sprintf(buffer, "\033[37m| %s\r\n", params[current_parameter].help);
            send_string(buffer, ptr);
        }
    }
}

void print_param_help(parameter_entry * params, uint8_t param_size, port_str *ptr){
    send_string("Parameters:\r\n", ptr);
    print_param_helperfunc(params, param_size, ptr,PARAM_DEFAULT);
    send_string("\r\nConfiguration:\r\n", ptr);
    print_param_helperfunc(params, param_size, ptr,PARAM_CONFIG);
}

void print_param(parameter_entry * params, uint8_t index, port_str *ptr){
    char buffer[100];
    uint32_t u_temp_buffer=0;
    int32_t i_temp_buffer=0;
    float f_temp_buffer=0.0;
    switch (params[index].type){
        case TYPE_UNSIGNED:
            switch (params[index].size){
                case 1:
                    u_temp_buffer = *(uint8_t*)params[index].value;
                    break;
                case 2:
                    u_temp_buffer = *(uint16_t*)params[index].value;
                    break;
                case 4:
                    u_temp_buffer = *(uint32_t*)params[index].value;
                    break;
            }
            if(params[index].div){
                sprintf(buffer, "\t%s=%u.%u\r\n", params[index].name,(u_temp_buffer/params[index].div),(u_temp_buffer%params[index].div));
            }else{
                sprintf(buffer, "\t%s=%u\r\n", params[index].name,u_temp_buffer);
            }
            send_string(buffer, ptr);
            break;
        case TYPE_SIGNED:
            switch (params[index].size){
            case 1:
                i_temp_buffer = *(int8_t*)params[index].value;
                break;
            case 2:
                i_temp_buffer = *(int16_t*)params[index].value;
                break;
            case 4:
                i_temp_buffer = *(int32_t*)params[index].value;
                break;
            }
            if(params[index].div){
                uint32_t mod;
                if(i_temp_buffer<0){
                    mod=(i_temp_buffer*-1)%params[index].div;
                }else{
                    mod=i_temp_buffer%params[index].div;
                }
                sprintf(buffer, "\t%s=%i.%u\r\n", params[index].name,(i_temp_buffer/params[index].div),mod);
            }else{
                sprintf(buffer, "\t%s=%i\r\n", params[index].name,i_temp_buffer);
            }
	        send_string(buffer, ptr);
            break;
        case TYPE_FLOAT:
            f_temp_buffer = *(float*)params[index].value;
            sprintf(buffer, "\t%s=%f\r\n", params[index].name,f_temp_buffer);
            send_string(buffer, ptr);
            break;
        case TYPE_CHAR:
            sprintf(buffer, "\t%s=%c\r\n", params[index].name,*(char*)params[index].value);
            send_string(buffer, ptr);
            break;
        case TYPE_STRING:
            sprintf(buffer, "\t%s=%s\r\n", params[index].name,(char*)params[index].value);
            send_string(buffer, ptr);
            break;
        }
}

void print_param_buffer(char * buffer, parameter_entry * params, uint8_t index){
    uint32_t u_temp_buffer=0;
    int32_t i_temp_buffer=0;
    float f_temp_buffer=0.0;
    switch (params[index].type){
        case TYPE_UNSIGNED:
            switch (params[index].size){
                case 1:
                    u_temp_buffer = *(uint8_t*)params[index].value;
                    break;
                case 2:
                    u_temp_buffer = *(uint16_t*)params[index].value;
                    break;
                case 4:
                    u_temp_buffer = *(uint32_t*)params[index].value;
                    break;
            }
            if(params[index].div){
                sprintf(buffer, "%s;%u.%u;%u;%u;%i.%u;%i.%u",
                    params[index].name,
                    (u_temp_buffer/params[index].div),
                    (u_temp_buffer%params[index].div),
                    params[index].type,
                    params[index].size,
                    (params[index].min/params[index].div),
                    (params[index].min%params[index].div),
                    (params[index].max/params[index].div),
                    (params[index].max%params[index].div));
            }else{
                sprintf(buffer, "%s;%u;%u;%u;%u;%u",
                    params[index].name,
                    u_temp_buffer,
                    params[index].type,
                    params[index].size,
                    params[index].min,
                    params[index].max
                );
            }
            break;
        case TYPE_SIGNED:
            switch (params[index].size){
            case 1:
                i_temp_buffer = *(int8_t*)params[index].value;
                break;
            case 2:
                i_temp_buffer = *(int16_t*)params[index].value;
                break;
            case 4:
                i_temp_buffer = *(int32_t*)params[index].value;
                break;
            }
            if(params[index].div){
                sprintf(buffer, "%s;%i.%u;%u;%u;%i.%u;%i.%u",
                    params[index].name,
                    (i_temp_buffer/params[index].div),
                    i_temp_buffer%params[index].div,
                    params[index].type,
                    params[index].size,
                    (params[index].min/params[index].div),
                    params[index].min%params[index].div,
                    (params[index].max/params[index].div),
                    params[index].max%params[index].div);
            }else{
                sprintf(buffer, "%s;%i;%u;%u;%i;%i",
                    params[index].name,
                    i_temp_buffer,
                    params[index].type,
                    params[index].size,
                    params[index].min,
                    params[index].max);
            }
            break;
        case TYPE_FLOAT:
            f_temp_buffer = *(float*)params[index].value;
            sprintf(buffer, "%s;%f;%u;%u;%i;%i",
                params[index].name,
                f_temp_buffer,
                params[index].type,
                params[index].size,
                params[index].min,
                params[index].max);
            break;
        case TYPE_CHAR:
            sprintf(buffer, "%s;%c;%u;%u;%s;%s",
                params[index].name,
                *(char*)params[index].value,
                params[index].type,
                params[index].size,
                "NULL",
                "NULL");
            break;
        case TYPE_STRING:
            sprintf(buffer, "%s;%s;%u;%u;%s;%s",
                params[index].name,
                (char*)params[index].value,
                params[index].type,
                params[index].size,
                "NULL",
                "NULL");
            break;
        }
}


uint8_t EEPROM_Read_Row(uint8_t row, uint8_t * buffer){
    uint16_t addr;
    addr = row * CY_EEPROM_SIZEOF_ROW;
    for(uint8_t i = 0; i<CY_EEPROM_SIZEOF_ROW;i++){
        *buffer=EEPROM_READ_BYTE(addr+i);
        buffer++;
    }
    return 1;
}

uint8_t EEPROM_buffer_write(uint8_t byte, uint16_t address, uint8_t flush){
    byte_cnt++;
    static uint8_t eeprom_buffer[CY_EEPROM_SIZEOF_ROW];
    static uint16_t last_row = 0xFFFF;
    static uint8_t changed=0;
    uint8_t ret_change=0;
    uint16_t rowNumber;
    uint16_t byteNumber;
    rowNumber = address/(uint16_t)CY_EEPROM_SIZEOF_ROW;
    byteNumber = address - (rowNumber * ((uint16_t)CY_EEPROM_SIZEOF_ROW));
    if(last_row==0xFFFF){
        last_row=rowNumber;
        EEPROM_Read_Row(rowNumber,eeprom_buffer);
    }else if(last_row!=rowNumber){
        if(changed){
            EEPROM_WRITE_ROW(last_row,eeprom_buffer);
        }
        EEPROM_Read_Row(rowNumber,eeprom_buffer);
        changed=0;
    }
    if(eeprom_buffer[byteNumber]!=byte){
       eeprom_buffer[byteNumber]=byte;
       changed=1;
       ret_change=1;
    }

    last_row = rowNumber;

    if(flush && changed){
        last_row = 0xFFFF;
        EEPROM_WRITE_ROW(rowNumber,eeprom_buffer);
        changed = 0;
    }else if(flush){
        last_row = 0xFFFF;
    }
    return ret_change;
}

uint32_t djb_hash(const char* cp)
{
    uint32_t hash = 5381;
    while (*cp)
        hash = 33 * hash ^ (unsigned char) *cp++;
    return hash;
}

void EEPROM_write_conf(parameter_entry * params, uint8_t param_size, uint16_t eeprom_offset ,port_str *ptr){
    byte_cnt=0;
	uint16_t count = eeprom_offset;
	uint8_t change_flag = 0;
	uint16_t change_count = 0;
	uint32_t temp_hash=0;
	uint16_t param_count=0;
	char buffer[44];
        EEPROM_buffer_write(0x00, count,0);
		count++;
		EEPROM_buffer_write(0xC0, count,0);
		count++;
		EEPROM_buffer_write(0xFF, count,0);
		count++;
		EEPROM_buffer_write(0xEE, count,0);
		count++;
		EEPROM_buffer_write(0x00, count,0);
		count++;
		for (uint8_t  current_parameter = 0; current_parameter < param_size; current_parameter++) {
            if(params[current_parameter].parameter_type == PARAM_CONFIG){
                param_count++;
                change_flag = 0;
                temp_hash=djb_hash(params[current_parameter].name);
                EEPROM_buffer_write(temp_hash, count ,0);
                count++;
                EEPROM_buffer_write(temp_hash>>8,count,0);
                count++;
                EEPROM_buffer_write(temp_hash>>16,count,0);
                count++;
                EEPROM_buffer_write(temp_hash>>24,count,0);
                count++;
                EEPROM_buffer_write(params[current_parameter].size,count,0);
                count++;
                for(uint8_t i=0;i<params[current_parameter].size;i++){
                    change_flag |= EEPROM_buffer_write(*(i+(uint8_t *)params[current_parameter].value), count,0);
                    count++;
                }
                if (change_flag) {
                    change_count++;
                }
            }
		}
		EEPROM_buffer_write(0xDE, count,0);
		count++;
		EEPROM_buffer_write(0xAD, count,0);
		count++;
		EEPROM_buffer_write(0xBE, count,0);
		count++;
		EEPROM_buffer_write(0xEF, count,0);
        count++;
		EEPROM_buffer_write(0x00, count,1);
		sprintf(buffer, "%i / %i new config params written. %i bytes from 2048 used.\r\n", change_count, param_count, byte_cnt);
        send_string(buffer, ptr);
}

void EEPROM_read_conf(parameter_entry * params, uint8_t param_size, uint16_t eeprom_offset ,port_str *ptr){
    char buffer[50];
    uint16_t addr=eeprom_offset;
    uint32_t temp_hash=0;
    uint8_t data[DATASET_BYTES];
    uint16_t param_count=0;
    uint16_t change_count=0;
    uint8_t change_flag=0;
        for(int i=0;i<DATASET_BYTES;i++){
            data[i] = EEPROM_READ_BYTE(addr);
            addr++;
        }
        if(!(data[0]== 0x00 && data[1] == 0xC0 && data[2] == 0xFF && data[3] == 0xEE)) {
            #ifndef BOOT
            send_string("WARNING: No or old EEPROM dataset found\r\n",ptr);
            #endif
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
        
        if(current_parameter == param_size){
            #ifndef BOOT
            sprintf(buffer,"WARNING: Unknown param ID %i found in EEPROM\r\n", data[0]);
            send_string(buffer, ptr);
            #endif
        }
    }
    uint8_t found_param=0;
    for (uint8_t current_parameter = 0; current_parameter < param_size; current_parameter++) {
        if(params[current_parameter].parameter_type==PARAM_CONFIG){
            param_count++;
            found_param=0;
            addr = DATASET_BYTES + eeprom_offset; //Skip header
            temp_hash=djb_hash(params[current_parameter].name);
            while(addr<CY_EEPROM_SIZE){
                for(int i=0;i<DATASET_BYTES;i++){
                    data[i] = EEPROM_READ_BYTE(addr);
                    addr++;
                }
                    addr += data[4];
                if(data[0] == 0xDE && data[1] == 0xAD && data[2] == 0xBE && data[3] == 0xEF) break;
                if((uint8_t)temp_hash == data[0] && (uint8_t)(temp_hash>>8) == data[1]&& (uint8_t)(temp_hash>>16) == data[2]&& (uint8_t)(temp_hash>>24) == data[3]){
                        found_param = 1;
                }
            }
            if(!found_param){
                //#ifndef BOOT
                sprintf(buffer,"WARNING: Param [%s] not found in EEPROM\r\n",params[current_parameter].name);
                send_string(buffer, ptr);
                //#endif
            }
        }
    }
    #ifndef BOOT
    sprintf(buffer, "%i / %i config params loaded\r\n", change_count, param_count);
    send_string(buffer, ptr);
    #endif
}

void Term_Erase_Screen(port_str *ptr) {
	send_string("\033[2J\033[1;1H", ptr);
}

void Term_Color_Green(port_str *ptr) {
	send_string("\033[32m", ptr);
}
void Term_Color_Red(port_str *ptr) {
	send_string("\033[31m", ptr);
}
void Term_Color_White(port_str *ptr) {
	send_string("\033[37m", ptr);
}
void Term_Color_Cyan(port_str *ptr) {
	send_string("\033[36m", ptr);
}
void Term_BGColor_Blue(port_str *ptr) {
	send_string("\033[44m", ptr);
}

void Term_Move_cursor_right(uint8_t column, port_str *ptr) {
	char buffer[10];
	sprintf(buffer, "\033[%i`", column);
	send_string(buffer, ptr);
}

void Term_Move_Cursor(uint8_t row, uint8_t column, port_str *ptr) {
	char buffer[44];
	sprintf(buffer, "\033[%i;%iH", row, column);
	send_string(buffer, ptr);
}

void Term_Box(uint8_t row1, uint8_t col1, uint8_t row2, uint8_t col2, port_str *ptr) {
	Term_Move_Cursor(row1, col1, ptr);
	Term_BGColor_Blue(ptr);
	send_string("\xE2\x95\x94", ptr); //edge upper left
	int i = 0;
	for (i = 1; i < (col2 - col1); i++) {
		send_string("\xE2\x95\x90", ptr); //=
	}
	send_string("\xE2\x95\x97", ptr); //edge upper right
	for (i = 1; i < (row2 - row1); i++) {
		Term_Move_Cursor(row1 + i, col1, ptr);
		send_string("\xE2\x95\x91", ptr); //left ||
		Term_Move_Cursor(row1 + i, col2, ptr);
		send_string("\xE2\x95\x91", ptr); //right ||
	}
	Term_Move_Cursor(row2, col1, ptr);
	send_string("\xE2\x95\x9A", ptr); //edge lower left
	for (i = 1; i < (col2 - col1); i++) {
		send_string("\xE2\x95\x90", ptr); //=
	}
	send_string("\xE2\x95\x9D", ptr); //edge lower right
	Term_Color_White(ptr);
}

void Term_Save_Cursor(port_str *ptr) {
	send_string("\033[s", ptr);
}
void Term_Restore_Cursor(port_str *ptr) {
	send_string("\033[u", ptr);
}

/********************************************
* Sends char to transmit queue
*********************************************/
void send_char(uint8 c, port_str *ptr) {
#ifndef BOOT
    switch(ptr->type){
        case PORT_TYPE_NULL:
        break;
        case PORT_TYPE_SERIAL:
            if (xUART_tx != NULL)
                xStreamBufferSend(xUART_tx,&c, 1,portMAX_DELAY);
        break;
        case PORT_TYPE_USB:
            if (qUSB_tx != NULL)
			    xQueueSend(qUSB_tx, &c, portMAX_DELAY);
        break;
        case PORT_TYPE_ETH:
            if (xETH_tx[ptr->num] != NULL) {
                xStreamBufferSend(xETH_tx[ptr->num],&c, 1,200 /portTICK_RATE_MS);
		    }
        break;
    }
#endif
}

/********************************************
* Sends string to transmit queue
*********************************************/
void send_string(char *data, port_str *ptr) {
#ifndef BOOT
    switch(ptr->type){
        case PORT_TYPE_NULL:
        break;
        case PORT_TYPE_SERIAL:
            if (xUART_tx != NULL) {
                    xStreamBufferSend(xUART_tx,data, strlen(data),portMAX_DELAY);
		    }
        break;
        case PORT_TYPE_USB:
            if (qUSB_tx != NULL) {
			    while ((*data) != '\0') {
				    if (xQueueSend(qUSB_tx, data, portMAX_DELAY))
					    data++;
			    }
		    }
        break;
        case PORT_TYPE_ETH:
            if (xETH_tx[ptr->num] != NULL) {
                xStreamBufferSend(xETH_tx[ptr->num],data, strlen(data),200 /portTICK_RATE_MS);
    		}
        break;
    }

#endif
}
/********************************************
* Sends buffer to transmit queue
*********************************************/
void send_buffer(uint8_t *data, uint16_t len, port_str *ptr) {
#ifndef BOOT
    switch(ptr->type){
        case PORT_TYPE_NULL:
        break;
        case PORT_TYPE_SERIAL:
            if (xUART_tx != NULL) {
                xStreamBufferSend(xUART_tx,data, len,portMAX_DELAY);
		    }
        break;
        case PORT_TYPE_USB:
            if (qUSB_tx != NULL) {
			    while (len) {
				    if (xQueueSend(qUSB_tx, data, portMAX_DELAY))
					    data++;
				    len--;
			    }
		    }
        break;
        case PORT_TYPE_ETH:
            if (xETH_tx[ptr->num] != NULL) {
            xStreamBufferSend(xETH_tx[ptr->num],data, len,200 /portTICK_RATE_MS);
		}
        break;
    }
#endif
}
