#ifndef NOTEMAPPER_INC
#define NOTEMAPPER_INC
    
#include <stdint.h>

#define MAP_ENA_PITCHBEND   0x80
#define MAP_ENA_STEREO      0x40
#define MAP_ENA_VOLUME      0x20
#define MAP_ENA_DAMPER      0x10
#define MAP_ENA_PORTAMENTO  0x08
#define MAP_ENA_EQ          0x04
#define MAP_FREQ_MODE       0x01

#define MAPPER_ALL_PROGRAMMS 0xff
#define MAPPER_VOLUME_MAX 0xff

#define FREQ_MODE_OFFSET    1
#define FREQ_MODE_FIXED     0

#define MAPPER_MAPMEM_SIZE  2048
#define MAPPER_MAP_NAME_LENGTH  18


#define MAPPER_VERSION 2

#define MAPPER_ENTRY_FROM_HEADER(HEADER_PTR, IDX) (MAPTABLE_ENTRY_t *) ((uint32_t ) HEADER_PTR + sizeof(MAPTABLE_HEADER_t) + IDX * sizeof(MAPTABLE_ENTRY_t))

typedef struct _MapData_ MAPTABLE_DATA_t;
typedef struct _MapEntry_ MAPTABLE_ENTRY_t;
typedef struct _MapHeader_ MAPTABLE_HEADER_t;

struct _MapData_{
    int16_t noteFreq;
    uint8_t targetOT;
    uint8_t flags;
    uint16_t startblockID;
} __attribute__((packed));

struct _MapEntry_{
    uint8_t startNote;
    uint8_t endNote;
    MAPTABLE_DATA_t data;
} __attribute__((packed));

struct _MapHeader_{
    uint8_t listEntries;
    uint8_t programNumberStart;
    uint8_t programNumberEnd;
    char name[MAPPER_MAP_NAME_LENGTH];
} __attribute__((packed));

//legacy data type definitions... required to allow the old config software to still work((packed)) LegayMapEntry;

typedef struct {
    uint8_t listEntries;
    uint8_t programNumberStart;
    char name[18];
} __attribute__((packed)) LegayMapHeader_t;

typedef struct {
    int16_t noteFreq;
    uint8_t targetOT;
    uint8_t flags;
    uint32_t startblockID;  //changed from VMS_Block pointer. This is acceptable behaviour as the usb layer communication used unliked lists anyway (with ID's instead of pointers)
} __attribute__((packed)) LegayMapData_t;

typedef struct {
    uint8_t startNote;
    uint8_t endNote;
    LegayMapData_t data;
} __attribute__((packed)) LegayMapEntry_t;


void Mapper_noteOffEventHandler(uint32_t output, uint32_t note, uint32_t channel);
void Mapper_programmChangeHandler(uint32_t channel, uint32_t programm);
void Mapper_bendHandler(uint32_t channel);
void Mapper_init();
const char * Mapper_getProgrammName(uint32_t channel);
void Mapper_volumeChangeHandler(uint32_t channel);
void Mapper_noteOnEventHandler(uint32_t output, uint32_t note, uint32_t velocity, uint32_t channel, uint32_t programm);
void Mapper_resetWritePointers();
void Mapper_writeHeader(MAPTABLE_HEADER_t * data);
void Mapper_writeEntry(MAPTABLE_ENTRY_t * data, uint32_t index);
void MAPPER_convertFromLegayMapHeader(MAPTABLE_HEADER_t * dst, LegayMapHeader_t * src);
void MAPPER_convertToLegayMapHeader(LegayMapHeader_t * dst, MAPTABLE_HEADER_t * src);
void MAPPER_convertFromLegayMapEntry(MAPTABLE_ENTRY_t * dst, LegayMapEntry_t * src);
void MAPPER_convertToLegayMapEntry(LegayMapEntry_t * dst, MAPTABLE_ENTRY_t * src);

#endif