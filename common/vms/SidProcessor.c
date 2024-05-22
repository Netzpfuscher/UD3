#include "SidProcessor.h"
#include <device.h>
#include <stdint.h>
#include "queue.h"
#include "telemetry.h"
#include "clock.h"
#include "qcw.h"
#include "tasks/tsk_sid.h"
#include "SidFilter.h"
#include <stdlib.h>

static const uint32_t ADSR_speedLut_A[] = {65535, 16384, 8192, 5459, 3449, 2341, 1928, 1638, 1311, 524, 262, 164, 131, 44, 26, 16};
static const uint32_t ADSR_speedLut_DR[] = {39779, 57846, 61571, 62865, 63836, 64378, 64581, 64723, 64885, 65275, 65405, 65454, 65471, 65514, 65523, 65528};
	
static SIDChannelData_t channelData[N_SIDCHANNEL];

void SidProcessor_resetSid(){
    //flush frame buffer
    //TODO figure out why this doesn't work :(
    //xQueueReset(qSID);
    
    //and reset all channels
    for(uint32_t i = 0; i < N_SIDCHANNEL; i ++){
        channelData[i].flags = 0;
        
        channelData[i].frequency_dHz = 0;
        
        SidFilter_updateVoice(i, &channelData[i]);
    }
}

SIDChannelData_t * SID_getChannelData(){
    return channelData;
}

const char * SID_getWaveName(SIDChannelData_t * data){
    if(data->flags & SID_FRAME_FLAG_NOISE) return "NOISE";
    if(data->flags & SID_FRAME_FLAG_SQUARE) return "SQUARE";
    if(data->flags & SID_FRAME_FLAG_SAWTOOTH) return "SAWTOOTH";
    if(data->flags & SID_FRAME_FLAG_TRIANGLE) return "TRIANGLE";
    
    return "?";
}

void SidProcessor_handleFrame(SIDFrame_t * frame){
    //is the frame NULL? if that is the case then a timeout occured and we need to KILL EVERYTHING
    
    if(frame == NULL){
        //reset all sid stuff
        SidProcessor_resetSid();
        
        return;
    }
    
    //we got a sid frame. Process it and update the voice.
    for(uint32_t i = 0; i < N_SIDCHANNEL; i ++){
        channelData[i].flags = frame->flags[i];
    
        channelData[i].attack = frame->attack[i];
        channelData[i].decay = frame->decay[i];
        channelData[i].sustainVolume = frame->sustain[i] * 2184;   //scale sustain volume into the 0-MAX_VOL range.
        channelData[i].release = frame->release[i];
        
        channelData[i].frequency_dHz = frame->frequency_dHz[i];
        channelData[i].pulsewidth = frame->pulsewidth[i];
        
        //why only update the voice after we received a new register frame? well thats because we compute ADSR independently of this here and the 50/60hz at which this data shows up
        //is still plenty for updating interrupter parameters
        SidFilter_updateVoice(i, &channelData[i]);
    }
}

