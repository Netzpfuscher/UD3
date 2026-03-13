/*
 * TTerm
 *
 * Copyright (c) 2020 Thorben Zethoff, Jens Kerrinnes
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

#include "wizard.h"
#include "cli_common.h"
#include "cli_basic.h"
#include "autotune.h"
#include "string.h"
#include "stdlib.h"

#define APP_NAME "wizard"
#define APP_DESCRIPTION "configuration wizard"

#define INPUT_BUF_SIZE 32
#define WIZ_COLOR_TITLE   "\033[1;36m"
#define WIZ_COLOR_PARAM   "\033[33m"
#define WIZ_COLOR_VALUE   "\033[1;32m"
#define WIZ_COLOR_RANGE   "\033[90m"
#define WIZ_COLOR_HELP    "\033[37m"
#define WIZ_COLOR_PROMPT  "\033[1;37m"
#define WIZ_COLOR_OK      "\033[1;32m"
#define WIZ_COLOR_ERR     "\033[1;31m"
#define WIZ_COLOR_RESET   "\033[0m"
#define WIZ_COLOR_STEP    "\033[1;35m"
#define WIZ_COLOR_HEADER  "\033[1;33m"

typedef struct {
	const char *title;
	const char **param_names;
	uint8_t param_count;
} wizard_section_t;

static const char *sec_identity[] = {"ud_name", "qcw_coil"};
static const char *sec_resonant[] = {"start_freq", "start_cycles", "lead_time"};
static const char *sec_safety[] = {"max_tr_pw", "max_tr_prf", "max_tr_duty", "max_qcw_pw", "max_qcw_duty"};
static const char *sec_current[] = {"max_tr_current", "min_tr_current", "max_qcw_current", "ct1_ratio", "ct1_burden", "ct2_ratio", "ct2_burden"};
static const char *sec_power[] = {"ps_scheme", "r_bus", "charge_delay", "min_fb_current"};
static const char *sec_temp[] = {"temp1_max", "temp1_setpoint", "temp2_max"};
static const char *sec_hardware[] = {"hw_rev", "vdrive", "baudrate"};

static const wizard_section_t sections[] = {
	{"Identity",             sec_identity, sizeof(sec_identity) / sizeof(char*)},
	{"Resonant Frequency",   sec_resonant, sizeof(sec_resonant) / sizeof(char*)},
	{"Safety Limits",        sec_safety,   sizeof(sec_safety)   / sizeof(char*)},
	{"Current Transformers", sec_current,  sizeof(sec_current)  / sizeof(char*)},
	{"Power Supply",         sec_power,    sizeof(sec_power)    / sizeof(char*)},
	{"Temperature",          sec_temp,     sizeof(sec_temp)     / sizeof(char*)},
	{"Hardware",             sec_hardware, sizeof(sec_hardware) / sizeof(char*)},
};

#define NUM_SECTIONS (sizeof(sections) / sizeof(wizard_section_t))

static int8_t find_param_index(const char *name) {
	for (uint8_t i = 0; i < get_conf_size(); i++) {
		if (strcmp(confparam[i].name, name) == 0) return i;
	}
	return -1;
}

static void wizard_print_value(TERMINAL_HANDLE *handle, parameter_entry *p) {
	uint32_t u = 0;
	int32_t s = 0;
	switch (p->type) {
	case TYPE_UNSIGNED:
		switch (p->size) {
		case 1: u = *(uint8_t *)p->value; break;
		case 2: u = *(uint16_t *)p->value; break;
		case 4: u = *(uint32_t *)p->value; break;
		}
		if (p->div) {
			ttprintf("%u.%0*u", u / p->div, n_number(p->div) - 1, u % p->div);
		} else {
			ttprintf("%u", u);
		}
		break;
	case TYPE_SIGNED:
		switch (p->size) {
		case 1: s = *(int8_t *)p->value; break;
		case 2: s = *(int16_t *)p->value; break;
		case 4: s = *(int32_t *)p->value; break;
		}
		if (p->div) {
			uint32_t mod = (s < 0) ? ((-s) % p->div) : (s % p->div);
			ttprintf("%i.%0*u", s / p->div, n_number(p->div) - 1, mod);
		} else {
			ttprintf("%i", s);
		}
		break;
	case TYPE_FLOAT:
		ttprintf("%f", *(float *)p->value);
		break;
	case TYPE_STRING:
		ttprintf("%s", (char *)p->value);
		break;
	case TYPE_CHAR:
		ttprintf("%c", *(char *)p->value);
		break;
	default:
		ttprintf("?");
		break;
	}
}

static void wizard_print_range(TERMINAL_HANDLE *handle, parameter_entry *p) {
	if (p->type == TYPE_STRING || p->type == TYPE_CHAR) return;
	if (p->div) {
		ttprintf("%i.%0*u .. %i.%0*u",
			p->min / p->div, n_number(p->div) - 1, abs(p->min) % p->div,
			p->max / p->div, n_number(p->div) - 1, p->max % p->div);
	} else {
		ttprintf("%i .. %i", p->min, p->max);
	}
}

static void wizard_draw_header(TERMINAL_HANDLE *handle, uint8_t section_idx) {
	TERM_sendVT100Code(handle, _VT100_CLS, 0);
	TERM_sendVT100Code(handle, _VT100_CURSOR_POS1, 0);

	ttprintf(WIZ_COLOR_TITLE);
	ttprintf("  UD3 Configuration Wizard\r\n");
	ttprintf(WIZ_COLOR_RESET);
	ttprintf("  ====================================================\r\n");

	ttprintf(WIZ_COLOR_STEP "  Step %d/%d: " WIZ_COLOR_HEADER "%s\r\n" WIZ_COLOR_RESET,
		section_idx + 1, NUM_SECTIONS, sections[section_idx].title);
	ttprintf("  ----------------------------------------------------\r\n\r\n");
}

static void wizard_draw_params(TERMINAL_HANDLE *handle, uint8_t section_idx, int8_t highlight) {
	const wizard_section_t *sec = &sections[section_idx];
	for (uint8_t i = 0; i < sec->param_count; i++) {
		int8_t pidx = find_param_index(sec->param_names[i]);
		if (pidx < 0) continue;
		parameter_entry *p = &confparam[pidx];

		if (i == highlight) {
			ttprintf("  " WIZ_COLOR_PROMPT ">> ");
		} else {
			ttprintf("     ");
		}

		ttprintf(WIZ_COLOR_PARAM "%-18s" WIZ_COLOR_RESET, p->name);
		ttprintf("= " WIZ_COLOR_VALUE);
		wizard_print_value(handle, p);
		ttprintf(WIZ_COLOR_RESET);

		ttprintf("  " WIZ_COLOR_RANGE "[");
		wizard_print_range(handle, p);
		ttprintf("]" WIZ_COLOR_RESET);

		ttprintf("  " WIZ_COLOR_HELP "%s" WIZ_COLOR_RESET, p->help);
		ttprintf("\r\n");
	}
}

static uint8_t wizard_readline(TERMINAL_HANDLE *handle, char *buf, uint8_t maxlen) {
	uint8_t pos = 0;
	memset(buf, 0, maxlen);
	for (;;) {
		uint8_t c = getch(handle, portMAX_DELAY);
		if (c == CTRL_C) {
			return 0;
		} else if (c == '\r' || c == '\n') {
			ttprintf("\r\n");
			return 1;
		} else if (c == 0x7f || c == '\b') {
			if (pos > 0) {
				pos--;
				buf[pos] = 0;
				ttprintf("\b \b");
			}
		} else if (c == 0x1b) {
			uint8_t c2 = getch(handle, pdMS_TO_TICKS(50));
			if (c2 == '[') {
				getch(handle, pdMS_TO_TICKS(50));
			}
		} else if (c >= ' ' && c <= '~' && pos < maxlen - 1) {
			buf[pos++] = c;
			buf[pos] = 0;
			ttprintf("%c", c);
		}
	}
}

static uint8_t wizard_edit_param(TERMINAL_HANDLE *handle, uint8_t section_idx, uint8_t param_idx) {
	const wizard_section_t *sec = &sections[section_idx];
	int8_t pidx = find_param_index(sec->param_names[param_idx]);
	if (pidx < 0) return 1;
	parameter_entry *p = &confparam[pidx];

	ttprintf("\r\n  " WIZ_COLOR_PARAM "%s" WIZ_COLOR_RESET " = ", p->name);
	ttprintf(WIZ_COLOR_VALUE);
	wizard_print_value(handle, p);
	ttprintf(WIZ_COLOR_RESET);
	ttprintf("\r\n  Enter new value (or press Enter to keep): ");
	ttprintf(WIZ_COLOR_VALUE);

	char buf[INPUT_BUF_SIZE];
	if (!wizard_readline(handle, buf, INPUT_BUF_SIZE)) {
		return 0;
	}
	ttprintf(WIZ_COLOR_RESET);

	if (buf[0] == 0) {
		ttprintf("  " WIZ_COLOR_OK "(kept)" WIZ_COLOR_RESET "\r\n");
		return 1;
	}

	if (updateDefaultFunction(confparam, buf, pidx, handle)) {
		if (p->callback_function) {
			if (p->callback_function(confparam, pidx, handle)) {
				ttprintf("  " WIZ_COLOR_OK "OK" WIZ_COLOR_RESET "\r\n");
			} else {
				ttprintf("  " WIZ_COLOR_ERR "Callback error" WIZ_COLOR_RESET "\r\n");
			}
		} else {
			ttprintf("  " WIZ_COLOR_OK "OK" WIZ_COLOR_RESET "\r\n");
		}
	} else {
		ttprintf("  " WIZ_COLOR_ERR "Value rejected" WIZ_COLOR_RESET "\r\n");
	}
	return 1;
}

uint8_t REGISTER_wizard(TermCommandDescriptor *desc) {
	TERM_addCommand(CMD_wizard, APP_NAME, APP_DESCRIPTION, 0, desc);
	return pdTRUE;
}

uint8_t CMD_wizard(TERMINAL_HANDLE *handle, uint8_t argCount, char **args) {
	uint8_t section = 0;

	TERM_sendVT100Code(handle, _VT100_CURSOR_DISABLE, 0);

	while (section < NUM_SECTIONS) {
		wizard_draw_header(handle, section);
		wizard_draw_params(handle, section, -1);

		ttprintf("\r\n  " WIZ_COLOR_HELP "[e]dit  [n]ext  [p]rev  [s]kip all  [q]uit" WIZ_COLOR_RESET "\r\n");
		ttprintf("  > ");

		uint8_t c = getch(handle, portMAX_DELAY);
		if (c == 'q' || c == CTRL_C) {
			goto done;
		} else if (c == 'n' || c == '\r') {
			section++;
		} else if (c == 'p') {
			if (section > 0) section--;
		} else if (c == 's') {
			break;
		} else if (c == 'e') {
			const wizard_section_t *sec = &sections[section];
			for (uint8_t i = 0; i < sec->param_count; i++) {
				wizard_draw_header(handle, section);
				wizard_draw_params(handle, section, i);
				if (!wizard_edit_param(handle, section, i)) {
					goto done;
				}
				vTaskDelay(pdMS_TO_TICKS(300));
			}
		}
	}

	TERM_sendVT100Code(handle, _VT100_CLS, 0);
	TERM_sendVT100Code(handle, _VT100_CURSOR_POS1, 0);
	ttprintf(WIZ_COLOR_TITLE "\r\n  Wizard Complete\r\n" WIZ_COLOR_RESET);
	ttprintf("  ================================================\r\n\r\n");
	ttprintf("  " WIZ_COLOR_HELP "Save configuration to EEPROM? [y/n]" WIZ_COLOR_RESET " ");

	uint8_t c = getch(handle, portMAX_DELAY);
	if (c == 'y' || c == 'Y') {
		ttprintf("\r\n\r\n");
		EEPROM_check_hash(confparam, get_conf_size(), handle);
		EEPROM_write_conf(confparam, get_conf_size(), 0, handle);
		ttprintf("  " WIZ_COLOR_OK "Configuration saved." WIZ_COLOR_RESET "\r\n");
	} else {
		ttprintf("\r\n\r\n  " WIZ_COLOR_PARAM "Not saved. Use 'eeprom save' to save later." WIZ_COLOR_RESET "\r\n");
	}

	ttprintf("\r\n  " WIZ_COLOR_TITLE "Primary Autotune" WIZ_COLOR_RESET "\r\n");
	ttprintf("  ---------------------------------------------------\r\n");
	ttprintf("\r\n" WIZ_COLOR_ERR "  WARNING: The Tesla coil will be energized!" WIZ_COLOR_RESET "\r\n");
	ttprintf("  " WIZ_COLOR_HELP "The bridge will hard-switch to sweep frequencies" WIZ_COLOR_RESET "\r\n");
	ttprintf("  " WIZ_COLOR_HELP "around start_freq (" WIZ_COLOR_VALUE);
	ttprintf("%u.%ukHz", configuration.start_freq / 10, configuration.start_freq % 10);
	ttprintf(WIZ_COLOR_HELP " +/- 20kHz)." WIZ_COLOR_RESET "\r\n");
	ttprintf("  " WIZ_COLOR_HELP "Make sure bus voltage is appropriate." WIZ_COLOR_RESET "\r\n");
	ttprintf("\r\n  " WIZ_COLOR_HELP "Run autotune now? [y/n]" WIZ_COLOR_RESET " ");

	c = getch(handle, portMAX_DELAY);
	if (c == 'y' || c == 'Y') {
		ttprintf("\r\n\r\n");

		uint16_t center = configuration.start_freq;
		uint16_t f_min = (center > 200) ? (center - 200) : 1;
		uint16_t f_max = center + 200;
		if (f_max > 5000) f_max = 5000;

		ttprintf("  Sweep range: " WIZ_COLOR_VALUE "%u.%ukHz" WIZ_COLOR_RESET " .. " WIZ_COLOR_VALUE "%u.%ukHz" WIZ_COLOR_RESET "\r\n\r\n",
			f_min / 10, f_min % 10, f_max / 10, f_max % 10);

		uint16_t peak = run_adc_sweep(f_min, f_max, param.tune_pw, param.tune_delay, handle);

		if (peak > 0) {
			ttprintf("\r\n  " WIZ_COLOR_OK "Peak found at: %u.%ukHz" WIZ_COLOR_RESET "\r\n", peak / 10, peak % 10);
			ttprintf("  " WIZ_COLOR_HELP "Set start_freq to %u.%ukHz? [y/n]" WIZ_COLOR_RESET " ", peak / 10, peak % 10);
			c = getch(handle, portMAX_DELAY);
			if (c == 'y' || c == 'Y') {
				configuration.start_freq = peak;
				ttprintf("\r\n  " WIZ_COLOR_OK "start_freq set to %u.%ukHz" WIZ_COLOR_RESET "\r\n", peak / 10, peak % 10);
				ttprintf("  " WIZ_COLOR_PARAM "Use 'eeprom save' to persist this change." WIZ_COLOR_RESET "\r\n");
			} else {
				ttprintf("\r\n  " WIZ_COLOR_PARAM "start_freq not changed." WIZ_COLOR_RESET "\r\n");
			}
		}
	} else {
		ttprintf("\r\n  " WIZ_COLOR_PARAM "Autotune skipped." WIZ_COLOR_RESET "\r\n");
	}

done:
	TERM_sendVT100Code(handle, _VT100_CURSOR_ENABLE, 0);
	ttprintf(WIZ_COLOR_RESET "\r\n");
	return TERM_CMD_EXIT_SUCCESS;
}
