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

#include "SignalGenerator.h"
#include "SignalGeneratorSID.h"
#include "SignalGeneratorMIDI.h"
#include "clock.h"
#include "qcw.h"
#include "interrupter.h"
#include "tasks/tsk_midi.h"
#include "telemetry.h"
#include <device.h>
#include <stdlib.h>
#include "MidiController.h"
#include "mapper.h"
#include "telemetry.h"
#include "tasks/tsk_cli.h"

// Tone generator channel status (updated according to MIDI messages)
CHANNEL channel[N_CHANNEL];

volatile uint32_t* test;

uint32_t hyper[2]={0x01FFFFFF,0x01FFFFFF};


void init_hyper(){
 /* Defines for Hyper_Ch1 */
    #define Hyper_Ch1_BYTES_PER_BURST 3
    #define Hyper_Ch1_REQUEST_PER_BURST 1
    #define Hyper_Ch1_SRC_BASE (CYDEV_SRAM_BASE)
    #define Hyper_Ch1_DST_BASE (CYDEV_PERIPH_BASE)

    /* Variable declarations for Hyper_Ch1 */
    /* Move these variable declarations to the top of the function */
    uint8 Hyper_Ch1_Chan;
    uint8 Hyper_Ch1_TD[4];

    /* DMA Configuration for Hyper_Ch1 */
    Hyper_Ch1_Chan = Hyper_Ch1_DmaInitialize(Hyper_Ch1_BYTES_PER_BURST, Hyper_Ch1_REQUEST_PER_BURST, 
        HI16(Hyper_Ch1_SRC_BASE), HI16(Hyper_Ch1_DST_BASE));
    Hyper_Ch1_TD[0] = CyDmaTdAllocate();
    Hyper_Ch1_TD[1] = CyDmaTdAllocate();
    Hyper_Ch1_TD[2] = CyDmaTdAllocate();
    Hyper_Ch1_TD[3] = CyDmaTdAllocate();
   
    CyDmaTdSetConfiguration(Hyper_Ch1_TD[0], 2, Hyper_Ch1_TD[1], TD_AUTO_EXEC_NEXT);
    CyDmaTdSetConfiguration(Hyper_Ch1_TD[1], 1, Hyper_Ch1_TD[2], 0);
    CyDmaTdSetConfiguration(Hyper_Ch1_TD[2], 2, Hyper_Ch1_TD[3], TD_AUTO_EXEC_NEXT);
    CyDmaTdSetConfiguration(Hyper_Ch1_TD[3], 1, Hyper_Ch1_TD[0], 0);
    CyDmaTdSetAddress(Hyper_Ch1_TD[0], LO16((uint32)&hyper[0]), LO16((uint32)((uint16_t *)DDS32_1_sD24_DDSdp_u0__16BIT_D0_REG)));
    CyDmaTdSetAddress(Hyper_Ch1_TD[1], LO16((uint32)&hyper[0])+2, LO16((uint32)((uint16_t *)DDS32_1_sD24_DDSdp_u2__16BIT_D0_REG)));
    CyDmaTdSetAddress(Hyper_Ch1_TD[2], LO16((uint32)&hyper[1]), LO16((uint32)((uint16_t *)DDS32_1_sD24_DDSdp_u0__16BIT_D0_REG)));
    CyDmaTdSetAddress(Hyper_Ch1_TD[3], LO16((uint32)&hyper[1])+2, LO16((uint32)((uint16_t *)DDS32_1_sD24_DDSdp_u2__16BIT_D0_REG)));
    CyDmaChSetInitialTd(Hyper_Ch1_Chan, Hyper_Ch1_TD[0]);
    //CyDmaChEnable(Hyper_Ch1_Chan, 1);

}

    


CY_ISR(isr_synth) {
    uint32_t r = SG_Timer_ReadCounter();
    clock_tick();
    if(qcw_reg){
        qcw_handle();
        return;
    }
    switch(param.synth){
        case SYNTH_MIDI:
            synthcode_MIDI(r);
            break;
        case SYNTH_SID:
            synthcode_SID(r);
            break;
        case SYNTH_SID_QCW:
            synthcode_QSID(r);
            break;
        default:
            break;
    }
}

struct _filter filter;

/*****************************************************************************
* Switches the synthesizer
******************************************************************************/
uint8_t callback_SynthFunction(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    SigGen_switch_synth(param.synth);
    return 1;
}

