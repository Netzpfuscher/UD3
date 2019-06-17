/*
 * UD3
 *
 * Copyright (c) 2018 Jens Kerrinnes
 * Copyright (c) 2015 Steve Ward
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

#include "cli_commands.h"
#include "ntlibc.h"
#include "telemetry.h"
#include "helper/printf.h"
#include "tasks/tsk_min.h"
#include "alarmevent.h"

/*****************************************************************************
* Clears the terminal screen and displays the logo
******************************************************************************/
uint8_t command_alarms(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    CHECK_NULL(commandline);

    char buffer[80];
    int ret=0;
    ALARMS temp;
    
    if (ntlibc_stricmp(commandline, "get") == 0) {
        static const uint8_t c_A = 9;
        static const uint8_t c_B = 20;
        static const uint8_t c_C = 40;
        Term_Move_cursor_right(c_A,ptr);
        SEND_CONST_STRING("Number", ptr);
        Term_Move_cursor_right(c_B,ptr);
        SEND_CONST_STRING("| Timestamp", ptr);
        Term_Move_cursor_right(c_C,ptr);
        SEND_CONST_STRING("| Message\r\n", ptr);
        
        for(uint16_t i=0;i<alarm_get_num();i++){
            if(alarm_get(i,&temp)==pdPASS){
                Term_Move_cursor_right(c_A,ptr);
                ret = snprintf(buffer, sizeof(buffer),"%u",temp.num);
                send_buffer((uint8_t*)buffer,ret,ptr); 
                
                Term_Move_cursor_right(c_B,ptr);
                ret = snprintf(buffer, sizeof(buffer),"| %u ms",temp.timestamp);
                send_buffer((uint8_t*)buffer,ret,ptr); 
                
                Term_Move_cursor_right(c_C,ptr);
                send_char('|',ptr);
                switch(temp.alarm_level){
                    case ALM_PRIO_INFO:
                        Term_Color_Green(ptr);
                    break;
                    case ALM_PRIO_WARN:
                        Term_Color_White(ptr);
                    break;
                    case ALM_PRIO_ALARM:
                        Term_Color_Cyan(ptr);
                    break;
                    case ALM_PRIO_CRITICAL:
                        Term_Color_Red(ptr);
                    break;
                }
                ret = snprintf(buffer, sizeof(buffer)," %s\r\n",temp.message);
                send_buffer((uint8_t*)buffer,ret,ptr); 
                Term_Color_White(ptr);
            }
        }
    	return 1;
    }
    if (ntlibc_stricmp(commandline, "reset") == 0) {
        alarm_clear();
        SEND_CONST_STRING("Alarms reset...\r\n", ptr);
        return 1;
    }
    
    HELP_TEXT("Usage: alarms [get|reset]\r\n");
}

/*****************************************************************************
* Displays the statistics of the min protocol
******************************************************************************/
uint8_t command_minstat(char *commandline, port_str *ptr){

    char buffer[60];
    int ret=0;
    ret = snprintf(buffer, sizeof(buffer),"Dropped frames        : %lu\r\n",min_ctx.transport_fifo.dropped_frames);
    send_buffer((uint8_t*)buffer,ret,ptr);
    ret = snprintf(buffer, sizeof(buffer),"Spurious acks         : %lu\r\n",min_ctx.transport_fifo.spurious_acks);
    send_buffer((uint8_t*)buffer,ret,ptr);
    ret = snprintf(buffer, sizeof(buffer),"Resets received       : %lu\r\n",min_ctx.transport_fifo.resets_received);
    send_buffer((uint8_t*)buffer,ret,ptr);
    ret = snprintf(buffer, sizeof(buffer),"Sequence mismatch drop: %lu\r\n",min_ctx.transport_fifo.sequence_mismatch_drop);
    send_buffer((uint8_t*)buffer,ret,ptr);
    ret = snprintf(buffer, sizeof(buffer),"Max frames in buffer  : %u\r\n",min_ctx.transport_fifo.n_frames_max);
    send_buffer((uint8_t*)buffer,ret,ptr);
    ret = snprintf(buffer, sizeof(buffer),"CRC errors            : %u\r\n",min_ctx.transport_fifo.crc_fails);
    send_buffer((uint8_t*)buffer,ret,ptr);
    return 1; 
}

