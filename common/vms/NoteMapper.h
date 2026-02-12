/**
 * @file NoteMapper.h
 * @brief MIDI note to frequency mapping with multi-program support
 *
 * Maps MIDI note events to output frequencies and targets using configurable
 * mapping tables. Supports multiple programs/patches, per-note configuration,
 * and various audio effects (pitch bend, volume, portamento, damper, stereo).
 * Mapping tables are stored in NVM and can be configured at runtime.
 */

#ifndef NOTEMAPPER_INC
#define NOTEMAPPER_INC

#include <stdint.h>

/** @name Mapper Flags
 * Per-note effect enable flags for map table entries
 * @{
 */
#define MAP_ENA_PITCHBEND  0x80 //!< Enable pitch bend effect
#define MAP_ENA_STEREO     0x40 //!< Enable stereo positioning
#define MAP_ENA_VOLUME     0x20 //!< Enable volume control
#define MAP_ENA_DAMPER     0x10 //!< Enable damper pedal effect
#define MAP_ENA_PORTAMENTO 0x08 //!< Enable portamento (glide between notes)
#define MAP_ENA_EQ         0x04 //!< Enable equalization
#define MAP_FREQ_MODE      0x01 //!< Frequency mode flag (offset vs fixed)
/** @} */

#define MAPPER_ALL_PROGRAMMS 0xff      //!< Wildcard program number (matches all)
#define MAPPER_VOLUME_MAX 0xff         //!< Maximum mapper volume value

#define FREQ_MODE_OFFSET 1             //!< Frequency mode: offset from MIDI note
#define FREQ_MODE_FIXED 0              //!< Frequency mode: fixed frequency

#define MAPPER_MAPMEM_SIZE 2048        //!< Size of NVM map memory in bytes
#define MAPPER_MAP_NAME_LENGTH 18      //!< Maximum length of map name string

#define MAPPER_VERSION 2               //!< Mapper table format version

/**
 * @brief Get map table entry from header by index
 *
 * Calculates pointer to entry within a map table.
 */
#define MAPPER_ENTRY_FROM_HEADER(HEADER_PTR, IDX) (MAPTABLE_ENTRY_t *)((char *)HEADER_PTR + sizeof(MAPTABLE_HEADER_t) + IDX * sizeof(MAPTABLE_ENTRY_t))

typedef struct _MapData_ MAPTABLE_DATA_t;
typedef struct _MapEntry_ MAPTABLE_ENTRY_t;
typedef struct _MapHeader_ MAPTABLE_HEADER_t;

/**
 * @brief Map table entry data
 *
 * Defines the output configuration for a note range.
 */
struct _MapData_ {
	int16_t noteFreq;      //!< Frequency offset (offset mode) or fixed frequency (fixed mode)
	uint8_t targetOT;      //!< Target output and volume scaling
	uint8_t flags;         //!< Effect enable flags (MAP_ENA_*)
	uint16_t startblockID; //!< VMS block ID for envelope/waveform
} __attribute__((packed));

/**
 * @brief Map table entry
 *
 * Defines a note range and its associated output configuration.
 */
struct _MapEntry_ {
	uint8_t startNote;    //!< First MIDI note in range (0-127)
	uint8_t endNote;      //!< Last MIDI note in range (0-127)
	MAPTABLE_DATA_t data; //!< Output configuration for this range
} __attribute__((packed));

/**
 * @brief Map table header
 *
 * Header for a mapping table, which defines note-to-output mappings for
 * a program number range.
 */
struct _MapHeader_ {
	uint8_t listEntries;       //!< Number of entries in this map table
	uint8_t programNumberStart; //!< First program number this map applies to
	uint8_t programNumberEnd;   //!< Last program number this map applies to
	char name[MAPPER_MAP_NAME_LENGTH]; //!< Human-readable map name
} __attribute__((packed));

/**
 * @name Legacy Data Type Definitions
 * Compatibility structures for old configuration software
 * @{
 */

/**
 * @brief Legacy map header (backward compatibility)
 */
typedef struct {
	uint8_t listEntries;       //!< Number of entries
	uint8_t programNumberStart; //!< First program number
	char name[18];             //!< Map name
} __attribute__((packed)) LegayMapHeader_t;

/**
 * @brief Legacy map entry data (backward compatibility)
 */
