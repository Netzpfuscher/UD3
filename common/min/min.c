/**
 * @file min.c
 * @brief MIN Protocol v2.0 implementation
 *
 * Complete implementation of the MIN serial protocol with optional
 * transport layer for reliable delivery. Uses FreeRTOS tick counter
 * for transport timing when the transport protocol is enabled.
 *
 * Copyright (c) 2014-2017 JK Energy Ltd.
 * Use authorized under the MIT license.
 */

#include "min.h"

#include <string.h>

/* ── Internal constants ───────────────────────────────────────────────── */

#define FIFO_FRAMES_MASK ((uint8_t)((1U << TRANSPORT_FIFO_SIZE_FRAMES_BITS) - 1U))
#define FIFO_DATA_MASK ((uint16_t)((1U << TRANSPORT_FIFO_SIZE_FRAME_DATA_BITS) - 1U))
#define ON_WIRE_SIZE(p) ((uint16_t)(p) + 14U)

enum {
	HEADER_BYTE = 0xaaU,
	STUFF_BYTE = 0x55U,
	EOF_BYTE = 0x55U,
};

typedef enum {
	RX_SEARCHING_FOR_SOF,
	RX_RECEIVING_ID_CONTROL,
	RX_RECEIVING_SEQ_3,
	RX_RECEIVING_SEQ_2,
	RX_RECEIVING_SEQ_1,
	RX_RECEIVING_SEQ_0,
	RX_RECEIVING_LENGTH,
	RX_RECEIVING_PAYLOAD,
	RX_RECEIVING_CHECKSUM_3,
	RX_RECEIVING_CHECKSUM_2,
	RX_RECEIVING_CHECKSUM_1,
	RX_RECEIVING_CHECKSUM_0,
	RX_RECEIVING_EOF,
} rx_state_t;

#define ID_TRANSPORT_BIT 0x80U
#define ID_MASK 0x3fU

uint8_t min_debug = 0;

/* ── Transport protocol configuration ─────────────────────────────────── */

#ifdef TRANSPORT_PROTOCOL

#ifndef TRANSPORT_ACK_RETRANSMIT_TIMEOUT_MS
#define TRANSPORT_ACK_RETRANSMIT_TIMEOUT_MS (25U)
#endif
#ifndef TRANSPORT_FRAME_RETRANSMIT_TIMEOUT_MS
#define TRANSPORT_FRAME_RETRANSMIT_TIMEOUT_MS (50U)
#endif
#ifndef TRANSPORT_MAX_WINDOW_SIZE
#define TRANSPORT_MAX_WINDOW_SIZE (16U)
#endif
#ifndef TRANSPORT_IDLE_TIMEOUT_MS
#define TRANSPORT_IDLE_TIMEOUT_MS (500U)
#endif

enum {
	ACK = 0xffU,
	RESET = 0xfeU,
};

#endif /* TRANSPORT_PROTOCOL */

/* ── CRC32 (software, no lookup table) ────────────────────────────────── */

static inline void crc32_init(struct crc32_context *ctx)
{
	ctx->crc = 0xffffffffU;
}