/*****************************************************************************
* Sends the configuration to teslaterm
******************************************************************************/
uint8_t command_config_get(char *commandline, port_str *ptr){
    char buffer[80];
	for (uint8_t current_parameter = 0; current_parameter < sizeof(confparam) / sizeof(parameter_entry); current_parameter++) {
        if(confparam[current_parameter].parameter_type == PARAM_CONFIG  && confparam[current_parameter].visible){
            print_param_buffer(buffer, confparam, current_parameter);
            send_config(buffer,confparam[current_parameter].help, ptr);
        }
    }
    send_config("NULL","NULL", ptr);
    return 1; 
}

/*****************************************************************************
* Kicks the controller into the bootloader
******************************************************************************/
uint8_t command_bootloader(char *commandline, port_str *ptr) {
	Bootloadable_Load();
	return 1;
}

/*****************************************************************************
* starts or stops the transient mode (classic mode)
* also spawns a timer for the burst mode.
******************************************************************************/
uint8_t command_tr(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    CHECK_NULL(commandline);
    
	if (ntlibc_stricmp(commandline, "start") == 0) {
        interrupter.pw = param.pw;
		interrupter.prd = param.pwd;
        update_interrupter();

		tr_running = 1;
        
        callback_BurstFunction(NULL, 0, ptr);
        
		SEND_CONST_STRING("Transient Enabled\r\n", ptr);
       
        return 0;
	}
	if (ntlibc_stricmp(commandline, "stop") == 0) {
        interrupter.pw = 0;
		update_interrupter();
     
        SEND_CONST_STRING("Transient Disabled\r\n", ptr);    
 
		interrupter.pw = 0;
		update_interrupter();
		tr_running = 0;
		
		return 0;
	}
	HELP_TEXT("Usage: tr [start|stop]\r\n");
}

/*****************************************************************************
* Timer callback for the QCW autofire
******************************************************************************/
void vQCW_Timer_Callback(TimerHandle_t xTimer){
    if(!qcw_reg){
         	ramp.modulation_value = 20;
            qcw_reg = 1;
		    ramp_control(); 
    }
    if(param.qcw_repeat<100) param.qcw_repeat = 100;
    xTimerChangePeriod( xTimer, param.qcw_repeat / portTICK_PERIOD_MS, 0 );
}

/*****************************************************************************
* starts the QCW mode. Spawns a timer for the automatic QCW pulses.
******************************************************************************/
uint8_t command_qcw(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    CHECK_NULL(commandline);
        
	if (ntlibc_stricmp(commandline, "start") == 0) {
        if(param.qcw_repeat>99){
            if(xQCW_Timer==NULL){
                xQCW_Timer = xTimerCreate("QCW-Tmr", param.qcw_repeat / portTICK_PERIOD_MS, pdFALSE,(void * ) 0, vQCW_Timer_Callback);
                if(xQCW_Timer != NULL){
                    xTimerStart(xQCW_Timer, 0);
                    SEND_CONST_STRING("QCW Enabled\r\n", ptr);
                }else{
                    SEND_CONST_STRING("Cannot create QCW Timer\r\n", ptr);
                }
            }
        }else{
		    ramp.modulation_value = 20;
            qcw_reg = 1;
		    ramp_control();
            SEND_CONST_STRING("QCW single shot\r\n", ptr);
        }
		
		return 0;
	}
	if (ntlibc_stricmp(commandline, "stop") == 0) {
        if (xQCW_Timer != NULL) {
				if(xTimerDelete(xQCW_Timer, 200 / portTICK_PERIOD_MS) != pdFALSE){
				    xQCW_Timer = NULL;
                } else {
                    SEND_CONST_STRING("Cannot delete QCW Timer\r\n", ptr);
                }
		}
        QCW_enable_Control = 0;
        qcw_reg = 0;
		SEND_CONST_STRING("QCW Disabled\r\n", ptr);
		return 0;
	}
	HELP_TEXT("Usage: qcw [start|stop]\r\n");
}