typedef struct {
	int16_t noteFreq;      //!< Frequency offset or fixed frequency
	uint8_t targetOT;      //!< Target output and volume
	uint8_t flags;         //!< Effect flags
	uint32_t startblockID; //!< Block ID (was pointer in old version)
} __attribute__((packed)) LegayMapData_t;

/**
 * @brief Legacy map entry (backward compatibility)
 */
typedef struct {
	uint8_t startNote;    //!< First note in range
	uint8_t endNote;      //!< Last note in range
	LegayMapData_t data;  //!< Entry data
} __attribute__((packed)) LegayMapEntry_t;
/** @} */

/**
 * @brief Handle note off event
 *
 * Processes MIDI note off event by finding and stopping the corresponding voice.
 *
 * @param output Target output index
 * @param note MIDI note number (0-127)
 * @param channel MIDI channel number (0-15)
 */
void Mapper_noteOffEventHandler(uint32_t output, uint32_t note, uint32_t channel);

/**
 * @brief Handle program change event
 *
 * Loads a new mapping table for the specified channel based on program number.
 * Searches NVM for matching program and assigns to channel.
 *
 * @param channel MIDI channel number (0-15)
 * @param programm MIDI program number (0-127)
 */
void Mapper_programmChangeHandler(uint32_t channel, uint32_t programm);

/**
 * @brief Handle pitch bend change
 *
 * Updates frequencies of all active notes on the channel to reflect
 * the new pitch bend value.
 *
 * @param channel MIDI channel number (0-15)
 */
void Mapper_bendHandler(uint32_t channel);

/**
 * @brief Initialize note mapper
 *
 * Clears all channel map assignments. Must be called once at startup.
 */
void Mapper_init();

/**
 * @brief Get current program name for channel
 *
 * Returns the name of the currently loaded mapping table for the channel.
 *
 * @param channel MIDI channel number (0-15)
 * @return Pointer to null-terminated program name string
 */
const char *Mapper_getProgrammName(uint32_t channel);

/**
 * @brief Handle volume change event
 *
 * Updates output volumes for all active notes on the channel.
 *
 * @param channel MIDI channel number (0-15)
 */
void Mapper_volumeChangeHandler(uint32_t channel);

/**
 * @brief Handle note on event
 *
 * Processes MIDI note on event by finding appropriate map entry, allocating
 * a voice, and starting audio output with configured parameters.
 *
 * @param output Target output index
 * @param note MIDI note number (0-127)
 * @param velocity Note velocity (0-127, 0 treated as note off)
 * @param channel MIDI channel number (0-15)
 * @param programm Program number for this note
 */
void Mapper_noteOnEventHandler(uint32_t output, uint32_t note, uint32_t velocity, uint32_t channel, uint32_t programm);

/**
 * @brief Reset NVM write pointers
 *
 * Resets internal pointers for writing new map tables to NVM.
 */
void Mapper_resetWritePointers();

/**
 * @brief Write map table header to NVM
 *
 * Writes a map table header to the next available position in NVM.
 *
 * @param data Pointer to header data to write
 */
void Mapper_writeHeader(MAPTABLE_HEADER_t *data);

/**
 * @brief Write map table entry to NVM
 *
 * Writes a map table entry at the specified index relative to current header.
 *
 * @param data Pointer to entry data to write
 * @param index Entry index within current map table
 */
void Mapper_writeEntry(MAPTABLE_ENTRY_t *data, uint32_t index);

/**
 * @brief Convert legacy map header to current format
 *
 * @param dst Destination pointer (current format)
 * @param src Source pointer (legacy format)
 */
void MAPPER_convertFromLegayMapHeader(MAPTABLE_HEADER_t *dst, LegayMapHeader_t *src);

/**
 * @brief Convert current map header to legacy format
 *
 * @param dst Destination pointer (legacy format)
 * @param src Source pointer (current format)
 */
void MAPPER_convertToLegayMapHeader(LegayMapHeader_t *dst, MAPTABLE_HEADER_t *src);

/**
 * @brief Convert legacy map entry to current format
 *
 * @param dst Destination pointer (current format)
 * @param src Source pointer (legacy format)
 */
void MAPPER_convertFromLegayMapEntry(MAPTABLE_ENTRY_t *dst, LegayMapEntry_t *src);

/**
 * @brief Convert current map entry to legacy format
 *
 * @param dst Destination pointer (legacy format)
 * @param src Source pointer (current format)
 */
void MAPPER_convertToLegayMapEntry(LegayMapEntry_t *dst, MAPTABLE_ENTRY_t *src);

#endif
