/*
 * File:   MidiFilter.c
 * Author: Thorb
 *
 * Created on 12 September 2023, 23:15
 */

#include "NoteMapper.h"
#include "MidiFilter.h"
#include "TTerm.h"
#include "SidProcessor.h"
#include "tasks/tsk_sid.h"
#include "interrupter.h"
#include "cli_common.h"

SIDFilterData_t SID_filterData = {
    .channelVolume = {[0 ... N_SIDCHANNEL-1] = MAX_VOL},
    .hpvEnabled = {[0 ... N_SIDCHANNEL-1] = 1},
    .flags = 0,
    .noiseVolume = 10000, //set to 20% = (6553 >> 15) as a default
    .hpvEnabledGlobally = 0
};

//test if selected wave is sqare and noise is off
#define SID_IS_HPV_AVAILABLE(FLAGS) ((FLAGS & SID_FRAME_FLAG_SQUARE) && !(FLAGS & SID_FRAME_FLAG_NOISE) && !(FLAGS & SID_FRAME_FLAG_SAWTOOTH) && (SID_filterData.hpvEnabledGlobally))

void SidFilter_updateVoice(uint32_t channel, SIDChannelData_t * channelData){
    
    //is this voice even on?
    if(    channelData->currentEnvelopeVolume != 0      //adsr volume > 0?
        && channelData->frequency_dHz != 0              //frequency not set to 0 (done when resetting voices after sid timeout for example)
        && !(channelData->flags & SID_FRAME_FLAG_TEST)  //test bit not set, that would have the side effect of disabling output on a realy sid
        && SID_filterData.channelVolume[channel] != 0   //master volume for the channel > 0?
        && (channel != 2 || !(channelData->flags & SID_FRAME_FLAG_CH3OFF))  //check for ch3off bit, of course only for channel 3
    ){
        //yes
        
        //calculate ontime 
            uint32_t ontime = param.pw;
        
        //calculate frequency
            uint32_t frequency_dHz = channelData->frequency_dHz; //not much to calc :)
    
        //calculate volume. Starting from max and scaling down with every parameter we have
            uint32_t totalVolume = MAX_VOL;
            
            //channel ADSR
            if(channelData->currentEnvelopeVolume != MAX_VOL) totalVolume = (totalVolume * channelData->currentEnvelopeVolume) >> 15;
            
            //channel filter and master volume
            if(SID_filterData.channelVolume[channel] != MAX_VOL) totalVolume = (totalVolume * SID_filterData.channelVolume[channel]) >> 15;
            
            //noise volume (if noise is on)
            if((channelData->flags & SID_FRAME_FLAG_NOISE) && (SID_filterData.noiseVolume != MAX_VOL)) totalVolume = (totalVolume * SID_filterData.noiseVolume) >> 15;
            
            totalVolume += (channelData->frequency_dHz>>3);
            if(totalVolume > MAX_VOL) totalVolume = MAX_VOL;
            
            //half volume scale for hpv is done in hpv calc code
   
        //calculate hpv stuff
            uint32_t hpvCount = 0;
            uint32_t hpvPhase = 0;
            uint32_t hpvVolume = totalVolume;
            
            //is hpv even required?
            //TODO evaluate having a hpv enable for every voice
            if(SID_IS_HPV_AVAILABLE(channelData->flags)){ // && (SID_filterData.hpvEnabled[channel])
                //yes => enable and calc phase
                hpvCount = 1; 
                hpvPhase = channelData->pulsewidth >> 4;
                
                //also scale volume down by half
                //TODO evaluate how good this actually sounds
                //totalVolume = totalVolume >> 1;
                //ontime = ontime >> 1;
                
                
                //start scaling down the volume starting from 1kHz to 1,32767kHz
                if(channelData->frequency_dHz > 10000){
                    hpvVolume = (hpvVolume * (18192 - channelData->frequency_dHz)) >> 13;
                }else if(channelData->frequency_dHz >= 18192){
                    hpvVolume = 0;
                }
            }
            
        //is noise required?
            uint32_t noiseAmplitude = 0;
            
            if(channelData->flags & SID_FRAME_FLAG_NOISE){
                //yes => set noise target
                noiseAmplitude = 80;
                
                /*
                    we will want to scale the frequency back a little too
                
                
                    how the frequency is scaled:
                    
                    in the earlier code we used the function newFreq = sqrt(freq*2) * 40; whis is probably completely picked at random because I had cpu cycles to spare
                
                    now however I don't, so we will approximate linearly. The graph looks kinda like a line after a few hundred herz anyway :P Linear scale target: 2000-5000Hz from 0-20000Hz note frequency
                
                    y = m*x + b;
                
                    b = 2000;
                    m = 3000Hz / 20000Hz = 0.15 = 2458 / 16384 = 2458 >> 14
                
                    and then everything needs to be x10 due ti the frequency being in dHz
                        
                    //TODO actually evaluate how this sounds
                */
                
                frequency_dHz = 20000 + ((frequency_dHz * 24576) >> 14);
                
                
            }
            
        //and finally update siggen
            SigGen_setVoiceSID(channel, 1, ontime, totalVolume, frequency_dHz, noiseAmplitude);
            SigGen_setHyperVoiceSID(channel, hpvCount, ontime, totalVolume, hpvPhase);
            
    }else{
        //no, channel muted. Just switch it off in siggen
        SigGen_setVoiceSID(channel, 0, 0, 0, 0, 0);
    }
    
}