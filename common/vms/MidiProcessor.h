/**
 * @file MidiProcessor.h
 * @brief MIDI command processing and channel management
 *
 * Processes MIDI commands, manages per-channel state (volume, pitch bend, etc.),
 * and routes events to MIDI filters. Supports standard MIDI commands including
 * note on/off, controller changes, pitch bend, and program changes across 16 channels.
 */

#ifndef MIDI_PROC_INC
#define MIDI_PROC_INC

#include <stdint.h>

/** @name MIDI Command Codes
 * Standard MIDI command byte values
 * @{
 */
#define MIDI_CMD_NOTE_OFF            0x80 //!< Note off command
#define MIDI_CLK                     0xF8 //!< MIDI clock tick
#define MIDI_CMD_NOTE_ON             0x90 //!< Note on command
#define MIDI_CMD_KEY_PRESSURE        0xA0 //!< Polyphonic key pressure (aftertouch)
#define MIDI_CMD_CONTROLLER_CHANGE   0xB0 //!< Controller change
#define MIDI_CMD_PROGRAM_CHANGE      0xC0 //!< Program change
#define MIDI_CMD_CHANNEL_PRESSURE    0xD0 //!< Channel pressure (aftertouch)
#define MIDI_CMD_PITCH_BEND          0xE0 //!< Pitch bend change
#define MIDI_CMD_SYSEX               0xF0 //!< System exclusive message
/** @} */

/** @name MIDI Control Change (CC) Numbers
 * Standard MIDI continuous controller numbers
 * @{
 */
#define MIDI_CC_ALL_SOUND_OFF          0x78 //!< Mute all sounding notes
#define MIDI_CC_RESET_ALL_CONTROLLERS  0x79 //!< Reset all controllers to default
#define MIDI_CC_ALL_NOTES_OFF          0x7B //!< Stop all notes
#define MIDI_CC_VOLUME                 0x07 //!< Channel volume
#define MIDI_CC_PORTAMENTO_TIME        0x05 //!< Portamento time
#define MIDI_CC_PAN                    0x0A //!< Pan position
#define MIDI_CC_RPN_LSB                0x64 //!< Registered Parameter Number LSB
#define MIDI_CC_RPN_MSB                0x65 //!< Registered Parameter Number MSB
#define MIDI_CC_NRPN_LSB               0x62 //!< Non-Registered Parameter Number LSB
#define MIDI_CC_SUSTAIN_PEDAL          0x40 //!< Sustain pedal (damper)
#define MIDI_CC_DAMPER_PEDAL           0x43 //!< Soft pedal
#define MIDI_CC_NRPN_MSB               0x63 //!< Non-Registered Parameter Number MSB
#define MIDI_CC_DATA_FINE              0x26 //!< Data entry LSB
#define MIDI_CC_DATA_COARSE            0x06 //!< Data entry MSB
/** @} */

#define MIDI_RPN_BENDRANGE 0x0000 //!< RPN number for pitch bend range

#define MIDI_CHANNELCOUNT 16      //!< Number of MIDI channels
#define MIDI_VOLUME_MAX 0x7f      //!< Maximum MIDI volume value (127)
#define MIDIPROC_VERSION 1        //!< MIDI processor version

/** @name Frequency Scaler Macros
 * Apply pitch bend to frequency values
 * @{
 */
//!< Apply pitch bend to frequency
#define MIDIPROC_SCALE_PITCHBEND(FREQ, CHANNEL) ((FREQ * MidiProcessor_getBendFactor(CHANNEL)) >> 13)
/** @} */

/** @name Volume Scaler Macros
 * Apply channel volume and stereo effects to volume values
 * @{
 */
//!< Scale volume by channel volume
#define MIDIPROC_SCALE_VOLUME(VOLUME, CHANNEL) (((VOLUME * MidiProcessor_getVolume(CHANNEL)) / 0x7f)
//!< Scale volume by channel volume with stereo positioning
#define MIDIPROC_SCALE_VOLUME_STEREO(VOLUME, CHANNEL) (((VOLUME * MidiProcessor_getStereoVolume(CHANNEL)) / 0x7f)
/** @} */

/**
 * @brief MIDI channel state descriptor
 *
 * Contains all per-channel state including controller values, pitch bend settings,
 * volume calculations, and program selection.
 */
