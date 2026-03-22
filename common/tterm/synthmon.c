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

#include "synthmon.h"
#include "string.h"
#include "system.h"
#include "SignalGenerator.h"
#include "cli_common.h"
#include "VMSWrapper.h"
#include "SidProcessor.h"
#include "SidFilter.h"
#include "MidiProcessor.h"
#include "NoteMapper.h"
#include "DutyCompressor.h"
#include "tasks/tsk_sid.h"

#define APP_NAME        "synthmon"
#define APP_DESCRIPTION "Synthesizer status monitor"
#define APP_STACK       350

#define BAR_WIDTH  14
#define COL_W      66
#define EL         TERM_getVT100Code(_VT100_ERASE_LINE_END, 0)

uint8_t CMD_synthmon_main(TERMINAL_HANDLE *handle, uint8_t argCount, char **args);
void TASK_synthmon(void *pvParameters);
uint8_t INPUT_synthmon(TERMINAL_HANDLE *handle, uint16_t c);

uint8_t REGISTER_synthmon(TermCommandDescriptor *desc) {
	TERM_addCommand(CMD_synthmon_main, APP_NAME, APP_DESCRIPTION, 0, desc);
	return pdTRUE;
}

uint8_t CMD_synthmon_main(TERMINAL_HANDLE *handle, uint8_t argCount, char **args) {
	uint8_t currArg = 0;
	uint8_t returnCode = TERM_CMD_EXIT_SUCCESS;
	char **cpy_args = NULL;
	argCount++;
	if (argCount) {
		cpy_args = pvPortMalloc(sizeof(char *) * argCount);
		cpy_args[0] = pvPortMalloc(sizeof(APP_NAME));
		cpy_args[0] = memcpy(cpy_args[0], APP_NAME, sizeof(APP_NAME));
		for (; currArg < argCount - 1; currArg++) {
			uint16_t len = strlen(args[currArg]) + 1;
			cpy_args[currArg + 1] = pvPortMalloc(len);
			memcpy(cpy_args[currArg + 1], args[currArg], len);
		}
	}
	TermProgram *prog = pvPortMalloc(sizeof(TermProgram));
	prog->inputHandler = INPUT_synthmon;
	prog->args = cpy_args;
	prog->argCount = argCount;
	prog->userData = NULL;
	TERM_sendVT100Code(handle, _VT100_RESET, 0);
	TERM_sendVT100Code(handle, _VT100_CURSOR_POS1, 0);
	returnCode = xTaskCreate(TASK_synthmon, APP_NAME, APP_STACK, handle, tskIDLE_PRIORITY + 1, &prog->task) ? TERM_CMD_EXIT_PROC_STARTED : TERM_CMD_EXIT_ERROR;
	if (returnCode == TERM_CMD_EXIT_PROC_STARTED) TERM_attachProgramm(handle, prog);
	return returnCode;
}

/* ── helpers ──────────────────────────────────────────────────────────────── */

static void print_bar(TERMINAL_HANDLE *handle, uint32_t value, uint32_t max, uint8_t color) {
	uint8_t filled = (max > 0) ? (uint8_t)((value * BAR_WIDTH) / max) : 0;
	if (filled > BAR_WIDTH) filled = BAR_WIDTH;
	char buf[BAR_WIDTH + 3];
	buf[0] = '[';
	for (uint8_t i = 0; i < BAR_WIDTH; i++) buf[1 + i] = (i < filled) ? '#' : '.';
	buf[BAR_WIDTH + 1] = ']';
	buf[BAR_WIDTH + 2] = '\0';
	ttprintf("%s%s%s%s",
		TERM_getVT100Code(_VT100_FOREGROUND_COLOR, color),
		buf,
		TERM_getVT100Code(_VT100_RESET_ATTRIB, 0),
		TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_WHITE));
}

static void print_hline(TERMINAL_HANDLE *handle) {
	char line[COL_W + 1];
	line[0] = '+';
	for (uint8_t i = 1; i < COL_W - 1; i++) line[i] = '-';
	line[COL_W - 1] = '+';
	line[COL_W] = '\0';
	ttprintf("%s%s%s%s\r\n",
		TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_CYAN),
		line,
		TERM_getVT100Code(_VT100_RESET_ATTRIB, 0),
		EL);
}

static void print_section_header(TERMINAL_HANDLE *handle, const char *title) {
	uint8_t tlen = strlen(title);
	char trail[COL_W + 1];
	uint8_t trail_len = COL_W - 6 - tlen;
	for (uint8_t i = 0; i < trail_len; i++) trail[i] = '-';
	trail[trail_len] = '\0';
	ttprintf("%s+-- %s%s%s %s%s+%s%s\r\n",
		TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_CYAN),
		TERM_getVT100Code(_VT100_BRIGHT, 0),
		TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_WHITE),
		title,
		TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_CYAN),
		trail,
		TERM_getVT100Code(_VT100_RESET_ATTRIB, 0),
		EL);
}

/* ── MIDI display ─────────────────────────────────────────────────────────── */

static const char *note_name(uint8_t note) {
	static const char *names[] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
	return names[note % 12];
}

