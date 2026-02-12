/**
 * @file min.h
 * @brief MIN Protocol v2.0 - Microcontroller Interconnect Network
 *
 * MIN is a lightweight reliable protocol for exchanging information from a microcontroller (MCU) to a host.
 * It is designed to run on an 8-bit MCU but also scale up to more powerful devices. A typical use case is to
 * send data from a UART on a small MCU over a UART-USB converter plugged into a PC host. A Python implementation
 * of host code is provided (or this code could be compiled for a PC).
 *
 * MIN supports frames of 0-255 bytes (with a lower limit selectable at compile time to reduce RAM). MIN frames
 * have identifier values between 0 and 63.
 *
 * An optional transport layer T-MIN can be compiled in. This provides sliding window reliable transmission of frames.
 *
 * ## Compile Options
 *
 * - Define NO_TRANSPORT_PROTOCOL to remove the code and other overheads of dealing with transport frames. Any
 *   transport frames sent from the other side are dropped.
 *
 * - Define MAX_PAYLOAD if the size of the frames is to be limited. This is particularly useful with the transport
 *   protocol where a deep FIFO is wanted but not for large frames.
 *
 * ## API Functions
 *
 * - min_init_context(): Initialize a MIN context for a serial port
 * - min_send_frame(): Send a non-transport frame (unreliable)
 * - min_queue_frame(): Queue a transport frame (reliable with retransmission)
 * - min_poll(): Pass in received bytes and drive protocol state machine
 *
 * ## Callbacks (must be provided by programmer)
 *
 * - min_tx_space(): Return available space in transmit buffer
 * - min_tx_byte(): Send a byte on the given port
 * - min_rx_space(): Return available space in receive buffer
 * - min_application_handler(): Handle received MIN frame
 * - min_time_ms(): Return current time in milliseconds (for transport protocol)
 * - min_tx_start()/min_tx_finished(): Indicate frame transmission start/end
 * - min_reset(): Called when MIN protocol reset occurs
 */

#ifndef MIN_H
#define MIN_H

#include <stdint.h>
#include <stdbool.h>

#ifdef ASSERTION_CHECKING
#include <assert.h>
#endif

#ifndef NO_TRANSPORT_PROTOCOL
#define TRANSPORT_PROTOCOL
#endif

#ifndef MAX_PAYLOAD
#define MAX_PAYLOAD (255U) //!< Maximum payload size in bytes
#endif

/** @name Transport FIFO Configuration
 * Powers of two for FIFO management. Default is 16 frames in the FIFO, total of 1024 bytes for frame data.
 * @{
 */
#ifndef TRANSPORT_FIFO_SIZE_FRAMES_BITS
#define TRANSPORT_FIFO_SIZE_FRAMES_BITS (4U) //!< FIFO size (2^4 = 16 frames)
#endif
#ifndef TRANSPORT_FIFO_SIZE_FRAME_DATA_BITS
#define TRANSPORT_FIFO_SIZE_FRAME_DATA_BITS (10U) //!< Frame data buffer size (2^10 = 1024 bytes)
#endif
/** @} */

#define TRANSPORT_FIFO_MAX_FRAMES (1U << TRANSPORT_FIFO_SIZE_FRAMES_BITS)       //!< Maximum frames in FIFO
#define TRANSPORT_FIFO_MAX_FRAME_DATA (1U << TRANSPORT_FIFO_SIZE_FRAME_DATA_BITS) //!< Maximum frame data bytes

#if (MAX_PAYLOAD > 255)
#error "MIN frame payloads can be no bigger than 255 bytes"
#endif

#if (TRANSPORT_FIFO_MAX_FRAMES > 256)
#error "Transport FIFO frames cannot exceed 256"
#endif

#if (TRANSPORT_FIFO_MAX_FRAME_DATA > 65536)
#error "Transport FIFO data allocated cannot exceed 64Kbytes"
#endif

/**
 * @brief CRC32 calculation context
 */
struct crc32_context {
	uint32_t crc; //!< Current CRC value
};

#ifdef TRANSPORT_PROTOCOL

/**
 * @brief Transport layer frame structure
 *
 * Stores metadata for a queued transport frame awaiting acknowledgment.
 */