/*****************************************************************************
* sets the kill bit, stops the interrupter and switches the bus off
******************************************************************************/
uint8_t command_udkill(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    CHECK_NULL(commandline);
    
    if (ntlibc_stricmp(commandline, "set") == 0) {
        interrupter_kill();
    	USBMIDI_1_callbackLocalMidiEvent(0, (uint8_t*)kill_msg);
    	bus_command = BUS_COMMAND_OFF;
        
        if (xQCW_Timer != NULL) {
    		if(xTimerDelete(xQCW_Timer, 200 / portTICK_PERIOD_MS) != pdFALSE){
    	        xQCW_Timer = NULL;
            }
    	}
        
    	interrupter1_control_Control = 0;
    	QCW_enable_Control = 0;
    	Term_Color_Green(ptr);
    	SEND_CONST_STRING("Killbit set\r\n", ptr);
        alarm_push(ALM_PRIO_CRITICAL,warn_kill_set);
    	Term_Color_White(ptr);
        return 1;
    }else if (ntlibc_stricmp(commandline, "reset") == 0) {
        interrupter_unkill();
        reset_fault();
        system_fault_Control = 0xFF;
        Term_Color_Green(ptr);
    	SEND_CONST_STRING("Killbit reset\r\n", ptr);
        alarm_push(ALM_PRIO_INFO,warn_kill_reset);
    	Term_Color_White(ptr);
        return 1;
    }else if (ntlibc_stricmp(commandline, "get") == 0) {
        char buf[30];
        Term_Color_Red(ptr);
        sprintf(buf, "Killbit: %u\r\n",interrupter_get_kill());
        send_string(buf,ptr);
        Term_Color_White(ptr);
        return 1;
    }
    
        
    HELP_TEXT("Usage: kill [set|reset|get]\r\n");
}

/*****************************************************************************
* commands a frequency sweep for the primary coil. It searches for a peak
* and makes a second run with +-6kHz around the peak
******************************************************************************/
uint8_t command_tune_p(char *commandline, port_str *ptr) {
    
    if(ptr->term_mode == PORT_TERM_TT){
        tsk_overlay_chart_stop();
        send_chart_clear(ptr);
        
    }
    
	run_adc_sweep(param.tune_start, param.tune_end, param.tune_pw, CT_PRIMARY, param.tune_delay, ptr);

	return 0;
}

/*****************************************************************************
* commands a frequency sweep for the secondary coil
******************************************************************************/
uint8_t command_tune_s(char *commandline, port_str *ptr) {
	run_adc_sweep(param.tune_start, param.tune_end, param.tune_pw, CT_SECONDARY, param.tune_delay, ptr);
	return 0;
}

/*****************************************************************************
* Prints the task list needs to be enabled in FreeRTOSConfig.h
* only use it for debugging reasons
******************************************************************************/
uint8_t command_tasks(char *commandline, port_str *ptr) {
    #if configUSE_STATS_FORMATTING_FUNCTIONS && configUSE_TRACE_FACILITY && configGENERATE_RUN_TIME_STATS
        char * buff = pvPortMalloc(sizeof(char)* 40 * uxTaskGetNumberOfTasks());
        if(buff == NULL){
            SEND_CONST_STRING("Malloc failed\r\n", ptr);
            return 0;
        }
        
	    SEND_CONST_STRING("**********************************************\n\r", ptr);
	    SEND_CONST_STRING("Task            State   Prio    Stack    Num\n\r", ptr);
	    SEND_CONST_STRING("**********************************************\n\r", ptr);
	    vTaskList(buff);
	    send_string(buff, ptr);
	    SEND_CONST_STRING("**********************************************\n\r\n\r", ptr);
        SEND_CONST_STRING("**********************************************\n\r", ptr);
	    SEND_CONST_STRING("Task            Abs time        % time\n\r", ptr);
	    SEND_CONST_STRING("**********************************************\n\r", ptr);
        vTaskGetRunTimeStats( buff );
        send_string(buff, ptr);
        SEND_CONST_STRING("**********************************************\n\r\r\n", ptr);
        sprintf(buff, "Free heap: %d\r\n",xPortGetFreeHeapSize());
        send_string(buff,ptr);
        vPortFree(buff);
        return 0;
    #endif
    
    #if !configUSE_STATS_FORMATTING_FUNCTIONS && !configUSE_TRACE_FACILITY
	    SEND_CONST_STRING("Taskinfo not active, activate it in FreeRTOSConfig.h\n\r", ptr);
        return 0;
	#endif
    
	
}

