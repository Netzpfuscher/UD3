/**
 * @file min.h
 * @brief MIN Protocol v2.0 - Microcontroller Interconnect Network
 *
 * MIN is a lightweight reliable serial protocol for microcontroller-to-host communication.
 * Supports frames of 0-255 bytes with identifiers 0-63. An optional transport layer (T-MIN)
 * provides sliding-window reliable delivery with retransmission.
 *
 * ## Compile Options
 *
 * - Define NO_TRANSPORT_PROTOCOL to remove transport layer overhead
 * - Define MAX_PAYLOAD to limit maximum frame size (reduces RAM usage)
 *
 * ## Callbacks (must be provided by application)
 *
 * - min_tx_space()            Return available TX buffer bytes
 * - min_tx_byte()             Send a byte on the given port
 * - min_rx_space()            Return available RX buffer bytes
 * - min_application_handler() Handle received MIN frame
 * - min_time_ms()             Return current time in milliseconds
 * - min_tx_start/finished()   Frame transmission boundary markers
 * - min_reset()               Called on MIN protocol reset
 * - time_cb()                 Remote time synchronization callback
 *
 * Copyright (c) 2014-2017 JK Energy Ltd.
 * Use authorized under the MIT license.
 */

#ifndef MIN_H
#define MIN_H

#include <stdbool.h>
#include <stdint.h>

#ifndef NO_TRANSPORT_PROTOCOL
#define TRANSPORT_PROTOCOL
#endif

#ifndef MAX_PAYLOAD
#define MAX_PAYLOAD (255U)
#endif

#ifndef TRANSPORT_FIFO_SIZE_FRAMES_BITS
#define TRANSPORT_FIFO_SIZE_FRAMES_BITS (4U)
#endif
#ifndef TRANSPORT_FIFO_SIZE_FRAME_DATA_BITS
#define TRANSPORT_FIFO_SIZE_FRAME_DATA_BITS (10U)
#endif

#define TRANSPORT_FIFO_MAX_FRAMES (1U << TRANSPORT_FIFO_SIZE_FRAMES_BITS)
#define TRANSPORT_FIFO_MAX_FRAME_DATA (1U << TRANSPORT_FIFO_SIZE_FRAME_DATA_BITS)

#if (MAX_PAYLOAD > 255)
#error "MIN frame payloads can be no bigger than 255 bytes"
#endif
#if (TRANSPORT_FIFO_MAX_FRAMES > 256)
#error "Transport FIFO frames cannot exceed 256"
#endif
#if (TRANSPORT_FIFO_MAX_FRAME_DATA > 65536)
#error "Transport FIFO data allocated cannot exceed 64Kbytes"
#endif

/* ── CRC32 context ────────────────────────────────────────────────────── */

struct crc32_context {
	uint32_t crc;
};

/* ── Transport layer structures ───────────────────────────────────────── */

#ifdef TRANSPORT_PROTOCOL

struct transport_frame {
	uint32_t last_sent_time_ms;
	uint16_t payload_offset;
	uint8_t payload_len;
	uint8_t min_id;
	uint32_t seq;
};

struct transport_fifo {
	struct transport_frame frames[TRANSPORT_FIFO_MAX_FRAMES];
	uint8_t payloads[TRANSPORT_FIFO_MAX_FRAME_DATA];
	uint32_t last_sent_ack_time_ms;
	uint32_t last_received_anything_ms;
	uint32_t last_received_frame_ms;
	uint32_t dropped_frames;
	uint32_t spurious_acks;
	uint32_t sequence_mismatch_drop;
	uint32_t resets_received;
	uint32_t crc_fails;
	uint16_t n_ring_buffer_bytes;
	uint16_t n_ring_buffer_bytes_max;
	uint16_t ring_buffer_tail_offset;
	uint8_t n_frames;
	uint8_t n_frames_max;
	uint8_t head_idx;
	uint8_t tail_idx;
	uint32_t sn_min;
	uint32_t sn_max;
	uint32_t rn;
};

#endif /* TRANSPORT_PROTOCOL */

/* ── MIN context ──────────────────────────────────────────────────────── */

struct min_context {
#ifdef TRANSPORT_PROTOCOL
	struct transport_fifo transport_fifo;
#endif
	uint8_t rx_frame_payload_buf[MAX_PAYLOAD];
	uint32_t rx_frame_checksum;
	struct crc32_context rx_checksum;
	struct crc32_context tx_checksum;
	uint8_t rx_header_bytes_seen;
	uint8_t rx_frame_state;
	uint8_t rx_frame_payload_bytes;
	uint8_t rx_frame_id_control;
	uint32_t rx_frame_seq;
	uint8_t rx_frame_length;
	uint8_t rx_control;
	uint8_t tx_header_byte_countdown;
	uint8_t port;
	uint32_t rx_space;
	uint32_t remote_rx_space;
};

/* ── Public API ───────────────────────────────────────────────────────── */

bool min_init_context(struct min_context *self, uint8_t port);
void min_poll(struct min_context *self, uint8_t *buf, uint32_t buf_len);
void min_rx_byte(struct min_context *self, uint8_t byte);
void min_send_frame(struct min_context *self, uint8_t min_id, uint8_t *payload, uint8_t payload_len);
void min_transport_reset(struct min_context *self, bool inform_other_side);

#ifdef TRANSPORT_PROTOCOL
bool min_queue_frame(struct min_context *self, uint8_t min_id, uint8_t *payload, uint8_t payload_len);
bool min_queue_has_space_for_frame(struct min_context *self, uint8_t payload_len);
#endif

/* ── Application callbacks (user must implement) ──────────────────────── */

void min_application_handler(uint8_t min_id, uint8_t *min_payload, uint8_t len_payload, uint8_t port);
uint16_t min_tx_space(uint8_t port);
uint32_t min_rx_space(uint8_t port);
void min_tx_byte(uint8_t port, uint8_t byte);
void min_tx_start(uint8_t port);
void min_tx_finished(uint8_t port);
void min_reset(uint8_t port);
void time_cb(uint32_t remote_time);

#ifdef TRANSPORT_PROTOCOL
uint32_t min_time_ms(void);
#endif

/* ── Debug support ────────────────────────────────────────────────────── */

extern uint8_t min_debug;

#define MIN_DEBUG_PRINTING
#ifdef MIN_DEBUG_PRINTING
void min_debug_prt(const char *msg, ...);
#define min_debug_print(msg, ...) \
	if (min_debug) min_debug_prt(msg, ##__VA_ARGS__);
#else
#define min_debug_print(...)
#endif

#endif /* MIN_H */