uint8_t callback_synthFilter(parameter_entry * params, uint8_t index, TERMINAL_HANDLE * handle){
    uint8_t cnt=0;
    uint16_t number=0;
    char substring[20];
    uint8_t str_size = strlen(configuration.synth_filter);
    uint8_t flag=0;
    
    filter.min=0;
    filter.max=20000;
    if(configuration.synth_filter[0]=='\0'){  //No filter
        for(uint8_t i=0;i<16;i++){
            filter.channel[i]=pdTRUE;
        }
        return 1;
    }else{
        for(uint8_t i=0;i<16;i++){
            filter.channel[i]=pdFALSE;
        }
    }
    
    for(uint8_t i=0;i<str_size;i++){
        if(configuration.synth_filter[i]=='\0') break;
        
        if(configuration.synth_filter[i]=='c'){
            cnt=0;
            substring[0]='\0';
            for(uint8_t w=i+1;w<str_size;w++){
                if(isdigit(configuration.synth_filter[w])){
                    substring[cnt]=configuration.synth_filter[w];
                    cnt++;
                }else{
                    substring[cnt]='\0';
                    break;
                }
            }
            if(substring[0]){
                number = atoi(substring);
                if(number<16){
                    filter.channel[number]=pdTRUE;
                    flag=1;
                }
            }  
        }else if(configuration.synth_filter[i]=='f'){
            if(configuration.synth_filter[i+1]=='>' || configuration.synth_filter[i+1]=='<'){
                cnt=0;
                for(uint8_t w=i+2;w<str_size;w++){
                    if(isdigit(configuration.synth_filter[w])){
                        substring[cnt]=configuration.synth_filter[w];
                        cnt++;
                    }else{
                        substring[cnt]='\0';
                        break;
                    }
                }
                if(cnt){
                    number = atoi(substring);
                    if(number<20000){
                        if(configuration.synth_filter[i+1]=='>') filter.max = number;
                        if(configuration.synth_filter[i+1]=='<') filter.min = number;
                    }
                } 
            }
        }     
    }
    if(filter.min > filter.max){
        ttprintf("Error: Min frequency ist greater than max\r\n");
    }
    if(!flag){
        for(uint8_t i=0;i<16;i++){
            filter.channel[i]=pdTRUE;
        }   
    }

    for(uint8_t i=0;i<sizeof(filter.channel);i++){
        ttprintf("Channel %u: %u\r\n",i,filter.channel[i]);
    }
    ttprintf("Min frequency: %u\r\n",filter.min);
    ttprintf("Max frequency: %u\r\n",filter.max);

    return 1;
}

void SigGen_channel_enable(uint8_t ch, uint32_t ena){
    switch(ch){
        case 0:
            if(ena) DDS32_1_Enable_ch(0); else DDS32_1_Disable_ch(0);
            break;
        case 1:
            if(ena) DDS32_1_Enable_ch(1); else DDS32_1_Disable_ch(1);
            break;
        case 2:
            if(ena) DDS32_2_Enable_ch(0); else DDS32_2_Disable_ch(0);
            break;
        case 3:
            if(ena) DDS32_2_Enable_ch(1); else DDS32_2_Disable_ch(1);
            break;
    }
 }

size_t SigGen_get_channel(uint8_t ch){
    switch(ch){
        case 0:
            return (DDS32_1_sCTRLReg_ctrlreg__CONTROL_REG & (1u << 1)) ? pdTRUE : pdFALSE;
        case 1:
            return (DDS32_1_sCTRLReg_ctrlreg__CONTROL_REG & (1u << 2)) ? pdTRUE : pdFALSE;
        case 2:
            return (DDS32_2_sCTRLReg_ctrlreg__CONTROL_REG & (1u << 1)) ? pdTRUE : pdFALSE;
        case 3:
            return (DDS32_2_sCTRLReg_ctrlreg__CONTROL_REG & (1u << 2)) ? pdTRUE : pdFALSE;
    }
    return pdFALSE;
 }



uint16_t SigGen_channel_freq_fp8(uint8_t ch, uint32_t freq){
    switch(ch){
        case 0:
            return DDS32_1_SetFrequency_FP8(0,freq);
        break;
        case 1:
            return DDS32_1_SetFrequency_FP8(1,freq);
        break;
        case 2:
            return DDS32_2_SetFrequency_FP8(0,freq);
        break;
        case 3:
            return DDS32_2_SetFrequency_FP8(1,freq);
        break;
    }
    return 0;
}