/*****************************************************************************
* Get a value from a parameter or print all parameters
******************************************************************************/
uint8_t command_get(char *commandline, port_str *ptr) {
	SKIP_SPACE(commandline);
    
	if (*commandline == 0 || commandline == 0) //no param --> show help text
	{
		print_param_help(confparam, PARAM_SIZE(confparam), ptr);
		return 1;
	}

	for (uint8_t current_parameter = 0; current_parameter < sizeof(confparam) / sizeof(parameter_entry); current_parameter++) {
		if (ntlibc_stricmp(commandline, confparam[current_parameter].name) == 0) {
			//Parameter found:
			print_param(confparam,current_parameter,ptr);
			return 1;
		}
	}
	Term_Color_Red(ptr);
	SEND_CONST_STRING("E: unknown param\r\n", ptr);
	Term_Color_White(ptr);
	return 0;
}

/*****************************************************************************
* Set a new value to a parameter
******************************************************************************/
uint8_t command_set(char *commandline, port_str *ptr) {
	SKIP_SPACE(commandline);
	char *param_value;

	if (commandline == NULL) {
		//if (!port)
		Term_Color_Red(ptr);
		SEND_CONST_STRING("E: no name\r\n", ptr);
		Term_Color_White(ptr);
		return 0;
	}

	param_value = strchr(commandline, ' ');
	if (param_value == NULL) {
		Term_Color_Red(ptr);
		SEND_CONST_STRING("E: no value\r\n", ptr);
		Term_Color_White(ptr);
		return 0;
	}

	*param_value = 0;
	param_value++;

	if (*param_value == '\0') {
		Term_Color_Red(ptr);
		SEND_CONST_STRING("E: no val\r\n", ptr);
		Term_Color_White(ptr);
		return 0;
	}

	for (uint8_t current_parameter = 0; current_parameter < sizeof(confparam) / sizeof(parameter_entry); current_parameter++) {
		if (ntlibc_stricmp(commandline, confparam[current_parameter].name) == 0) {
			//parameter name found:

			if (updateDefaultFunction(confparam, param_value,current_parameter, ptr)){
                if(confparam[current_parameter].callback_function){
                    if (confparam[current_parameter].callback_function(confparam, current_parameter, ptr)){
                        Term_Color_Green(ptr);
                        SEND_CONST_STRING("OK\r\n", ptr);
                        Term_Color_White(ptr);
                        return 1;
                    }else{
                        Term_Color_Red(ptr);
                        SEND_CONST_STRING("ERROR: Callback\r\n", ptr);
                        Term_Color_White(ptr);
                        return 1;
                    }
                }else{
                    Term_Color_Green(ptr);
                    SEND_CONST_STRING("OK\r\n", ptr);
                    Term_Color_White(ptr);
                    return 1;
                }
			} else {
				Term_Color_Red(ptr);
				SEND_CONST_STRING("NOK\r\n", ptr);
				Term_Color_White(ptr);
				return 1;
			}
		}
	}
	Term_Color_Red(ptr);
	SEND_CONST_STRING("E: unknown param\r\n", ptr);
	Term_Color_White(ptr);
	return 0;
}




/*****************************************************************************
* Saves confparams to eeprom
******************************************************************************/
uint8_t command_eprom(char *commandline, port_str *ptr) {
	SKIP_SPACE(commandline);
    CHECK_NULL(commandline);
    
    EEPROM_1_UpdateTemperature();
	uint8 sfflag = system_fault_Read();
	system_fault_Control = 0; //halt tesla coil operation during updates!
	if (ntlibc_stricmp(commandline, "save") == 0) {

	    EEPROM_write_conf(confparam, PARAM_SIZE(confparam),0, ptr);

		system_fault_Control = sfflag;
		return 0;
	}
	if (ntlibc_stricmp(commandline, "load") == 0) {
        uint8 sfflag = system_fault_Read();
        system_fault_Control = 0; //halt tesla coil operation during updates!
		EEPROM_read_conf(confparam, PARAM_SIZE(confparam) ,0,ptr);
        
        initialize_interrupter();
	    initialize_charging();
	    configure_ZCD_to_PWM();
	    system_fault_Control = sfflag;
		return 0;
	}
	HELP_TEXT("Usage: eprom [load|save]\r\n");
}

