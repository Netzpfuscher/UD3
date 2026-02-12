/**
 * @file VMS.h
 * @brief Voice Modulation System (VMS) - Advanced audio synthesis engine
 *
 * VMS provides a block-based modulation system for controlling voice parameters
 * (frequency, volume, on-time, noise) over time. Supports exponential, linear,
 * sinusoidal, and jump modulation types with conditional branching based on
 * parameter thresholds. Used for complex envelopes, effects, and note behaviors.
 */

#ifndef VMS_INC
#define VMS_INC

#include <stdint.h>

typedef struct _SINE_DATA_ SINE_DATA;
typedef struct _VMS_listDataObject_ VMS_listDataObject;
typedef struct __attribute__((packed)) _VMS_BLOCK_ VMS_Block_t;
typedef struct _VMS_VoiceData_t_ VMS_VoiceData_t;

/**
 * @brief Tone generation types
 */
typedef enum {
	TONE_NORMAL,      //!< Normal tone generation
	TONE_NOISE,       //!< Noise generation
	TONE_SINGE_SHOT   //!< Single-shot tone (non-repeating)
} ToneType;

/**
 * @brief VMS modulation types
 */
typedef enum {
	VMS_INVALID,  //!< Invalid/uninitialized modulation type
	VMS_EXP,      //!< Exponential modulation (attack)
	VMS_EXP_INV,  //!< Inverted exponential modulation (decay/release)
	VMS_LIN,      //!< Linear modulation
	VMS_SIN,      //!< Sinusoidal modulation (LFO)
	VMS_JUMP      //!< Immediate jump to target value
} VMS_MODTYPE;

/**
 * @brief Known parameter values for VMS blocks
 *
 * Defines all accessible voice parameters and control values that can be
 * modulated or used as threshold sources.
 */
typedef enum {
	maxOnTime, minOnTime, onTime, otCurrent, otTarget, otFactor,
	frequency, freqCurrent, freqTarget, freqFactor,
	noise, pTime,
	circ1, circ2, circ3, circ4, //!< Circular/scratch parameters
	CC_102, CC_103, CC_104, CC_105, CC_106, CC_107, CC_108, CC_109, //!< MIDI CC values
	CC_110, CC_111, CC_112, CC_113, CC_114, CC_115, CC_116, CC_117, CC_118, CC_119,
	HyperVoice_Count, HyperVoice_Phase, HyperVoice_Volume,
	volume, volumeCurrent, volumeTarget, volumeFactor,
	KNOWNVAL_MAX,
	ANNOYINGPAD = 0xffff //!< Padding for enum size
} KNOWN_VALUE;

/**
 * @brief Threshold crossing directions for conditional branching
 */
typedef enum {
	RISING,  //!< Trigger when value crosses threshold while rising
	FALLING, //!< Trigger when value crosses threshold while falling
	ANY,     //!< Trigger on any threshold crossing
	NONE     //!< No threshold (always execute)
} DIRECTION;

/**
 * @brief Note-off behavior modes
 */
typedef enum {
	INVERTED = 0, //!< Inverted note-off (legacy)
	NORMAL = 1    //!< Normal note-off behavior
} NOTEOFF_BEHAVIOR;

#define VMS_MAX_BRANCHES 4            //!< Maximum conditional branches per block
#define VMS_CIRC_PARAM_COUNT 4        //!< Number of circular/scratch parameters

/** @name VMS Block Flags
 * Flags controlling block behavior and parameter sources
 * @{
 */
#define VMS_FLAG_BLOCK_INVALID 0xffffffff       //!< Block is invalid/uninitialized
#define VMS_FLAG_ISVARIABLE_PARAM1 0x00000001   //!< param1 is a KNOWN_VALUE reference
#define VMS_FLAG_ISVARIABLE_PARAM2 0x00000002   //!< param2 is a KNOWN_VALUE reference
#define VMS_FLAG_ISVARIABLE_PARAM3 0x00000004   //!< param3 is a KNOWN_VALUE reference
#define VMS_FLAG_ISVARIABLE_TARGETFACTOR 0x00000008 //!< targetFactor is a KNOWN_VALUE reference
#define VMS_FLAG_ISBLOCKPERSISTENT 0x80000000   //!< Block is sustain block (persistent)
#define VMS_DIE 0xdeadbeef                      //!< Magic value to kill voice
/** @} */

/** @name VMS Block IDs
 * Special block ID values
 * @{
 */