void SigGen_noise(uint8_t ch, uint32_t ena, uint32_t rnd){
    if(ena==0) return;
    switch(ch){
        case 0:
            DDS32_1_WriteRand0(rnd);
            break;
        case 1:
            DDS32_1_WriteRand1(rnd);
            break;
        case 2:
            DDS32_2_WriteRand0(rnd);
            break;
        case 3:
            DDS32_2_WriteRand1(rnd);
            break;
    }
}

void SigGen_setNoteTPR(uint8_t voice, uint32_t freqTenths){
    SigGen_limit();
    
    Midi_voice[voice].freqCurrent = freqTenths;
    channel[voice].freq = (freqTenths / 10);
    
    uint32_t freq = ((channel[voice].freq)<<8) + ((0xFF *  (freqTenths % 10)) / 10);
    if(freqTenths != 0){
        channel[voice].halfcount = (SG_CLOCK_HALFCOUNT<<14) / (freq<<6);
        switch(voice){
            case 0:
                DDS32_1_SetFrequency_FP8(0,freq);
                hyper[0] = DDS32_1_tune_word[0];
                hyper[1] = DDS32_1_tune_word[0];
                DDS32_1_Enable_ch(0);
            break;
            case 1:
                DDS32_1_SetFrequency_FP8(1,freq);
                DDS32_1_Enable_ch(1);
            break;
            case 2:
                DDS32_2_SetFrequency_FP8(0,freq);
                DDS32_2_Enable_ch(0);
            break;
            case 3:
                DDS32_2_SetFrequency_FP8(1,freq);
                DDS32_2_Enable_ch(1);
            break;
        }
        return;  
    }else{
        switch(voice){
        case 0:
            DDS32_1_Disable_ch(0);
            break;
        case 1:
            DDS32_1_Disable_ch(1);
            break;
        case 2:
            DDS32_2_Disable_ch(0);
            break;
        case 3:
            DDS32_2_Disable_ch(1);
            break;
        }
 
    }
}

uint8_t SigGen_masterVol = 255;

void SigGen_limit(){ //<------------------------Todo Ontime and so on
    
    uint32_t totalDuty = 0;
    uint32_t scale = 1000;
    uint8_t c = 0;
    
    for(; c < MIDI_VOICECOUNT; c++){
        uint32_t ourDuty = (Midi_voice[c].otCurrent * Midi_voice[c].freqCurrent) / 10; //TODO preemtively increase the frequency used for calculation if noise is on
        if(Midi_voice[c].hyperVoiceCount == 2 && Midi_voice[c].noiseCurrent == 0) ourDuty *= 2;
        totalDuty += ourDuty;
    }
    
    //UART_print("duty = %d%% -> scale = %d%%m\r\n", totalDuty / 10000, scale);
    
    
    tt.n.midi_voices.value = 0;
    for(c = 0; c < MIDI_VOICECOUNT; c++){
        uint32_t ot;
        ot = (Midi_voice[c].otCurrent * scale) / 1330;
        ot = (ot * SigGen_masterVol) / 255;
        Midi_voice[c].outputOT = ot;
        if(Midi_voice[c].outputOT){
            tt.n.midi_voices.value++;
        }
        uint32_t out = Midi_voice[c].outputOT<<12;
        interrupter_set_pw_vol(c, param.pw, out);
        channel[c].volume = out;
        
        
        SigGen_channel_enable(c, out);
    }
}

void SigGen_init(){
    
    SigGen_SID_init();
    
    //init_hyper();

    for (uint8_t ch = 0; ch < N_CHANNEL; ch++) {
        channel[ch].volume=0;
        channel[ch].freq=0;
        channel[ch].halfcount=0;
        SigGen_channel_enable(ch,0);
	}
    
    isr_midi_StartEx(isr_synth);
}

void SigGen_switch_synth(uint8_t synth){
    if(configuration.is_qcw){
        if(param.synth==SYNTH_MIDI) param.synth = SYNTH_MIDI_QCW;
        if(param.synth==SYNTH_SID) param.synth = SYNTH_SID_QCW;
    }
    if(synth==SYNTH_OFF){
        interrupter_DMA_mode(INTR_DMA_TR);
        if(configuration.ext_interrupter) interrupter_update_ext();
    }else if(synth==SYNTH_MIDI || synth==SYNTH_SID){
        interrupter_DMA_mode(INTR_DMA_DDS);       
    }
    tt.n.midi_voices.value=0;
    xQueueReset(qMIDI_rx);
    xQueueReset(qSID);
    SigGen_kill(); 
}