/*****************************************************************************
* Prints the help text
******************************************************************************/
uint8_t command_help(char *commandline, port_str *ptr) {
	UNUSED_VARIABLE(commandline);
	SEND_CONST_STRING("\r\nCommands:\r\n", ptr);
	for (uint8_t current_command = 0; current_command < (sizeof(commands) / sizeof(command_entry)); current_command++) {
		SEND_CONST_STRING("\t", ptr);
		Term_Color_Cyan(ptr);
		send_string((char *)commands[current_command].text, ptr);
		Term_Color_White(ptr);
		if (strlen(commands[current_command].text) > 7) {
			SEND_CONST_STRING("\t-> ", ptr);
		} else {
			SEND_CONST_STRING("\t\t-> ", ptr);
		}
		send_string((char *)commands[current_command].help, ptr);
		SEND_CONST_STRING("\r\n", ptr);
	}

	SEND_CONST_STRING("\r\nParameters:\r\n", ptr);
	for (uint8_t current_command = 0; current_command < sizeof(confparam) / sizeof(parameter_entry); current_command++) {
        if(confparam[current_command].parameter_type == PARAM_DEFAULT && confparam[current_command].visible){
            SEND_CONST_STRING("\t", ptr);
            Term_Color_Cyan(ptr);
            send_string((char *)confparam[current_command].name, ptr);
            Term_Color_White(ptr);
            if (strlen(confparam[current_command].name) > 7) {
                SEND_CONST_STRING("\t-> ", ptr);
            } else {
                SEND_CONST_STRING("\t\t-> ", ptr);
            }

            send_string((char *)confparam[current_command].help, ptr);
            SEND_CONST_STRING("\r\n", ptr);
        }
	}

	SEND_CONST_STRING("\r\nConfiguration:\r\n", ptr);
	for (uint8_t current_command = 0; current_command < sizeof(confparam) / sizeof(parameter_entry); current_command++) {
        if(confparam[current_command].parameter_type == PARAM_CONFIG && confparam[current_command].visible){
            SEND_CONST_STRING("\t", ptr);
            Term_Color_Cyan(ptr);
            send_string((char *)confparam[current_command].name, ptr);
            Term_Color_White(ptr);
            if (strlen(confparam[current_command].name) > 7) {
                SEND_CONST_STRING("\t-> ", ptr);
            } else {
                SEND_CONST_STRING("\t\t-> ", ptr);
            }

            send_string((char *)confparam[current_command].help, ptr);
            SEND_CONST_STRING("\r\n", ptr);
        }
	}

	return 0;
}

/*****************************************************************************
* Switches the bus on/off
******************************************************************************/
uint8_t command_bus(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    CHECK_NULL(commandline);

	if (ntlibc_stricmp(commandline, "on") == 0) {
		bus_command = BUS_COMMAND_ON;
		SEND_CONST_STRING("BUS ON\r\n", ptr);
        return 1;
	}
	if (ntlibc_stricmp(commandline, "off") == 0) {
		bus_command = BUS_COMMAND_OFF;
		SEND_CONST_STRING("BUS OFF\r\n", ptr);
        return 1;
	}
    
    HELP_TEXT("Usage: bus [on|off]\r\n");
}
/*****************************************************************************
* Resets the software fuse
******************************************************************************/
uint8_t command_fuse(char *commandline, port_str *ptr){
    i2t_reset();
    return 1;
}

/*****************************************************************************
* Loads the default parametes out of flash
******************************************************************************/
uint8_t command_load_default(char *commandline, port_str *ptr) {
    SEND_CONST_STRING("Default parameters loaded\r\n", ptr);
    init_config();
    return 1;
}

/*****************************************************************************
* Reset of the controller
******************************************************************************/
uint8_t command_reset(char *commandline, port_str *ptr){
    CySoftwareReset();
    return 1;
}


/*****************************************************************************
* Task handles for the overlay tasks
******************************************************************************/
xTaskHandle overlay_Serial_TaskHandle;
xTaskHandle overlay_USB_TaskHandle;
xTaskHandle overlay_ETH_TaskHandle[NUM_ETH_CON];