struct transport_frame {
	uint32_t last_sent_time_ms; //!< When frame was last sent (for re-send timeouts)
	uint16_t payload_offset;    //!< Where in the ring buffer the payload is
	uint8_t payload_len;        //!< Size of payload
	uint8_t min_id;             //!< ID of frame
	uint32_t seq;               //!< Sequence number of frame
};

/**
 * @brief Transport FIFO structure
 *
 * Manages queued transport frames with sliding window protocol.
 */
struct transport_fifo {
	struct transport_frame frames[TRANSPORT_FIFO_MAX_FRAMES]; //!< Array of queued frames
	uint32_t last_sent_ack_time_ms;                           //!< Time of last ACK sent
	uint32_t last_received_anything_ms;                       //!< Time of last received byte
	uint32_t last_received_frame_ms;                          //!< Time of last valid frame
	uint32_t dropped_frames;                                  //!< Counter: frames dropped
	uint32_t spurious_acks;                                   //!< Counter: spurious ACKs
	uint32_t sequence_mismatch_drop;                          //!< Counter: sequence mismatches
	uint32_t resets_received;                                 //!< Counter: resets received
	uint32_t crc_fails;                                       //!< Counter: CRC failures
	uint16_t n_ring_buffer_bytes;                             //!< Bytes used in payload ring buffer
	uint16_t n_ring_buffer_bytes_max;                         //!< Peak bytes used in ring buffer
	uint16_t ring_buffer_tail_offset;                         //!< Tail of payload ring buffer
	uint8_t n_frames;                                         //!< Number of frames in FIFO
	uint8_t n_frames_max;                                     //!< Peak frames in FIFO
	uint8_t head_idx;                                         //!< Head index (frames taken from here)
	uint8_t tail_idx;                                         //!< Tail index (new frames added here)
	uint32_t sn_min;                                          //!< Minimum sequence number in window
	uint32_t sn_max;                                          //!< Maximum sequence number in window
	uint32_t rn;                                              //!< Next expected receive sequence number
};
#endif

/**
 * @brief MIN protocol context
 *
 * Contains all state for one MIN serial port instance.
 */
struct min_context {
#ifdef TRANSPORT_PROTOCOL
	struct transport_fifo transport_fifo;    //!< T-MIN queue of outgoing frames
#endif
	uint8_t rx_frame_payload_buf[MAX_PAYLOAD]; //!< Payload received so far
	uint32_t rx_frame_checksum;                //!< Checksum received over the wire
	struct crc32_context rx_checksum;          //!< Calculated checksum for receiving frame
	struct crc32_context tx_checksum;          //!< Calculated checksum for sending frame
	uint8_t rx_header_bytes_seen;              //!< Countdown of header bytes to reset state
	uint8_t rx_frame_state;                    //!< State of receiver state machine
	uint8_t rx_frame_payload_bytes;            //!< Length of payload received so far
	uint8_t rx_frame_id_control;               //!< ID and control bit of frame being received
	uint32_t rx_frame_seq;                     //!< Sequence number of frame being received
	uint8_t rx_frame_length;                   //!< Length of frame
	uint8_t rx_control;                        //!< Control byte
	uint8_t tx_header_byte_countdown;          //!< Count out the header bytes
	uint8_t port;                              //!< Number of the port associated with the context
	uint32_t rx_space;                         //!< Local receive buffer space
	uint32_t remote_rx_space;                  //!< Remote receive buffer space
};

#ifdef TRANSPORT_PROTOCOL
/**
 * @brief Queue a MIN frame in the transport queue
 *
 * Frame will be retransmitted until acknowledged.
 *
 * @param self Pointer to MIN context
 * @param min_id Frame ID (0-63)
 * @param payload Pointer to payload data
 * @param payload_len Length of payload (0-MAX_PAYLOAD)
 * @return true if queued successfully, false if FIFO full
 */
bool min_queue_frame(struct min_context *self, uint8_t min_id, uint8_t *payload, uint8_t payload_len);

/**
 * @brief Determine if MIN has space to queue a transport frame
 *
 * @param self Pointer to MIN context
 * @param payload_len Length of payload to be queued
 * @return true if space available, false otherwise
 */
bool min_queue_has_space_for_frame(struct min_context *self, uint8_t payload_len);
#endif

