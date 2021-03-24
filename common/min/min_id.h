#ifndef MIN_ID_H
#define MIN_ID_H

#define MIN_ID_TERM     0
#define MIN_ID_WD       10
#define MIN_ID_RESET    11
#define MIN_ID_COMMAND  12
#define MIN_ID_SOCKET   13
#define MIN_ID_SYNTH    14
#define MIN_ID_FEATURE  15
#define MIN_ID_MIDI     20
#define MIN_ID_SID      21
    
#define MIN_ID_EVENT    40
#define MIN_ID_ALARM    41
#define MIN_ID_DEBUG    42
    
#define SYNTH_CMD_FLUSH     1
#define SYNTH_CMD_SID       2
#define SYNTH_CMD_MIDI      3
#define SYNTH_CMD_OFF       4

    
#define SOCKET_DISCONNECTED  0
#define SOCKET_CONNECTED     1
    
#define CMD_HELLO_WORLD     0x01    
#define CMD_FEATURE_FRAME   0x02
#define CMD_LINK            0x03
    
#define EVENT_GET_INFO      1    
    
#endif