//run through all channels and calculate ADSR. This is currently designed to run every millisecond
//TODO maybe make this slower?
void SidProcessor_runADSR(){
    for(uint32_t currentChannel = 0; currentChannel < N_SIDCHANNEL; currentChannel++){
        
        //what state are we in?
        switch(channelData[currentChannel].adsrState) {
            //ADSR IDLE / channel off => check if the gate was just turned on
        	case ADSR_IDLE:
        		if(channelData[currentChannel].flags & SID_FRAME_FLAG_GATE) channelData[currentChannel].adsrState = ADSR_ATTACK;
        		channelData[currentChannel].currentEnvelopeVolume = 0;
        		break;
        		
            //Attack => linearly increase volume until we reach either MAX_VOL if a decay would happen after this oder sustain volume if decay is off
        	case ADSR_ATTACK:
                //is the channel still active?
                if(!(channelData[currentChannel].flags & SID_FRAME_FLAG_GATE)) { 
                    //no actually :( Switch to release mode and make sure to remember what volume we reached
        			channelData[currentChannel].adsrState = ADSR_RELEASE;
        			channelData[currentChannel].currentEnvelopeStartValue = channelData[currentChannel].currentEnvelopeVolume;
        			channelData[currentChannel].currentEnvelopeFactor = MAX_VOL;
        			break;
        		}
                
                //increase current volume. Speed is given by attack parameter
        		channelData[currentChannel].currentEnvelopeVolume += ADSR_speedLut_A[channelData[currentChannel].attack];
                
                //what threshold do we need to compare against? If we reached or exceeded it set to target and continue to next state
        		if(channelData[currentChannel].decay > 0) {
                    //decay is enabled => we must reach 100% volume 
        			if(channelData[currentChannel].currentEnvelopeVolume >= MAX_VOL) {
        				channelData[currentChannel].adsrState = ADSR_DECAY;
        				channelData[currentChannel].currentEnvelopeFactor = MAX_VOL;
        				channelData[currentChannel].currentEnvelopeVolume = MAX_VOL;
        			}
        		}else {
                    //decay is enabled => we only need to reach the sustain level
        			if(channelData[currentChannel].currentEnvelopeVolume >= channelData[currentChannel].sustainVolume) {
        				channelData[currentChannel].adsrState = ADSR_SUSTAIN;
        				channelData[currentChannel].currentEnvelopeFactor = MAX_VOL;
        				channelData[currentChannel].currentEnvelopeVolume = channelData[currentChannel].sustainVolume;
        			}
        		}
        		break;
        		
            //decay => exponentially reduce volume from max_vol (attack target) to the sustain value
        	case ADSR_DECAY:
                //is the channel still active?
        		if(!(channelData[currentChannel].flags & SID_FRAME_FLAG_GATE)){ 
                    //no actually :( Switch to release mode and make sure to remember what volume we reached
        			channelData[currentChannel].adsrState = ADSR_RELEASE;
        			channelData[currentChannel].currentEnvelopeStartValue = channelData[currentChannel].currentEnvelopeVolume;
        			channelData[currentChannel].currentEnvelopeFactor = MAX_VOL;
        			break;
        		}
                
                //exponentially decrease volume at speed given by the decay parameter
        		channelData[currentChannel].currentEnvelopeFactor = (channelData[currentChannel].currentEnvelopeFactor * ADSR_speedLut_DR[channelData[currentChannel].decay]) >> 16;
                
                //check if we reached the target
        		if(channelData[currentChannel].currentEnvelopeFactor <= 3500) {
                    //yes sustain volume reached => go to next state
        			channelData[currentChannel].adsrState = ADSR_SUSTAIN;
        			channelData[currentChannel].currentEnvelopeVolume = channelData[currentChannel].sustainVolume;
        		}else{
                    //no not yet => update adsr volume
                    //This is equal to the exponential start volume (MAX_VOL in this case) minus the difference between it and the target multiplied by the current decay factor (0-MAX_VOL)
            		channelData[currentChannel].currentEnvelopeVolume = channelData[currentChannel].sustainVolume + (((MAX_VOL - channelData[currentChannel].sustainVolume) * channelData[currentChannel].currentEnvelopeFactor) >> 15);
                }
        		break;
        		
        	case ADSR_SUSTAIN:
                //is the channel still active?
        		if(!(channelData[currentChannel].flags & SID_FRAME_FLAG_GATE)){ 
                    //no. Switch to release mode and make sure to remember what volume we reached
        			channelData[currentChannel].adsrState = ADSR_RELEASE;
        			channelData[currentChannel].currentEnvelopeStartValue = channelData[currentChannel].currentEnvelopeVolume;
        			channelData[currentChannel].currentEnvelopeFactor = MAX_VOL;
        		}
        		break;
        		
        	case ADSR_RELEASE:
                //is the channel still inactive?
        		if(channelData[currentChannel].flags & SID_FRAME_FLAG_GATE) { 
                    //no, note turned on again. Restart adsr with the attack state
        			channelData[currentChannel].adsrState = ADSR_ATTACK;
        			channelData[currentChannel].currentEnvelopeStartValue = channelData[currentChannel].currentEnvelopeVolume;
        			channelData[currentChannel].currentEnvelopeFactor = MAX_VOL;
        			break;
        		}
                
                //exponentially decrease volume at speed given by the release parameter
        		channelData[currentChannel].currentEnvelopeFactor = (channelData[currentChannel].currentEnvelopeFactor * ADSR_speedLut_DR[channelData[currentChannel].release]) >> 16;
                
                //check if we reached the target
        		if(channelData[currentChannel].currentEnvelopeFactor <= 3500) {
                    //yes => go to idle mode
        			channelData[currentChannel].adsrState = ADSR_IDLE;
        			channelData[currentChannel].currentEnvelopeVolume = 0;
        		}else{
                    //no => update volume
                    //equal to a exponential decay down to zero from currentEnvelopeStartValue
                    channelData[currentChannel].currentEnvelopeVolume = (channelData[currentChannel].currentEnvelopeStartValue * channelData[currentChannel].currentEnvelopeFactor) >> 15;
                }
        		break;
    	}
    }
}