#define VMS_BLOCKID_INVALID 0xffff        //!< Invalid block ID (16-bit)
#define VMS_BLOCKID32_INVALID 0xffffffff  //!< Invalid block ID (32-bit)
#define VMS_BLOCKID_DEFATTACK 0xfff0      //!< Default attack envelope block
#define VMS_BLOCKID_DEFSUSTAIN 0xfff1     //!< Default sustain block
#define VMS_BLOCKID_DEFRELEASE 0xfff2     //!< Default release envelope block
/** @} */

#define VMS_BLOCKSET_NEXTBLOCKS 1 //!< Next block set selector
#define VMS_BLOCKSET_OFFBLOCK 0   //!< Note-off block selector

#define VMS_VERSION 2             //!< VMS block format version
#define VMS_UNITY_FACTOR 1000000  //!< Unity value for fixed-point factors

/**
 * @brief Sine wave modulation state
 */
struct _SINE_DATA_ {
	int32_t currCount; //!< Current phase counter
};

/**
 * @brief VMS voice data structure
 *
 * Contains all runtime state for a single voice including target/current values,
 * modulation factors, portamento, hypervoice, and source tracking.
 */
struct _VMS_VoiceData_t_ {
	uint32_t freqTarget;   //!< Target frequency (dHz)
	uint32_t freqCurrent;  //!< Current frequency (dHz)
	int32_t freqFactor;    //!< Frequency modulation factor
	uint32_t freqLast;     //!< Last frequency (for portamento)

	uint32_t onTimeTarget;  //!< Target on-time
	uint32_t onTimeCurrent; //!< Current on-time
	int32_t onTimeFactor;   //!< On-time modulation factor

	uint32_t volumeTarget;  //!< Target volume
	uint32_t volumeCurrent; //!< Current volume
	int32_t volumeFactor;   //!< Volume modulation factor

	uint32_t noiseTarget;  //!< Target noise level
	uint32_t noiseCurrent; //!< Current noise level
	int32_t noiseFactor;   //!< Noise modulation factor

	uint32_t hypervoiceCount;         //!< Hypervoice instance count
	uint32_t hypervoiceVolumeFactor;  //!< Hypervoice volume scaling
	uint32_t hypervoiceVolumeCurrent; //!< Current hypervoice volume
	uint32_t hypervoicePhase;         //!< Hypervoice phase offset

	uint32_t portamentoTarget; //!< Portamento target frequency
	int32_t portamentoCurrent; //!< Current portamento value

	uint32_t on; //!< Voice is active flag

	uint32_t flags;        //!< Voice flags
	uint32_t baseVolume;   //!< Base volume (from mapper)
	uint32_t baseVelocity; //!< MIDI velocity

	uint32_t startTime;    //!< Voice start time (ms)
	uint8_t sourceChannel; //!< MIDI source channel
	uint8_t sourceNote;    //!< MIDI source note number

	int32_t circ[VMS_CIRC_PARAM_COUNT]; //!< Circular/scratch parameters

	uint32_t referencingBlocksCount; //!< Number of active blocks referencing this voice

	uint32_t voiceIndex; //!< Voice index in signal generator
};

/**
 * @brief VMS block list data object
 *
 * Runtime instance of a VMS block associated with a specific voice.
 */
struct _VMS_listDataObject_ {
	uint32_t targetOutputIndex; //!< Output index
	uint32_t targetVoiceIndex;  //!< Voice index
	const VMS_Block_t *block;   //!< Pointer to block definition
	void *data;                 //!< Block-specific runtime data

	DIRECTION thresholdDirection; //!< Threshold crossing direction
	uint32_t nextRunTime;         //!< Next execution time (ms)
	uint32_t periodMs;            //!< Execution period (ms)
};

/**
 * @brief Range parameters for threshold checking
 *
 * Packed structure defining a value source and threshold range.
 */
typedef struct {
	KNOWN_VALUE sourceId : 8;  //!< Parameter to check
	int32_t rangeStart : 12;   //!< Threshold lower bound
	int32_t rangeEnd : 12;     //!< Threshold upper bound
} __attribute__((packed)) RangeParameters;

/**
 * @brief VMS block definition
 *
 * Defines a modulation operation with conditional branching. Blocks are chained
 * together to create complex envelopes and effects.
 */
struct _VMS_BLOCK_ {
	uint16_t nextBlocks[VMS_MAX_BRANCHES]; //!< Next block IDs (conditional branches)
	uint16_t offBlock;                     //!< Block to execute on note-off

