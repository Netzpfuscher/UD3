#include "cli_basic.h"
#include <stdio.h>
#include <stdlib.h>
#include <device.h>
#include "tasks/tsk_uart.h"
#include "tasks/tsk_usb.h"

uint8_t EEPROM_Read_Row(uint8_t row, uint8_t * buffer);
uint8_t EEPROM_1_Write_Row(uint8_t row, uint8_t * buffer);


#define EEPROM_READ_BYTE(x) EEPROM_1_ReadByte(x)
#define EEPROM_WRITE_ROW(x,y) EEPROM_1_Write(y,x)


uint8_t updateDefaultFunction(parameter_entry * params, char * newValue, uint8_t index, uint8_t port) {
    char buffer[60];
    uint32_t value;
    float fvalue;
    char* ptr;
    switch (params[index].type){
    case TYPE_UNSIGNED:
        value = strtoul(newValue,&ptr,10);
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
        value = strtol(newValue,&ptr,10);
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
                fvalue=strtof(newValue,&ptr);
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
        sprintf(buffer, "E:Range %i-%i\r\n", params[index].min, params[index].max);
        send_string(buffer,port);

    return 0;
}

void print_param_helperfunc(parameter_entry * params, uint8_t param_size, uint8_t port, uint8_t param_type){
    char buffer[100];
    #define COL_A 9
    #define COL_B 33
    #define COL_C 49
    uint8_t current_parameter;
    uint32_t u_temp_buffer=0;
    int32_t i_temp_buffer=0;
    Term_Move_cursor_right(COL_A,port);
    send_string("Parameter", port);
    Term_Move_cursor_right(COL_B,port);
    send_string("| Value", port);
    Term_Move_cursor_right(COL_C,port);
    send_string("| Text\r\n", port);
    for (current_parameter = 0; current_parameter < param_size; current_parameter++) {
        if(params[current_parameter].parameter_type==param_type){
            Term_Move_cursor_right(COL_A,port);
            sprintf(buffer, "\033[36m%s", params[current_parameter].name);
            send_string(buffer, port);

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

                Term_Move_cursor_right(COL_B,port);
                sprintf(buffer, "\033[37m| \033[32m%u", u_temp_buffer);
                send_string(buffer, port);

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

                Term_Move_cursor_right(COL_B,port);
                sprintf(buffer, "\033[37m| \033[32m%i", i_temp_buffer);
                send_string(buffer, port);

                break;
            case TYPE_FLOAT:

                Term_Move_cursor_right(COL_B,port);
                sprintf(buffer, "\033[37m| \033[32m%f", *(float*)params[current_parameter].value);
                send_string(buffer, port);

                break;
            case TYPE_CHAR:

                Term_Move_cursor_right(COL_B,port);
                sprintf(buffer, "\033[37m| \033[32m%c", *(char*)params[current_parameter].value);
                send_string(buffer, port);

                break;
            case TYPE_STRING:

                Term_Move_cursor_right(COL_B,port);
                sprintf(buffer, "\033[37m| \033[32m%s", (char*)params[current_parameter].value);
                send_string(buffer, port);

                break;

            }
            Term_Move_cursor_right(COL_C,port);
            sprintf(buffer, "\033[37m| %s\r\n", params[current_parameter].help);
            send_string(buffer, port);
        }
    }
}

void print_param_help(parameter_entry * params, uint8_t param_size, uint8_t port){
    send_string("Parameters:\r\n", port);
    print_param_helperfunc(params, param_size, port,PARAM_DEFAULT);
    send_string("\r\nConfiguration:\r\n", port);
    print_param_helperfunc(params, param_size, port,PARAM_CONFIG);
}

void print_param(parameter_entry * params, uint8_t index, uint8_t port){
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
            sprintf(buffer, "\t%s=%u\r\n", params[index].name,u_temp_buffer);
            send_string(buffer, port);
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
            sprintf(buffer, "\t%s=%i\r\n", params[index].name,i_temp_buffer);
	        send_string(buffer, port);
            break;
        case TYPE_FLOAT:
            f_temp_buffer = *(float*)params[index].value;
            sprintf(buffer, "\t%s=%f\r\n", params[index].name,f_temp_buffer);
            send_string(buffer, port);
            break;
        case TYPE_CHAR:
            sprintf(buffer, "\t%s=%c\r\n", params[index].name,*(char*)params[index].value);
            send_string(buffer, port);
            break;
        case TYPE_STRING:
            sprintf(buffer, "\t%s=%s\r\n", params[index].name,(char*)params[index].value);
            send_string(buffer, port);
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

void EEPROM_write_conf(parameter_entry * params, uint8_t param_size, uint16_t eeprom_offset ,uint8_t port){
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
		sprintf(buffer, "%i / %i new config params written\r\n", change_count, param_count);
        send_string(buffer, port);
}

void EEPROM_read_conf(parameter_entry * params, uint8_t param_size, uint16_t eeprom_offset ,uint8_t port){
    char buffer[50];
    uint16_t addr=eeprom_offset;
    uint32_t temp_hash=0;
    uint8_t data[DATASET_BYTES];
    uint16_t param_count=0;
    uint16_t change_count=0;
        for(int i=0;i<DATASET_BYTES;i++){
            data[i] = EEPROM_READ_BYTE(addr);
            addr++;
        }
        if(!(data[0]== 0x00 && data[1] == 0xC0 && data[2] == 0xFF && data[3] == 0xEE)) {
            send_string("WARNING: No or old EEPROM dataset found\r\n",port);
            return;
        }

    while(addr<CY_EEPROM_SIZE){
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
                    break;
                }
            }
        }
        if(current_parameter == param_size){
            sprintf(buffer,"WARNING: Unknown param ID %i found in EEPROM\r\n", data[0]);
            send_string(buffer, port);
        }
    }
    uint8_t found_param=0;
    for (uint8_t current_parameter = 0; current_parameter < param_size; current_parameter++) {
        if(params[current_parameter].parameter_type==PARAM_CONFIG){
            param_count++;
            found_param=0;
            addr = DATASET_BYTES + eeprom_offset; //Skip header
            while(addr<CY_EEPROM_SIZE){
                for(int i=0;i<DATASET_BYTES;i++){
                    data[i] = EEPROM_READ_BYTE(addr);
                    addr++;
                }
                    addr += data[4];
                if(data[0] == 0xDE && data[1] == 0xAD && data[2] == 0xBE && data[3] == 0xEF) break;
                temp_hash=djb_hash(params[current_parameter].name);
                if((uint8_t)temp_hash == data[0] && (uint8_t)(temp_hash>>8) == data[1]&& (uint8_t)(temp_hash>>16) == data[2]&& (uint8_t)(temp_hash>>24) == data[3]){
                        found_param = 1;
                }
            }
            if(!found_param){
                sprintf(buffer,"WARNING: Param [%s] not found in EEPROM\r\n",params[current_parameter].name);
                send_string(buffer, port);
            }
        }
    }
    sprintf(buffer, "%i / %i config params loaded\r\n", change_count, param_count);
    send_string(buffer, port);
}