/**
 * @brief Send a non-transport MIN frame
 *
 * Frame is sent immediately with no retransmission.
 *
 * @param self Pointer to MIN context
 * @param min_id Frame ID (0-63)
 * @param payload Pointer to payload data
 * @param payload_len Length of payload (0-MAX_PAYLOAD)
 */
void min_send_frame(struct min_context *self, uint8_t min_id, uint8_t *payload, uint8_t payload_len);

/**
 * @brief Poll MIN protocol with received bytes
 *
 * Must be regularly called with received bytes. If transport protocol is enabled,
 * must be called even without bytes to drive retransmit state machine.
 *
 * @param self Pointer to MIN context
 * @param buf Buffer of received bytes
 * @param buf_len Number of bytes in buffer
 */
void min_poll(struct min_context *self, uint8_t *buf, uint32_t buf_len);

/**
 * @brief Reset the MIN transport state machine
 *
 * @param self Pointer to MIN context
 * @param inform_other_side If true, send RESET frame to remote side
 */
void min_transport_reset(struct min_context *self, bool inform_other_side);

/**
 * @brief Process a single received byte
 *
 * @param self Pointer to MIN context
 * @param byte Received byte
 */
void min_rx_byte(struct min_context *self, uint8_t byte);

/**
 * @brief CALLBACK: Handle incoming MIN frame
 *
 * Must be implemented by application.
 *
 * @param min_id Frame ID
 * @param min_payload Pointer to payload data
 * @param len_payload Length of payload
 * @param port Port number that received the frame
 */
void min_application_handler(uint8_t min_id, uint8_t *min_payload, uint8_t len_payload, uint8_t port);

#ifdef TRANSPORT_PROTOCOL
/**
 * @brief CALLBACK: Return current time in milliseconds
 *
 * Must be implemented by application. Typically a tick timer interrupt
 * increments a 32-bit variable every 1ms.
 *
 * @return Current time in milliseconds
 */
uint32_t min_time_ms(void);
#endif

/**
 * @brief CALLBACK: Return transmit buffer space
 *
 * Must be implemented by application.
 *
 * @param port Port number
 * @return Number of bytes available in transmit buffer
 */
uint16_t min_tx_space(uint8_t port);

/**
 * @brief CALLBACK: Return receive buffer space for flow control
 *
 * Must be implemented by application.
 *
 * @param port Port number
 * @return Number of bytes available in receive buffer
 */
uint32_t min_rx_space(uint8_t port);

/**
 * @brief CALLBACK: Send a byte on the given port
 *
 * Must be implemented by application.
 *
 * @param port Port number
 * @param byte Byte to send
 */
void min_tx_byte(uint8_t port, uint8_t byte);

/**
 * @brief CALLBACK: Indicate frame transmission start
 *
 * Useful for buffering bytes into a single serial call.
 *
 * @param port Port number
 */
void min_tx_start(uint8_t port);

/**
 * @brief CALLBACK: Indicate frame transmission finished
 *
 * Useful for buffering bytes into a single serial call.
 *
 * @param port Port number
 */
void min_tx_finished(uint8_t port);

/**
 * @brief CALLBACK: Handle remote time synchronization
 *
 * @param remote_time Remote system time in milliseconds
 */
void time_cb(uint32_t remote_time);

/**
 * @brief CALLBACK: Handle MIN reset
 *
 * @param port Port number that was reset
 */
void min_reset(uint8_t port);

/**
 * @brief Initialize a MIN context ready for receiving bytes
 *
 * Can have multiple MIN contexts for multiple serial ports.
 *
 * @param self Pointer to MIN context to initialize
 * @param port Port number associated with this context
 */
void min_init_context(struct min_context *self, uint8_t port);

extern uint8_t min_debug; //!< Debug printing enable flag

#define MIN_DEBUG_PRINTING
#ifdef MIN_DEBUG_PRINTING
/**
 * @brief Debug print function
 *
 * @param msg Format string
 * @param ... Variable arguments
 */
void min_debug_prt(const char *msg, ...);

/**
 * @brief Debug print macro (only prints if min_debug is set)
 */
#define min_debug_print(msg, ...) \
	if (min_debug) min_debug_prt(msg, ##__VA_ARGS__);

#else
#define min_debug_print(...)
#endif

#endif // MIN_H