/*****************************************************************************
* Helper function for spawning the overlay task
******************************************************************************/
void start_overlay_task(port_str *ptr){
    switch(ptr->type){
        case PORT_TYPE_SERIAL:
            if (overlay_Serial_TaskHandle == NULL) {
        		xTaskCreate(tsk_overlay_TaskProc, "Overl_S", STACK_OVERLAY, ptr, PRIO_OVERLAY, &overlay_Serial_TaskHandle);
            }
        break;
        case PORT_TYPE_USB:
            if (overlay_USB_TaskHandle == NULL) {
    			xTaskCreate(tsk_overlay_TaskProc, "Overl_U", STACK_OVERLAY, ptr, PRIO_OVERLAY, &overlay_USB_TaskHandle);
    		}
        break;
        case PORT_TYPE_ETH:
            if (overlay_ETH_TaskHandle[ptr->num] == NULL) {
        		xTaskCreate(tsk_overlay_TaskProc, "Overl_E", STACK_OVERLAY, ptr, PRIO_OVERLAY, &overlay_ETH_TaskHandle[ptr->num]);
        	}
        break;
    }
}


/*****************************************************************************
* Helper function for killing the overlay task
******************************************************************************/
void stop_overlay_task(port_str *ptr){
    
    switch(ptr->type){
        case PORT_TYPE_SERIAL:
            if (overlay_Serial_TaskHandle != NULL) {
    				vTaskDelete(overlay_Serial_TaskHandle);
    				overlay_Serial_TaskHandle = NULL;
    		}
        break;
        case PORT_TYPE_USB:
            if (overlay_USB_TaskHandle != NULL) {
    				vTaskDelete(overlay_USB_TaskHandle);
    				overlay_USB_TaskHandle = NULL;
    		}
        break;
        case PORT_TYPE_ETH:
            if (overlay_ETH_TaskHandle[ptr->num] != NULL) {
			    vTaskDelete(overlay_ETH_TaskHandle[ptr->num]);
			    overlay_ETH_TaskHandle[ptr->num] = NULL;
		    }
        break;
    }
}

/*****************************************************************************
* 
******************************************************************************/
uint8_t command_status(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    CHECK_NULL(commandline);
	

	if (ntlibc_stricmp(commandline, "start") == 0) {
		start_overlay_task(ptr);
        return 1;
	}
	if (ntlibc_stricmp(commandline, "stop") == 0) {
		stop_overlay_task(ptr);
        return 1;
	}

	HELP_TEXT("Usage: status [start|stop]\r\n");
}

/*****************************************************************************
* Initializes the teslaterm telemetry
* Spawns the overlay task for telemetry stream generation
******************************************************************************/
uint8_t command_tterm(char *commandline, port_str *ptr){
    SKIP_SPACE(commandline);
    CHECK_NULL(commandline);
    
	if (ntlibc_stricmp(commandline, "start") == 0) {
        ptr->term_mode = PORT_TERM_TT;
        send_gauge_config(0, GAUGE0_MIN, GAUGE0_MAX, GAUGE0_NAME, ptr);
        send_gauge_config(1, GAUGE1_MIN, GAUGE1_MAX, GAUGE1_NAME, ptr);
        send_gauge_config(2, GAUGE2_MIN, GAUGE2_MAX, GAUGE2_NAME, ptr);
        send_gauge_config(3, GAUGE3_MIN, GAUGE3_MAX, GAUGE3_NAME, ptr);
        send_gauge_config(4, GAUGE4_MIN, GAUGE4_MAX, GAUGE4_NAME, ptr);
        send_gauge_config(5, GAUGE5_MIN, GAUGE5_MAX, GAUGE5_NAME, ptr);
        send_gauge_config(6, GAUGE6_MIN, GAUGE6_MAX, GAUGE6_NAME, ptr);
        
        send_chart_config(0, CHART0_MIN, CHART0_MAX, CHART0_OFFSET, CHART0_UNIT, CHART0_NAME, ptr);
        send_chart_config(1, CHART1_MIN, CHART1_MAX, CHART1_OFFSET, CHART1_UNIT, CHART1_NAME, ptr);
        send_chart_config(2, CHART2_MIN, CHART2_MAX, CHART2_OFFSET, CHART2_UNIT, CHART2_NAME, ptr);
        send_chart_config(3, CHART3_MIN, CHART3_MAX, CHART3_OFFSET, CHART3_UNIT, CHART3_NAME, ptr);

        start_overlay_task(ptr);
        return 1;

    }
	if (ntlibc_stricmp(commandline, "stop") == 0) {
        ptr->term_mode = PORT_TERM_VT100;
        stop_overlay_task(ptr);
        return 1;
	} 
    
    HELP_TEXT("Usage: tterm [start|stop]\r\n");
}