typedef struct {
	uint8_t parameters[128]; //!< All MIDI CC parameter values for this channel

	// Pitch bend parameters
	uint32_t bendFactor;   //!< Current pitch bend factor (fixed-point, 1.0 = 1<<13)
	uint16_t bendRangeRaw; //!< Raw pitch bend range from RPN
	float bendRange;       //!< Pitch bend range in semitones (cached for speed)

	uint32_t lastFrequency; //!< Last frequency played on this channel (dHz)

	// Volume effect variables
	uint32_t stereoVolume; //!< Stereo-adjusted volume (cached for speed)

	uint8_t programm; //!< Current program/patch number
} MidiChannelDescriptor_t;

/**
 * @brief Initialize MIDI processor
 *
 * Allocates channel descriptors and initializes MIDI filters for all outputs.
 * Must be called once at system startup before processing MIDI events.
 */
void MidiProcessor_init();

/**
 * @brief Reset all MIDI state
 *
 * Stops all audio, resets all channel parameters to defaults, and triggers
 * panic handler. Called when MIDI stop/panic commands are received.
 */
void MidiProcessor_resetMidi();

/**
 * @brief Configure stereo positioning
 *
 * Sets stereo panning parameters for multi-channel audio output.
 *
 * @param stereoPosition Center position for stereo field
 * @param stereoWidth Width of stereo field
 * @param stereoSlope Slope of stereo panning curve
 */
void MidiProc_setStereoConfig(uint32_t stereoPosition, uint32_t stereoWidth, uint32_t stereoSlope);

/**
 * @brief Convert MIDI note number to frequency
 *
 * Uses lookup table to convert MIDI note number (0-127) to frequency in decihertz.
 *
 * @param note MIDI note number (0-127, where 60 = middle C)
 * @return Frequency in decihertz (dHz, 1/10 Hz)
 */
uint32_t MidiProcessor_noteToFrequency_dHz(uint32_t note);

/**
 * @brief Process incoming MIDI command
 *
 * Main MIDI command dispatcher. Handles note on/off, controller changes,
 * pitch bend, and program changes. Routes events to appropriate filters
 * and updates channel state.
 *
 * @param cable MIDI cable/port number
 * @param channel MIDI channel number (0-15)
 * @param cmd MIDI command byte (e.g., MIDI_CMD_NOTE_ON)
 * @param param1 First data byte (e.g., note number)
 * @param param2 Second data byte (e.g., velocity)
 */
void MidiProcessor_processCmd(uint32_t cable, uint32_t channel, uint32_t cmd, uint32_t param1, uint32_t param2);

/**
 * @brief Get stereo-adjusted volume for channel
 *
 * Returns the channel volume scaled by stereo pan position.
 *
 * @param channel MIDI channel number (0-15)
 * @return Stereo-adjusted volume (0-127)
 */
uint32_t MidiProcessor_getStereoVolume(uint32_t channel);

/**
 * @brief Get controller value for channel
 *
 * Returns the current value of a MIDI continuous controller.
 *
 * @param channel MIDI channel number (0-15)
 * @param id Controller number (0-127)
 * @return Controller value (0-127)
 */
uint32_t MidiProcessor_getCCValue(uint32_t channel, uint32_t id);

/**
 * @brief Get channel volume
 *
 * Returns the current volume setting (CC 0x07) for the channel.
 *
 * @param channel MIDI channel number (0-15)
 * @return Volume value (0-127)
 */
uint32_t MidiProcessor_getVolume(uint32_t channel);

/**
 * @brief Get damper pedal volume effect
 *
 * Returns reduced volume (0xd3) if damper pedal is pressed, otherwise full volume.
 *
 * @param channel MIDI channel number (0-15)
 * @return Volume scaling factor (0xd3 or 0x7f)
 */
uint32_t MidiProcessor_getDamperVolume(uint32_t channel);

/**
 * @brief Get pitch bend factor for channel
 *
 * Returns the current pitch bend factor in fixed-point format (1.0 = 1<<13).
 * Use with MIDIPROC_SCALE_PITCHBEND macro to apply to frequencies.
 *
 * @param channel MIDI channel number (0-15)
 * @return Pitch bend factor (fixed-point, neutral = 8192)
 */
uint32_t MidiProcessor_getBendFactor(uint32_t channel);

/**
 * @brief Set last played frequency for channel
 *
 * Stores the most recent frequency played on this channel, used for
 * pitch bend calculations and portamento effects.
 *
 * @param channel MIDI channel number (0-15)
 * @param frequency Frequency in decihertz (dHz)
 */
void MidiProcessor_setLastFrequency(uint32_t channel, uint32_t frequency);

/**
 * @brief Get last played frequency for channel
 *
 * Returns the most recent frequency played on this channel.
 *
 * @param channel MIDI channel number (0-15)
 * @return Frequency in decihertz (dHz)
 */
uint32_t MidiProcessor_getLastFrequency(uint32_t channel);

#endif