void Synthmon_MIDI(TERMINAL_HANDLE * handle){
    
    uint32_t freq=0;
    
    TERM_sendVT100Code(handle, _VT100_CURSOR_DISABLE,0);
    TERM_sendVT100Code(handle, _VT100_CLS,0);
    ttprintf("Synthesizer monitor    [CTRL+C] for quit\r\n");
    ttprintf("-----------------------------------------------------------\r\n");
    while(Term_check_break(handle,100)){
        TERM_setCursorPos(handle, 3,1);
        
        for(uint8_t i=0;i<N_CHANNEL;i++){

            freq= Midi_voice[i].freqCurrent/10;
            uint8_t noteOrigin=Midi_voice[i].currNoteOrigin;
            
            TERM_sendVT100Code(handle, _VT100_CURSOR_SET_COLUMN, 0);
            ttprintf("Ch:   ");
            TERM_sendVT100Code(handle, _VT100_CURSOR_SET_COLUMN, 0);
            ttprintf("Ch: %u", i+1);
            TERM_sendVT100Code(handle, _VT100_CURSOR_SET_COLUMN, 7);
            ttprintf("Freq:      ");
            TERM_sendVT100Code(handle, _VT100_CURSOR_SET_COLUMN, 7);
            ttprintf("Freq: %u", freq);
            TERM_sendVT100Code(handle, _VT100_CURSOR_SET_COLUMN, 20);
            ttprintf("Prog:   ");
            TERM_sendVT100Code(handle, _VT100_CURSOR_SET_COLUMN, 20);
            ttprintf("Prog: %u", channelData[noteOrigin].currProgramm);
    
            MAPTABLE_HEADER* map = MAPPER_findHeader(channelData[noteOrigin].currProgramm);
            TERM_sendVT100Code(handle, _VT100_CURSOR_SET_COLUMN, 28);
            ttprintf("Name:                  ", map->name);
            TERM_sendVT100Code(handle, _VT100_CURSOR_SET_COLUMN, 28);
            ttprintf("Name: %s", map->name);
            
            TERM_sendVT100Code(handle, _VT100_CURSOR_SET_COLUMN, 50);
            ttprintf("Vol: ",i+1);

            uint8_t cnt = (channel[i].volume>>16)/12;

            for(uint8_t w=0;w<10;w++){
                if(w<cnt){
                    ttprintf("o");
                }else{
                    ttprintf(" ");
                }
            }
            ttprintf("\r\n");
        }
    }
    ttprintf("\r\n");
    TERM_sendVT100Code(handle, _VT100_CURSOR_ENABLE,0);    
    
}

void Synthmon_SID(TERMINAL_HANDLE * handle){
    
    uint32_t freq=0;
    
    TERM_sendVT100Code(handle, _VT100_CURSOR_DISABLE,0);
    TERM_sendVT100Code(handle, _VT100_CLS,0);
    ttprintf("Synthesizer monitor    [CTRL+C] for quit\r\n");
    ttprintf("-----------------------------------------------------------\r\n");
    while(Term_check_break(handle,100)){
        TERM_setCursorPos(handle, 3,1);
        
        for(uint8_t i=0;i<SID_CHANNELS;i++){
            ttprintf("Ch:   Freq:      \r");
            if(channel[i].volume>0){
                freq=channel[i].freq;
            }else{
                freq=0;
            }
            ttprintf("Ch: %u Freq: %u",i+1,freq);             
            TERM_sendVT100Code(handle, _VT100_CURSOR_SET_COLUMN, 20);
            ttprintf("Vol: ",i+1);
            uint8_t cnt = (channel[i].volume>>16)/12;

            for(uint8_t w=0;w<10;w++){
                if(w<cnt){
                    ttprintf("o");
                }else{
                    ttprintf(" ");
                }
            }
            ttprintf("\r\n");
        }
    }
    ttprintf("\r\n");
    TERM_sendVT100Code(handle, _VT100_CURSOR_ENABLE,0);
    
    
}



uint8_t CMD_SynthMon(TERMINAL_HANDLE * handle, uint8_t argCount, char ** args){
    if(param.synth == SYNTH_SID || param.synth == SYNTH_SID_QCW) Synthmon_SID(handle);
    if(param.synth == SYNTH_MIDI || param.synth == SYNTH_MIDI_QCW) Synthmon_MIDI(handle);
    return TERM_CMD_EXIT_SUCCESS;
}

void SigGen_kill(){
    for (uint8_t ch = 0; ch < N_CHANNEL; ch++) {
        channel[ch].volume=0;
        channel[ch].freq=0;
        channel[ch].halfcount=0;
        SigGen_setNoteTPR(ch,0);
	}
}