static const uint32_t crc32_lut[256] = {
	0x00000000U, 0x77073096U, 0xee0e612cU, 0x990951baU, 0x076dc419U, 0x706af48fU, 0xe963a535U, 0x9e6495a3U,
	0x0edb8832U, 0x79dcb8a4U, 0xe0d5e91eU, 0x97d2d988U, 0x09b64c2bU, 0x7eb17cbdU, 0xe7b82d07U, 0x90bf1d91U,
	0x1db71064U, 0x6ab020f2U, 0xf3b97148U, 0x84be41deU, 0x1adad47dU, 0x6ddde4ebU, 0xf4d4b551U, 0x83d385c7U,
	0x136c9856U, 0x646ba8c0U, 0xfd62f97aU, 0x8a65c9ecU, 0x14015c4fU, 0x63066cd9U, 0xfa0f3d63U, 0x8d080df5U,
	0x3b6e20c8U, 0x4c69105eU, 0xd56041e4U, 0xa2677172U, 0x3c03e4d1U, 0x4b04d447U, 0xd20d85fdU, 0xa50ab56bU,
	0x35b5a8faU, 0x42b2986cU, 0xdbbbc9d6U, 0xacbcf940U, 0x32d86ce3U, 0x45df5c75U, 0xdcd60dcfU, 0xabd13d59U,
	0x26d930acU, 0x51de003aU, 0xc8d75180U, 0xbfd06116U, 0x21b4f4b5U, 0x56b3c423U, 0xcfba9599U, 0xb8bda50fU,
	0x2802b89eU, 0x5f058808U, 0xc60cd9b2U, 0xb10be924U, 0x2f6f7c87U, 0x58684c11U, 0xc1611dabU, 0xb6662d3dU,
	0x76dc4190U, 0x01db7106U, 0x98d220bcU, 0xefd5102aU, 0x71b18589U, 0x06b6b51fU, 0x9fbfe4a5U, 0xe8b8d433U,
	0x7807c9a2U, 0x0f00f934U, 0x9609a88eU, 0xe10e9818U, 0x7f6a0dbbU, 0x086d3d2dU, 0x91646c97U, 0xe6635c01U,
	0x6b6b51f4U, 0x1c6c6162U, 0x856530d8U, 0xf262004eU, 0x6c0695edU, 0x1b01a57bU, 0x8208f4c1U, 0xf50fc457U,
	0x65b0d9c6U, 0x12b7e950U, 0x8bbeb8eaU, 0xfcb9887cU, 0x62dd1ddfU, 0x15da2d49U, 0x8cd37cf3U, 0xfbd44c65U,
	0x4db26158U, 0x3ab551ceU, 0xa3bc0074U, 0xd4bb30e2U, 0x4adfa541U, 0x3dd895d7U, 0xa4d1c46dU, 0xd3d6f4fbU,
	0x4369e96aU, 0x346ed9fcU, 0xad678846U, 0xda60b8d0U, 0x44042d73U, 0x33031de5U, 0xaa0a4c5fU, 0xdd0d7cc9U,
	0x5005713cU, 0x270241aaU, 0xbe0b1010U, 0xc90c2086U, 0x5768b525U, 0x206f85b3U, 0xb966d409U, 0xce61e49fU,
	0x5edef90eU, 0x29d9c998U, 0xb0d09822U, 0xc7d7a8b4U, 0x59b33d17U, 0x2eb40d81U, 0xb7bd5c3bU, 0xc0ba6cadU,
	0xedb88320U, 0x9abfb3b6U, 0x03b6e20cU, 0x74b1d29aU, 0xead54739U, 0x9dd277afU, 0x04db2615U, 0x73dc1683U,
	0xe3630b12U, 0x94643b84U, 0x0d6d6a3eU, 0x7a6a5aa8U, 0xe40ecf0bU, 0x9309ff9dU, 0x0a00ae27U, 0x7d079eb1U,
	0xf00f9344U, 0x8708a3d2U, 0x1e01f268U, 0x6906c2feU, 0xf762575dU, 0x806567cbU, 0x196c3671U, 0x6e6b06e7U,
	0xfed41b76U, 0x89d32be0U, 0x10da7a5aU, 0x67dd4accU, 0xf9b9df6fU, 0x8ebeeff9U, 0x17b7be43U, 0x60b08ed5U,
	0xd6d6a3e8U, 0xa1d1937eU, 0x38d8c2c4U, 0x4fdff252U, 0xd1bb67f1U, 0xa6bc5767U, 0x3fb506ddU, 0x48b2364bU,
	0xd80d2bdaU, 0xaf0a1b4cU, 0x36034af6U, 0x41047a60U, 0xdf60efc3U, 0xa867df55U, 0x316e8eefU, 0x4669be79U,
	0xcb61b38cU, 0xbc66831aU, 0x256fd2a0U, 0x5268e236U, 0xcc0c7795U, 0xbb0b4703U, 0x220216b9U, 0x5505262fU,
	0xc5ba3bbeU, 0xb2bd0b28U, 0x2bb45a92U, 0x5cb36a04U, 0xc2d7ffa7U, 0xb5d0cf31U, 0x2cd99e8bU, 0x5bdeae1dU,
	0x9b64c2b0U, 0xec63f226U, 0x756aa39cU, 0x026d930aU, 0x9c0906a9U, 0xeb0e363fU, 0x72076785U, 0x05005713U,
	0x95bf4a82U, 0xe2b87a14U, 0x7bb12baeU, 0x0cb61b38U, 0x92d28e9bU, 0xe5d5be0dU, 0x7cdcefb7U, 0x0bdbdf21U,
	0x86d3d2d4U, 0xf1d4e242U, 0x68ddb3f8U, 0x1fda836eU, 0x81be16cdU, 0xf6b9265bU, 0x6fb077e1U, 0x18b74777U,
	0x88085ae6U, 0xff0f6a70U, 0x66063bcaU, 0x11010b5cU, 0x8f659effU, 0xf862ae69U, 0x616bffd3U, 0x166ccf45U,
	0xa00ae278U, 0xd70dd2eeU, 0x4e048354U, 0x3903b3c2U, 0xa7672661U, 0xd06016f7U, 0x4969474dU, 0x3e6e77dbU,
	0xaed16a4aU, 0xd9d65adcU, 0x40df0b66U, 0x37d83bf0U, 0xa9bcae53U, 0xdebb9ec5U, 0x47b2cf7fU, 0x30b5ffe9U,
	0xbdbdf21cU, 0xcabac28aU, 0x53b39330U, 0x24b4a3a6U, 0xbad03605U, 0xcdd70693U, 0x54de5729U, 0x23d967bfU,
	0xb3667a2eU, 0xc4614ab8U, 0x5d681b02U, 0x2a6f2b94U, 0xb40bbe37U, 0xc30c8ea1U, 0x5a05df1bU, 0x2d02ef8dU,
};