/*****************************************************************************
* Clears the terminal screen and displays the logo
******************************************************************************/
uint8_t command_cls(char *commandline, port_str *ptr) {
    tsk_overlay_chart_start();
	Term_Erase_Screen(ptr);
    SEND_CONST_STRING("  _   _   ___    ____    _____              _\r\n",ptr);
    SEND_CONST_STRING(" | | | | |   \\  |__ /   |_   _|  ___   ___ | |  __ _\r\n",ptr);
    SEND_CONST_STRING(" | |_| | | |) |  |_ \\     | |   / -_) (_-< | | / _` |\r\n",ptr);
    SEND_CONST_STRING("  \\___/  |___/  |___/     |_|   \\___| /__/ |_| \\__,_|\r\n\r\n",ptr);
    SEND_CONST_STRING("\tCoil: ",ptr);
    send_string(configuration.ud_name,ptr);
    SEND_CONST_STRING("\r\n\r\n",ptr);
	return 1;
}

/*****************************************************************************
* Signal debugging
******************************************************************************/

void send_signal_state(uint8_t signal, uint8_t inverted, port_str *ptr){
    if(inverted) signal = !signal; 
    if(signal){
        Term_Color_Red(ptr);
        SEND_CONST_STRING("true\r\n",ptr);
        Term_Color_White(ptr)  
    }else{
        Term_Color_Green(ptr);
        SEND_CONST_STRING("false\r\n",ptr);
        Term_Color_White(ptr);
    }
}

uint8_t command_signals(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    char buffer[65];
    SEND_CONST_STRING("\r\nSignal state:\r\n", ptr);
    SEND_CONST_STRING("*************\r\n", ptr);
    SEND_CONST_STRING("UVLO pin: ", ptr);
    send_signal_state(UVLO_status_Status,pdTRUE,ptr);
    
    SEND_CONST_STRING("Sysfault bit driver undervoltage: ", ptr);
    send_signal_state(telemetry.sys_fault[SYS_FAULT_UVLO],pdFALSE,ptr);
    SEND_CONST_STRING("Sysfault bit temperature: ", ptr);
    send_signal_state(telemetry.sys_fault[SYS_FAULT_TEMP],pdFALSE,ptr);
    SEND_CONST_STRING("Sysfault bit fuse: ", ptr);
    send_signal_state(telemetry.sys_fault[SYS_FAULT_FUSE],pdFALSE,ptr);
    SEND_CONST_STRING("Sysfault bit charging: ", ptr);
    send_signal_state(telemetry.sys_fault[SYS_FAULT_CHARGE],pdFALSE,ptr);
    SEND_CONST_STRING("Sysfault bit watchdog: ", ptr);
    send_signal_state(telemetry.sys_fault[SYS_FAULT_WD],pdFALSE,ptr);
    SEND_CONST_STRING("Sysfault bit updating: ", ptr);
    send_signal_state(telemetry.sys_fault[SYS_FAULT_UPDATE],pdFALSE,ptr);
    SEND_CONST_STRING("Sysfault bit bus undervoltage: ", ptr);
    send_signal_state(telemetry.sys_fault[SYS_FAULT_BUS_UV],pdFALSE,ptr);
    SEND_CONST_STRING("_________________________________\r\n", ptr);
    SEND_CONST_STRING("Sysfault combined: ", ptr);
    send_signal_state(system_fault_Read(),pdTRUE,ptr);
    
    SEND_CONST_STRING("Relay 1: ", ptr);
    send_signal_state((relay_Read()&0b1),pdFALSE,ptr);
    SEND_CONST_STRING("Relay 2: ", ptr);
    send_signal_state((relay_Read()&0b10),pdFALSE,ptr);
    SEND_CONST_STRING("Fan: ", ptr);
    send_signal_state(an_ctrl_Read(),pdFALSE,ptr);
    
    SEND_CONST_STRING("Bus status: ", ptr);
    Term_Color_Cyan(ptr);
    switch(telemetry.bus_status){
        case BUS_BATT_OV_FLT:
            SEND_CONST_STRING("Overvoltage", ptr);
        break;
        case BUS_BATT_UV_FLT:
            SEND_CONST_STRING("Undervoltage", ptr);
        break;
        case BUS_CHARGING:
            SEND_CONST_STRING("Charging", ptr);
        break;
        case BUS_OFF:
            SEND_CONST_STRING("Off", ptr);
        break;
        case BUS_READY:
            SEND_CONST_STRING("Ready", ptr);
        break;
        case BUS_TEMP1_FAULT:
            SEND_CONST_STRING("Temperature fault", ptr);
        break;
    }
    SEND_CONST_STRING("\r\n", ptr);
    Term_Color_White(ptr); 
    snprintf(buffer,sizeof(buffer),"Temp 1: %i*C\r\nTemp 1: %i*C\r\n", telemetry.temp1, telemetry.temp2);
    send_string(buffer, ptr);
    snprintf(buffer,sizeof(buffer),"Vbus: %u mV\r\n", ADC_CountsTo_mVolts(ADC_sample_buf[DATA_VBUS]));
    send_string(buffer, ptr);
    snprintf(buffer,sizeof(buffer),"Vbatt: %u mV\r\n", ADC_CountsTo_mVolts(ADC_sample_buf[DATA_VBATT]));
    send_string(buffer, ptr);
    snprintf(buffer,sizeof(buffer),"Ibus: %u mV\r\n\r\n", ADC_CountsTo_mVolts(ADC_sample_buf[DATA_IBUS]));
    send_string(buffer, ptr);
    
    
	return 1;
}