void Term_Erase_Screen(uint8_t port) {
	send_string("\033[2J\033[1;1H", port);
}

void Term_Color_Green(uint8_t port) {
	send_string("\033[32m", port);
}
void Term_Color_Red(uint8_t port) {
	send_string("\033[31m", port);
}
void Term_Color_White(uint8_t port) {
	send_string("\033[37m", port);
}
void Term_Color_Cyan(uint8_t port) {
	send_string("\033[36m", port);
}
void Term_BGColor_Blue(uint8_t port) {
	send_string("\033[44m", port);
}

void Term_Move_cursor_right(uint8_t column, uint8_t port) {
	char buffer[10];
	sprintf(buffer, "\033[%i`", column);
	send_string(buffer, port);
}

void Term_Move_Cursor(uint8_t row, uint8_t column, uint8_t port) {
	char buffer[44];
	sprintf(buffer, "\033[%i;%iH", row, column);
	send_string(buffer, port);
}

void Term_Box(uint8_t row1, uint8_t col1, uint8_t row2, uint8_t col2, uint8_t port) {
	Term_Move_Cursor(row1, col1, port);
	Term_BGColor_Blue(port);
	send_string("\xE2\x95\x94", port); //edge upper left
	int i = 0;
	for (i = 1; i < (col2 - col1); i++) {
		send_string("\xE2\x95\x90", port); //=
	}
	send_string("\xE2\x95\x97", port); //edge upper right
	for (i = 1; i < (row2 - row1); i++) {
		Term_Move_Cursor(row1 + i, col1, port);
		send_string("\xE2\x95\x91", port); //left ||
		Term_Move_Cursor(row1 + i, col2, port);
		send_string("\xE2\x95\x91", port); //right ||
	}
	Term_Move_Cursor(row2, col1, port);
	send_string("\xE2\x95\x9A", port); //edge lower left
	for (i = 1; i < (col2 - col1); i++) {
		send_string("\xE2\x95\x90", port); //=
	}
	send_string("\xE2\x95\x9D", port); //edge lower right
	Term_Color_White(port);
}

void Term_Save_Cursor(uint8_t port) {
	send_string("\033[s", port);
}
void Term_Restore_Cursor(uint8_t port) {
	send_string("\033[u", port);
}


/********************************************
* Sends char to transmit queue
*********************************************/
void send_char(uint8 c, uint8_t port) {
	char buf[4];
	switch (port) {
	case USB:
		if (qUSB_tx != NULL)
			xQueueSend(qUSB_tx, &c, portMAX_DELAY);
		break;
	case SERIAL:
		buf[0] = c;
		buf[1] = '\0';
		if (qUart_tx != NULL)
			xQueueSend(qUart_tx, buf, portMAX_DELAY);
		break;
	}
}

/********************************************
* Sends string to transmit queue
*********************************************/
void send_string(char *data, uint8_t port) {

	switch (port) {
	case USB:
		if (qUSB_tx != NULL) {

			while ((*data) != '\0') {
				if (xQueueSend(qUSB_tx, data, portMAX_DELAY))
					data++;
			}
		}

		break;
	case SERIAL:
		if (qUart_tx != NULL) {
			while ((*data) != '\0') {
				if (xQueueSend(qUart_tx, data, portMAX_DELAY))
					data++;
			}
		}
		break;
	}
}
/********************************************
* Sends buffer to transmit queue
*********************************************/
void send_buffer(uint8_t *data, uint16_t len, uint8_t port) {

	switch (port) {
	case USB:
		if (qUSB_tx != NULL) {
			while (len) {
				if (xQueueSend(qUSB_tx, data, portMAX_DELAY))
					data++;
				len--;
			}
		}

		break;
	case SERIAL:
		if (qUart_tx != NULL) {
			while (len) {
				if (xQueueSend(qUart_tx, data, portMAX_DELAY))
					data++;
				len--;
			}
		}
		break;
	}
}