static void print_midi(TERMINAL_HANDLE *handle) {
	print_section_header(handle, "MIDI Voices");

	ttprintf("%s| %sV# Note  Ch Freq bar         PW bar           Vol bar%s%s\r\n",
		TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_CYAN),
		TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_YELLOW),
		TERM_getVT100Code(_VT100_RESET_ATTRIB, 0),
		EL);
	print_hline(handle);

	for (uint8_t i = 0; i < SIGGEN_VOICECOUNT; i++) {
		VMS_VoiceData_t *v = &(VMSW_getVoiceData()[i]);
		uint8_t active = v->on;
		uint8_t row_color = active ? _VT100_GREEN : _VT100_WHITE;

		ttprintf("%s| %s%d %s%s%s  %2d ",
			TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_CYAN),
			TERM_getVT100Code(_VT100_FOREGROUND_COLOR, row_color),
			i,
			note_name(v->sourceNote),
			active ? "*" : " ",
			TERM_getVT100Code(_VT100_RESET_ATTRIB, 0),
			v->sourceChannel);
		print_bar(handle, v->freqCurrent, 200000, active ? _VT100_CYAN : _VT100_BLUE);
		ttprintf(" ");
		print_bar(handle, v->onTimeCurrent, 500, active ? _VT100_MAGENTA : _VT100_BLUE);
		ttprintf(" ");
		print_bar(handle, v->volumeCurrent, 0xffff, active ? _VT100_GREEN : _VT100_BLUE);
		if (v->hypervoiceCount > 0) {
			ttprintf(" H%u%s\r\n", v->hypervoiceCount, EL);
		} else {
			ttprintf("%s\r\n", EL);
		}
	}

	print_section_header(handle, "Channel Maps");
	for (uint8_t i = 0; i < MIDI_CHANNELCOUNT; i += 2) {
		const char *n0 = Mapper_getProgrammName(i);
		const char *n1 = (i + 1 < MIDI_CHANNELCOUNT) ? Mapper_getProgrammName(i + 1) : NULL;
			ttprintf("%s|%s %s[%2d]%s %-22s  %s[%2d]%s %-22s%s|%s%s\r\n",
			TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_CYAN),
			TERM_getVT100Code(_VT100_RESET_ATTRIB, 0),
			TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_YELLOW), i,
			TERM_getVT100Code(_VT100_RESET_ATTRIB, 0),
			n0 ? n0 : "-",
			TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_YELLOW), i + 1,
			TERM_getVT100Code(_VT100_RESET_ATTRIB, 0),
			n1 ? n1 : "-",
			TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_CYAN),
			TERM_getVT100Code(_VT100_RESET_ATTRIB, 0),
			EL);
	}
	print_hline(handle);
}

/* ── SID display ──────────────────────────────────────────────────────────── */

static const char *adsr_state_name(uint8_t s) {
	switch (s) {
		case 0: return "ATK";
		case 1: return "DCY";
		case 2: return "SUS";
		case 3: return "REL";
		default: return "---";
	}
}

static void print_sid(TERMINAL_HANDLE *handle) {
	print_section_header(handle, "SID Channels");

	ttprintf("%s| %s Ch  Wave     ADSR  Freq(dHz)            Vol%s%s\r\n",
		TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_CYAN),
		TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_YELLOW),
		TERM_getVT100Code(_VT100_RESET_ATTRIB, 0),
		EL);
	print_hline(handle);

	for (uint8_t i = 0; i < N_SIDCHANNEL; i++) {
		SIDChannelData_t *ch = &(SID_getChannelData()[i]);
		uint8_t active = (ch->currentEnvelopeVolume > 0);
		uint8_t row_color = active ? _VT100_GREEN : _VT100_WHITE;

		ttprintf("%s|%s  %s%d   %-8s [%s] ",
			TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_CYAN),
			TERM_getVT100Code(_VT100_FOREGROUND_COLOR, row_color),
			active ? "*" : " ",
			i,
			SID_getWaveName(ch),
			adsr_state_name(ch->adsrState));
		ttprintf("%s", TERM_getVT100Code(_VT100_RESET_ATTRIB, 0));
		print_bar(handle, ch->frequency_dHz, 200000, active ? _VT100_CYAN : _VT100_BLUE);
		ttprintf(" %6u dHz%s\r\n", ch->frequency_dHz, EL);

		ttprintf("%s|%s                     Vol: ",
			TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_CYAN),
			TERM_getVT100Code(_VT100_RESET_ATTRIB, 0));
		print_bar(handle, ch->currentEnvelopeVolume, 0xffff, active ? _VT100_GREEN : _VT100_BLUE);
		ttprintf(" %04x  mVol:%d%s\r\n", ch->currentEnvelopeVolume, SID_filterData.channelVolume[i], EL);

		print_hline(handle);
	}
}

/* ── compressor display ───────────────────────────────────────────────────── */