	// Modulation parameters
	NOTEOFF_BEHAVIOR behavior : 8; //!< Note-off behavior mode
	VMS_MODTYPE type : 8;          //!< Modulation type
	KNOWN_VALUE target : 16;       //!< Target parameter to modulate

	int32_t targetFactor; //!< Target value or modulation factor
	int32_t param1;       //!< Type-specific parameter 1
	int32_t param2;       //!< Type-specific parameter 2
	int32_t param3;       //!< Type-specific parameter 3 (threshold/branch)

	uint32_t periodMs; //!< Execution period (ms)
	uint32_t flags;    //!< VMS_FLAG_* flags
} __attribute__((packed));

/**
 * @brief Legacy VMS block format (backward compatibility)
 *
 * Used for communication with old configuration software.
 */
typedef struct {
	uint32_t uid;                           //!< Unique block ID
	uint32_t nextBlocks[VMS_MAX_BRANCHES];  //!< Next block IDs
	uint32_t offBlock;                      //!< Note-off block ID
	NOTEOFF_BEHAVIOR behavior;              //!< Note-off behavior
	VMS_MODTYPE type;                       //!< Modulation type
	KNOWN_VALUE target;                     //!< Target parameter
	DIRECTION thresholdDirection;           //!< Threshold direction
	int32_t targetFactor;                   //!< Target factor
	int32_t param1;                         //!< Parameter 1
	int32_t param2;                         //!< Parameter 2
	int32_t param3;                         //!< Parameter 3
	uint32_t period;                        //!< Period (ms)
	uint32_t flags;                         //!< Flags
} __attribute__((packed)) VMS_LEGAYBLOCK_t;

/**
 * @brief Remove all blocks targeting a specific voice
 *
 * Cleans up VMS block list by removing all blocks associated with the given voice.
 * Used when a voice is deallocated.
 *
 * @param targetVoice Voice index to remove blocks for
 */
void VMS_removeBlocksWithTargetVoice(uint32_t targetVoice);

/**
 * @brief Check if block pointer is valid
 *
 * Validates that a block pointer is non-null and not marked as invalid.
 *
 * @param block Pointer to VMS block
 * @return Non-zero if valid, 0 if invalid
 */
uint32_t VMS_isBlockValid(const VMS_Block_t *block);

/**
 * @brief Kill all active VMS blocks
 *
 * Clears the entire VMS block list, stopping all modulation.
 */
void VMS_killBlocks();

/**
 * @brief Add block to execution list
 *
 * Schedules a VMS block for execution on the specified voice and output.
 * Creates a list data object and adds it to the active block list.
 *
 * @param block Pointer to VMS block definition
 * @param output Output index
 * @param voiceId Voice index
 */
void VMS_addBlockToList(const VMS_Block_t *block, uint32_t output, uint32_t voiceId);

/**
 * @brief Run VMS scheduler
 *
 * Executes all scheduled VMS blocks that are due to run. Should be called
 * periodically (typically at MIDI_ISR_Hz rate) to update voice modulation.
 */
void VMS_run();

/**
 * @brief Acquire VMS semaphore
 *
 * Takes the VMS semaphore to prevent concurrent access to block list.
 * Must be paired with VMS_returnSemaphore().
 */
void VMS_getSemaphore();

/**
 * @brief Release VMS semaphore
 *
 * Returns the VMS semaphore, allowing other tasks to access block list.
 */
void VMS_returnSemaphore();

/**
 * @brief Initialize VMS system
 *
 * Creates semaphore and initializes block list. Must be called once at startup.
 */
void VMS_init();

/**
 * @brief Convert current block to legacy format
 *
 * @param dst Destination pointer (legacy format)
 * @param src Source pointer (current format)
 */
void VMS_convertToLegacyBlock(VMS_LEGAYBLOCK_t *dst, VMS_Block_t *src);

/**
 * @brief Convert legacy block to current format
 *
 * @param dst Destination pointer (current format)
 * @param src Source pointer (legacy format)
 */
void VMS_convertFromLegacyBlock(VMS_Block_t *dst, VMS_LEGAYBLOCK_t *src);

/**
 * @brief Convert legacy block with 63-byte size to current format
 *
 * Handles old block format with different size.
 *
 * @param dst Destination pointer (current format)
 * @param src Source pointer (legacy 63-byte format)
 */
void VMS_convertFromLegacyBlockWith63Bytes(VMS_Block_t *dst, VMS_LEGAYBLOCK_t *src);

#endif