/*****************************************************************************
* Prints the ethernet connections
******************************************************************************/
uint8_t command_ethcon(char *commandline, port_str *ptr) {
    SKIP_SPACE(commandline);
    #define COL_A 9
    #define COL_B 15
    char buffer[40];
    uint8_t ret;
    SEND_CONST_STRING("Ethernet\r\nIP: ",ptr);
    send_string(pEth_info[ETH_INFO_ETH]->ip,ptr);
    SEND_CONST_STRING("\r\nGateway: ",ptr);
    send_string(pEth_info[ETH_INFO_ETH]->gw,ptr);
    SEND_CONST_STRING("\r\nDevice info: ",ptr);
    send_string(pEth_info[ETH_INFO_ETH]->info,ptr);
    SEND_CONST_STRING("\r\n\nWIFI\r\nIP: ",ptr);
    send_string(pEth_info[ETH_INFO_WIFI]->ip,ptr);
    SEND_CONST_STRING("\r\nGateway: ",ptr);
    send_string(pEth_info[ETH_INFO_WIFI]->gw,ptr);
    SEND_CONST_STRING("\r\nDevice info: ",ptr);
    send_string(pEth_info[ETH_INFO_WIFI]->info,ptr);
    
    
    SEND_CONST_STRING("\r\n\nConnected clients:\r\n",ptr);
    Term_Move_cursor_right(COL_A,ptr);
    SEND_CONST_STRING("Num", ptr);
    Term_Move_cursor_right(COL_B,ptr);
    SEND_CONST_STRING("| Remote IP\r\n", ptr);
    
    for(uint8_t i=0;i<NUM_ETH_CON;i++){
        if(socket_info[i].socket==SOCKET_CONNECTED){
            Term_Move_cursor_right(COL_A,ptr);
            ret = snprintf(buffer,sizeof(buffer), "\033[36m%d", i);
            send_buffer((uint8_t*)buffer,ret,ptr);
            Term_Move_cursor_right(COL_B,ptr);
            ret = snprintf(buffer,sizeof(buffer), "\033[32m%s\r\n", socket_info[i].info);
            send_buffer((uint8_t*)buffer,ret,ptr);
        }
    }
    Term_Color_White(ptr);
	return 1;
}