static void print_compressor(TERMINAL_HANDLE *handle) {
	print_section_header(handle, "Duty Compressor");

	uint32_t gain = Comp_getGain();
	uint32_t state = Comp_getState();
	uint32_t maxDuty = Comp_getMaxDutyOffset();

	static const char *state_names[] = {"ATTACK ", "SUSTAIN", "RELEASE"};
	const char *sname = (state < 3) ? state_names[state] : "???    ";

	uint8_t gain_color = (gain > (COMP_UNITYGAIN * 3 / 4)) ? _VT100_GREEN :
	                     (gain > (COMP_UNITYGAIN / 2))      ? _VT100_YELLOW : _VT100_RED;

	ttprintf("%s|%s %sState:%s %s  Gain: ",
		TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_CYAN),
		TERM_getVT100Code(_VT100_RESET_ATTRIB, 0),
		TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_YELLOW),
		TERM_getVT100Code(_VT100_RESET_ATTRIB, 0),
		sname);
	print_bar(handle, gain, COMP_UNITYGAIN, gain_color);
	ttprintf(" %5u  maxDuty:%u%s\r\n", gain, maxDuty, EL);
	print_hline(handle);
}

/* ── title banner ─────────────────────────────────────────────────────────── */

static void print_banner(TERMINAL_HANDLE *handle, uint8_t synth) {
	static const char *logo[] = {
		" _   _  ____   _____",
		"| | | ||  _ \\  |___ /",
		"| | | || | | |  |_ \\",
		"| |_| || |_| | ___) |",
		" \\___/ |____/ |____/ ",
	};

	static const char *mode_str[] = {"OFF", "MIDI", "SID", "TR", "MIDI+QCW", "SID+QCW"};
	const char *mstr = (synth < 6) ? mode_str[synth] : "?";
	uint8_t mcolor = (synth == SYNTH_OFF)                              ? _VT100_RED :
	                 (synth == SYNTH_SID || synth == SYNTH_SID_QCW)    ? _VT100_MAGENTA
	                                                                    : _VT100_CYAN;

	static const char *sidebar[] = {
		"  synthmon",
		"  UD3 Synthesizer Monitor",
		"",
		"  [q] quit",
	};

	for (uint8_t r = 0; r < 5; r++) {
		ttprintf("%s%s%s",
			TERM_getVT100Code(_VT100_BRIGHT, 0),
			TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_CYAN),
			logo[r]);
		ttprintf("%s", TERM_getVT100Code(_VT100_RESET_ATTRIB, 0));
		if (r < 4) {
			if (r == 2) {
				ttprintf("  Mode: %s%s%s%s",
					TERM_getVT100Code(_VT100_BRIGHT, 0),
					TERM_getVT100Code(_VT100_FOREGROUND_COLOR, mcolor),
					mstr,
					TERM_getVT100Code(_VT100_RESET_ATTRIB, 0));
			} else {
				ttprintf("%s%s%s",
					TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_WHITE),
					sidebar[r],
					TERM_getVT100Code(_VT100_RESET_ATTRIB, 0));
			}
		}
		ttprintf("%s\r\n", EL);
	}
	if (!synth) {
		print_hline(handle);
	}
}

/* ── main task ────────────────────────────────────────────────────────────── */

void TASK_synthmon(void *pvParameters) {
	TERMINAL_HANDLE *handle = (TERMINAL_HANDLE *)pvParameters;
	char c = 0;

	TERM_sendVT100Code(handle, _VT100_CURSOR_DISABLE, 0);
	TERM_sendVT100Code(handle, _VT100_CLS, 0);

	do {
		TERM_setCursorPos(handle, 1, 1);

		uint8_t synth = param.synth;

		print_banner(handle, synth);

		if (synth == SYNTH_SID || synth == SYNTH_SID_QCW) {
			print_sid(handle);
			print_compressor(handle);
		} else if (synth == SYNTH_MIDI || synth == SYNTH_MIDI_QCW) {
			print_midi(handle);
			print_compressor(handle);
		} else {
			ttprintf("  %sNo synthesizer active%s%s\r\n",
				TERM_getVT100Code(_VT100_FOREGROUND_COLOR, _VT100_RED),
				TERM_getVT100Code(_VT100_RESET_ATTRIB, 0),
				EL);
			print_hline(handle);
		}

		ttprintf("\x1b[J");

		xStreamBufferReceive(handle->currProgram->inputStream, &c, sizeof(c), pdMS_TO_TICKS(500));
	} while (c != CTRL_C);

	TERM_sendVT100Code(handle, _VT100_CURSOR_ENABLE, 0);
	TERM_sendVT100Code(handle, _VT100_CLS, 0);
	TERM_killProgramm(handle);
}

/* ── input handler ────────────────────────────────────────────────────────── */

uint8_t INPUT_synthmon(TERMINAL_HANDLE *handle, uint16_t c) {
	if (handle->currProgram->inputStream == NULL) return TERM_CMD_EXIT_SUCCESS;
	switch (c) {
		case 'q':
		case CTRL_C:
			c = CTRL_C;
			xStreamBufferSend(handle->currProgram->inputStream, &c, 1, 20);
			return TERM_CMD_EXIT_SUCCESS;
		default:
			return TERM_CMD_CONTINUE;
	}
}