static bool crc32_lut_check(void)
{
	for (uint32_t i = 0; i < 256U; i++) {
		uint32_t c = i;
		for (uint32_t j = 0; j < 8U; j++) {
			if (c & 1U) {
				c = (c >> 1) ^ 0xedb88320U;
			} else {
				c >>= 1;
			}
		}
		if (c != crc32_lut[i]) {
			return false;
		}
	}
	return true;
}

static inline void crc32_step(struct crc32_context *ctx, uint8_t byte)
{
	ctx->crc = (ctx->crc >> 8) ^ crc32_lut[(ctx->crc ^ byte) & 0xffU];
}

static inline uint32_t crc32_finalize(const struct crc32_context *ctx)
{
	return ~ctx->crc;
}

/* ── Byte-order helpers ───────────────────────────────────────────────── */

static inline uint8_t u32_byte(uint32_t v, uint8_t n)
{
	return (uint8_t)((v >> (n * 8)) & 0xffU);
}

static inline uint32_t read_be32(const uint8_t *p)
{
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
		   ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static inline void write_be32(uint8_t *p, uint32_t v)
{
	p[0] = u32_byte(v, 3);
	p[1] = u32_byte(v, 2);
	p[2] = u32_byte(v, 1);
	p[3] = u32_byte(v, 0);
}

/* ── TX with byte-stuffing ────────────────────────────────────────────── */

static inline void stuff_check(struct min_context *self, uint8_t byte)
{
	if (byte == HEADER_BYTE) {
		if (--self->tx_header_byte_countdown == 0) {
			min_tx_byte(self->port, STUFF_BYTE);
			self->tx_header_byte_countdown = 2U;
		}
	} else {
		self->tx_header_byte_countdown = 2U;
	}
}

static void tx_byte_stuffed(struct min_context *self, uint8_t byte, bool update_crc)
{
	min_tx_byte(self->port, byte);
	if (update_crc) {
		crc32_step(&self->tx_checksum, byte);
	}
	stuff_check(self, byte);
}

/* ── Frame construction and transmission ──────────────────────────────── */

static void tx_header(struct min_context *self)
{
	min_tx_byte(self->port, HEADER_BYTE);
	min_tx_byte(self->port, HEADER_BYTE);
	min_tx_byte(self->port, HEADER_BYTE);
}

static void transmit_frame(struct min_context *self, uint8_t id_control, uint32_t seq,
						   uint8_t *payload_base, uint16_t payload_offset,
						   uint16_t payload_mask, uint8_t payload_len)
{
	self->tx_header_byte_countdown = 2U;
	crc32_init(&self->tx_checksum);
	min_tx_start(self->port);

	tx_header(self);

	tx_byte_stuffed(self, id_control, true);

	if (id_control & ID_TRANSPORT_BIT) {
		tx_byte_stuffed(self, u32_byte(seq, 3), true);
		tx_byte_stuffed(self, u32_byte(seq, 2), true);
		tx_byte_stuffed(self, u32_byte(seq, 1), true);
		tx_byte_stuffed(self, u32_byte(seq, 0), true);
	}

	tx_byte_stuffed(self, payload_len, true);

	for (uint8_t i = 0; i < payload_len; i++) {
		tx_byte_stuffed(self, payload_base[payload_offset], true);
		payload_offset = (payload_offset + 1) & payload_mask;
	}

	uint32_t checksum = crc32_finalize(&self->tx_checksum);
	tx_byte_stuffed(self, u32_byte(checksum, 3), false);
	tx_byte_stuffed(self, u32_byte(checksum, 2), false);
	tx_byte_stuffed(self, u32_byte(checksum, 1), false);
	tx_byte_stuffed(self, u32_byte(checksum, 0), false);

	min_tx_byte(self->port, EOF_BYTE);
	min_tx_finished(self->port);
}

/* ── Transport FIFO operations ────────────────────────────────────────── */

#ifdef TRANSPORT_PROTOCOL

static inline uint32_t now_ms(void)
{
	return min_time_ms();
}

static void fifo_pop(struct min_context *self)
{
	struct transport_frame *frame = &self->transport_fifo.frames[self->transport_fifo.head_idx];
	min_debug_print("Popping frame id=%d seq=%d\r\n", frame->min_id, frame->seq);

	self->transport_fifo.n_frames--;
	self->transport_fifo.head_idx = (self->transport_fifo.head_idx + 1) & FIFO_FRAMES_MASK;
	self->transport_fifo.n_ring_buffer_bytes -= frame->payload_len;
}

static struct transport_frame *fifo_push(struct min_context *self, uint16_t data_size)
{
	if (self->transport_fifo.n_frames >= TRANSPORT_FIFO_MAX_FRAMES) {
		min_debug_print("No FIFO frame slots\r\n");
		return NULL;
	}

	if (self->transport_fifo.n_ring_buffer_bytes > TRANSPORT_FIFO_MAX_FRAME_DATA - data_size) {
		min_debug_print("No FIFO payload space: data_size=%d, n_ring_buffer_bytes=%d\r\n",
						data_size, self->transport_fifo.n_ring_buffer_bytes);
		return NULL;
	}

	self->transport_fifo.n_frames++;
	if (self->transport_fifo.n_frames > self->transport_fifo.n_frames_max) {
		self->transport_fifo.n_frames_max = self->transport_fifo.n_frames;
	}

	struct transport_frame *frame = &self->transport_fifo.frames[self->transport_fifo.tail_idx];
	frame->payload_offset = self->transport_fifo.ring_buffer_tail_offset;

	self->transport_fifo.n_ring_buffer_bytes += data_size;
	if (self->transport_fifo.n_ring_buffer_bytes > self->transport_fifo.n_ring_buffer_bytes_max) {
		self->transport_fifo.n_ring_buffer_bytes_max = self->transport_fifo.n_ring_buffer_bytes;
	}

	self->transport_fifo.ring_buffer_tail_offset =
		(self->transport_fifo.ring_buffer_tail_offset + data_size) & FIFO_DATA_MASK;
	self->transport_fifo.tail_idx =
		(self->transport_fifo.tail_idx + 1) & FIFO_FRAMES_MASK;

	return frame;
}

static inline struct transport_frame *fifo_get(struct min_context *self, uint8_t n)
{
	return &self->transport_fifo.frames[(self->transport_fifo.head_idx + n) & FIFO_FRAMES_MASK];
}

static void fifo_send(struct min_context *self, struct transport_frame *frame)
{
	transmit_frame(self, frame->min_id | ID_TRANSPORT_BIT, frame->seq,
				   self->transport_fifo.payloads, frame->payload_offset,
				   FIFO_DATA_MASK, frame->payload_len);
	frame->last_sent_time_ms = now_ms();
}

static void fifo_reset(struct min_context *self)
{
	uint32_t now = now_ms();

	self->transport_fifo.n_frames = 0;
	self->transport_fifo.head_idx = 0;
	self->transport_fifo.tail_idx = 0;
	self->transport_fifo.n_ring_buffer_bytes = 0;
	self->transport_fifo.ring_buffer_tail_offset = 0;
	self->transport_fifo.sn_max = 0;
	self->transport_fifo.sn_min = 0;
	self->transport_fifo.rn = 0;

	self->transport_fifo.last_received_anything_ms = now;
	self->transport_fifo.last_sent_ack_time_ms = now;
	self->transport_fifo.last_received_frame_ms = 0;
}

/* ── Transport protocol frames (ACK / RESET) ─────────────────────────── */

static void send_ack(struct min_context *self)
{
	min_debug_print("send ACK: seq=%d\r\n", self->transport_fifo.rn);

	if (ON_WIRE_SIZE(8) > min_tx_space(self->port)) {
		return;
	}

	self->rx_space = min_rx_space(self->port);

	uint8_t payload[8];
	write_be32(&payload[0], self->transport_fifo.rn);
	write_be32(&payload[4], self->rx_space);

	transmit_frame(self, ACK, self->transport_fifo.rn, payload, 0, 0xffU, sizeof(payload));
	self->transport_fifo.last_sent_ack_time_ms = now_ms();
}

static void send_reset(struct min_context *self)
{
	min_debug_print("send RESET\r\n");

	if (ON_WIRE_SIZE(0) <= min_tx_space(self->port)) {
		transmit_frame(self, RESET, 0, NULL, 0, 0, 0);
	}
}

static struct transport_frame *find_oldest_unacked_frame(struct min_context *self)
{
	uint32_t now = now_ms();
	uint8_t window_size = self->transport_fifo.sn_max - self->transport_fifo.sn_min;

	struct transport_frame *oldest = &self->transport_fifo.frames[self->transport_fifo.head_idx];
	uint32_t oldest_age = now - oldest->last_sent_time_ms;

	uint8_t idx = self->transport_fifo.head_idx;
	for (uint8_t i = 0; i < window_size; i++) {
		uint32_t age = now - self->transport_fifo.frames[idx].last_sent_time_ms;
		if (age > oldest_age) {
			oldest_age = age;
			oldest = &self->transport_fifo.frames[idx];
		}
		idx = (idx + 1) & FIFO_FRAMES_MASK;
	}

	return oldest;
}

/* ── Transport protocol: ACK processing ───────────────────────────────── */

static void handle_ack(struct min_context *self, uint32_t seq, uint8_t *payload, uint8_t payload_len)
{
	if (payload_len < 8) {
		min_debug_print("ACK payload too short: %d bytes\r\n", payload_len);
		return;
	}

	uint32_t num_acked = seq - self->transport_fifo.sn_min;
	uint32_t num_nacked = read_be32(&payload[0]);
	uint32_t num_in_window = self->transport_fifo.sn_max - self->transport_fifo.sn_min;

	if (num_nacked < seq) {
		min_debug_print("Invalid ACK: num_nacked=%u < seq=%u\r\n", num_nacked, seq);
		num_nacked = 0;
	} else {
		num_nacked -= seq;
	}

	self->remote_rx_space = read_be32(&payload[4]);

	if (payload_len >= 12) {
		time_cb(read_be32(&payload[8]));
	}

	if (num_acked > num_in_window) {
		min_debug_print("Received spurious ACK seq=%d\r\n", seq);
		self->transport_fifo.spurious_acks++;
		return;
	}

	self->transport_fifo.sn_min = seq;
	min_debug_print("Received ACK seq=%d, num_acked=%d, num_nacked=%d\r\n", seq, num_acked, num_nacked);

	for (uint8_t i = 0; i < num_acked; i++) {
		fifo_pop(self);
	}

	uint8_t idx = self->transport_fifo.head_idx;
	for (uint8_t i = 0; i < num_nacked; i++) {
		fifo_send(self, &self->transport_fifo.frames[idx]);
		idx = (idx + 1) & FIFO_FRAMES_MASK;
	}
}

static void handle_app_frame(struct min_context *self, uint8_t id_control, uint32_t seq,
							 uint8_t *payload, uint8_t payload_len, uint32_t now)
{
	self->transport_fifo.last_received_frame_ms = now;

	if (seq == self->transport_fifo.rn) {
		self->transport_fifo.rn++;
		send_ack(self);

		min_debug_print("Incoming app frame seq=%d, id=%d, payload len=%d\r\n",
						seq, id_control & ID_MASK, payload_len);
		min_application_handler(id_control & ID_MASK, payload, payload_len, self->port);
	} else {
		self->transport_fifo.sequence_mismatch_drop++;
	}
}

#endif /* TRANSPORT_PROTOCOL */

/* ── RX frame dispatch ────────────────────────────────────────────────── */

static void valid_frame_received(struct min_context *self)
{
	uint8_t id_control = self->rx_frame_id_control;
	uint8_t *payload = self->rx_frame_payload_buf;
	uint8_t payload_len = self->rx_control;

#ifdef TRANSPORT_PROTOCOL
	uint32_t now = now_ms();
	uint32_t seq = self->rx_frame_seq;

	self->transport_fifo.last_received_anything_ms = now;

	switch (id_control) {
	case ACK:
		handle_ack(self, seq, payload, payload_len);
		break;
	case RESET:
		self->transport_fifo.resets_received++;
		fifo_reset(self);
		break;
	default:
		if (id_control & ID_TRANSPORT_BIT) {
			handle_app_frame(self, id_control, seq, payload, payload_len, now);
		} else {
			min_application_handler(id_control & ID_MASK, payload, payload_len, self->port);
		}
		break;
	}
#else
	min_application_handler(id_control & ID_MASK, payload, payload_len, self->port);
#endif
}

/* ── RX state machine ─────────────────────────────────────────────────── */

static void rx_handle_header_context(struct min_context *self, uint8_t byte)
{
	self->rx_header_bytes_seen = 0;

	if (byte == HEADER_BYTE) {
		self->rx_frame_state = RX_RECEIVING_ID_CONTROL;
	} else if (byte != STUFF_BYTE) {
		self->rx_frame_state = RX_SEARCHING_FOR_SOF;
	}
}

void min_rx_byte(struct min_context *self, uint8_t byte)
{
	if (self->rx_header_bytes_seen == 2) {
		rx_handle_header_context(self, byte);
		return;
	}

	if (byte == HEADER_BYTE) {
		self->rx_header_bytes_seen++;
	} else {
		self->rx_header_bytes_seen = 0;
	}

	switch ((rx_state_t)self->rx_frame_state) {
	case RX_SEARCHING_FOR_SOF:
		break;

	case RX_RECEIVING_ID_CONTROL:
		self->rx_frame_id_control = byte;
		self->rx_frame_payload_bytes = 0;
		crc32_init(&self->rx_checksum);
		crc32_step(&self->rx_checksum, byte);

		if (byte & ID_TRANSPORT_BIT) {
#ifdef TRANSPORT_PROTOCOL
			self->rx_frame_state = RX_RECEIVING_SEQ_3;
#else
			self->rx_frame_state = RX_SEARCHING_FOR_SOF;
#endif
		} else {
			self->rx_frame_seq = 0;
			self->rx_frame_state = RX_RECEIVING_LENGTH;
		}
		break;

	case RX_RECEIVING_SEQ_3:
		self->rx_frame_seq = (uint32_t)byte << 24;
		crc32_step(&self->rx_checksum, byte);
		self->rx_frame_state = RX_RECEIVING_SEQ_2;
		break;

	case RX_RECEIVING_SEQ_2:
		self->rx_frame_seq |= (uint32_t)byte << 16;
		crc32_step(&self->rx_checksum, byte);
		self->rx_frame_state = RX_RECEIVING_SEQ_1;
		break;

	case RX_RECEIVING_SEQ_1:
		self->rx_frame_seq |= (uint32_t)byte << 8;
		crc32_step(&self->rx_checksum, byte);
		self->rx_frame_state = RX_RECEIVING_SEQ_0;
		break;

	case RX_RECEIVING_SEQ_0:
		self->rx_frame_seq |= byte;
		crc32_step(&self->rx_checksum, byte);
		self->rx_frame_state = RX_RECEIVING_LENGTH;
		break;

	case RX_RECEIVING_LENGTH:
		self->rx_frame_length = byte;
		self->rx_control = byte;
		crc32_step(&self->rx_checksum, byte);

		if (byte == 0) {
			self->rx_frame_state = RX_RECEIVING_CHECKSUM_3;
		} else if (byte <= MAX_PAYLOAD) {
			self->rx_frame_state = RX_RECEIVING_PAYLOAD;
		} else {
			self->rx_frame_state = RX_SEARCHING_FOR_SOF;
		}
		break;

	case RX_RECEIVING_PAYLOAD:
		self->rx_frame_payload_buf[self->rx_frame_payload_bytes++] = byte;
		crc32_step(&self->rx_checksum, byte);
		if (--self->rx_frame_length == 0) {
			self->rx_frame_state = RX_RECEIVING_CHECKSUM_3;
		}
		break;

	case RX_RECEIVING_CHECKSUM_3:
		self->rx_frame_checksum = (uint32_t)byte << 24;
		self->rx_frame_state = RX_RECEIVING_CHECKSUM_2;
		break;

	case RX_RECEIVING_CHECKSUM_2:
		self->rx_frame_checksum |= (uint32_t)byte << 16;
		self->rx_frame_state = RX_RECEIVING_CHECKSUM_1;
		break;

	case RX_RECEIVING_CHECKSUM_1:
		self->rx_frame_checksum |= (uint32_t)byte << 8;
		self->rx_frame_state = RX_RECEIVING_CHECKSUM_0;
		break;

	case RX_RECEIVING_CHECKSUM_0: {
		self->rx_frame_checksum |= byte;
		uint32_t crc = crc32_finalize(&self->rx_checksum);
		if (self->rx_frame_checksum == crc) {
			self->rx_frame_state = RX_RECEIVING_EOF;
		} else {
			self->transport_fifo.crc_fails++;
			self->rx_frame_state = RX_SEARCHING_FOR_SOF;
		}
		break;
	}

	case RX_RECEIVING_EOF:
		if (byte == EOF_BYTE) {
			valid_frame_received(self);
		}
		self->rx_frame_state = RX_SEARCHING_FOR_SOF;
		break;

	default:
		self->rx_frame_state = RX_SEARCHING_FOR_SOF;
		break;
	}
}

/* ── Transport polling (retransmit / window management) ───────────────── */

#ifdef TRANSPORT_PROTOCOL

static void poll_transport(struct min_context *self)
{
	if (self->rx_frame_state != RX_SEARCHING_FOR_SOF) {
		return;
	}

	uint32_t now = now_ms();

	bool remote_connected = (now - self->transport_fifo.last_received_anything_ms < TRANSPORT_IDLE_TIMEOUT_MS);
	bool remote_active = (now - self->transport_fifo.last_received_frame_ms < TRANSPORT_IDLE_TIMEOUT_MS);

	if (!remote_connected) {
		min_transport_reset(self, true);
		return;
	}

	uint8_t window_size = self->transport_fifo.sn_max - self->transport_fifo.sn_min;

	if (window_size < TRANSPORT_MAX_WINDOW_SIZE && self->transport_fifo.n_frames > window_size) {
		struct transport_frame *frame = fifo_get(self, window_size);
		uint16_t wire_size = ON_WIRE_SIZE(frame->payload_len);

		if (wire_size <= min_tx_space(self->port) && wire_size <= self->remote_rx_space) {
			frame->seq = self->transport_fifo.sn_max;
			fifo_send(self, frame);
			self->transport_fifo.sn_max++;
		}
	} else if (window_size > 0) {
		struct transport_frame *oldest = find_oldest_unacked_frame(self);

		if (now - oldest->last_sent_time_ms >= TRANSPORT_FRAME_RETRANSMIT_TIMEOUT_MS) {
			uint16_t wire_size = ON_WIRE_SIZE(oldest->payload_len);

			if (wire_size <= min_tx_space(self->port) && wire_size <= self->remote_rx_space) {
				fifo_send(self, oldest);
			}
		}
	}

#ifndef DISABLE_TRANSPORT_ACK_RETRANSMIT
	if (now - self->transport_fifo.last_sent_ack_time_ms > TRANSPORT_ACK_RETRANSMIT_TIMEOUT_MS) {
		if (remote_active) {
			send_ack(self);
		}
	}
#endif
}

#endif /* TRANSPORT_PROTOCOL */

/* ── Public API ───────────────────────────────────────────────────────── */

void min_poll(struct min_context *self, uint8_t *buf, uint32_t buf_len)
{
	for (uint32_t i = 0; i < buf_len; i++) {
		min_rx_byte(self, buf[i]);
	}

#ifdef TRANSPORT_PROTOCOL
	poll_transport(self);
#endif
}

bool min_init_context(struct min_context *self, uint8_t port)
{
	if (!crc32_lut_check()) {
		return false;
	}

	memset(self, 0, sizeof(*self));
	self->rx_frame_state = RX_SEARCHING_FOR_SOF;
	self->port = port;
	self->remote_rx_space = 512;

#ifdef TRANSPORT_PROTOCOL
	fifo_reset(self);
#endif
	return true;
}

void min_send_frame(struct min_context *self, uint8_t min_id, uint8_t *payload, uint8_t payload_len)
{
	if (ON_WIRE_SIZE(payload_len) <= min_tx_space(self->port)) {
		transmit_frame(self, min_id & ID_MASK, 0, payload, 0, 0xffffU, payload_len);
	}
}

void min_transport_reset(struct min_context *self, bool inform_other_side)
{
#ifdef TRANSPORT_PROTOCOL
	if (inform_other_side) {
		send_reset(self);
	}
	fifo_reset(self);
#endif
	min_reset(self->port);
}

#ifdef TRANSPORT_PROTOCOL

bool min_queue_frame(struct min_context *self, uint8_t min_id, uint8_t *payload, uint8_t payload_len)
{
	struct transport_frame *frame = fifo_push(self, payload_len);
	if (frame == NULL) {
		self->transport_fifo.dropped_frames++;
		return false;
	}

	frame->min_id = min_id & ID_MASK;
	frame->payload_len = payload_len;

	uint16_t offset = frame->payload_offset;
	for (uint32_t i = 0; i < payload_len; i++) {
		self->transport_fifo.payloads[offset] = payload[i];
		offset = (offset + 1) & FIFO_DATA_MASK;
	}

	min_debug_print("Queued ID=%u, len=%d\r\n", min_id, payload_len);
	return true;
}

bool min_queue_has_space_for_frame(struct min_context *self, uint8_t payload_len)
{
	return self->transport_fifo.n_frames < TRANSPORT_FIFO_MAX_FRAMES &&
		   self->transport_fifo.n_ring_buffer_bytes <= TRANSPORT_FIFO_MAX_FRAME_DATA - payload_len;
}

#endif /* TRANSPORT_PROTOCOL */
