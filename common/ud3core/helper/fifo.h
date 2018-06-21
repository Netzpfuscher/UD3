#ifndef FIFO_H
#define FIFO_H
#include <stdint.h>

typedef struct
{
	uint8_t volatile count;		 // # Zeichen im Puffer
	uint8_t size;				 // Puffer-Größe
	uint8_t *pread;				 // Lesezeiger
	uint8_t *pwrite;			 // Schreibzeiger
	uint8_t read2end, write2end; // # Zeichen bis zum Überlauf Lese-/Schreibzeiger
} fifo_t;

extern void fifo_init(fifo_t *, uint8_t *buf, const uint8_t size);
extern uint8_t fifo_put(fifo_t *, const uint8_t data);
extern uint8_t fifo_get_wait(fifo_t *);
extern int fifo_get_nowait(fifo_t *);

static inline uint8_t
_inline_fifo_put(fifo_t *f, const uint8_t data) {
	if (f->count >= f->size)
		return 0;

	uint8_t *pwrite = f->pwrite;

	*(pwrite++) = data;

	uint8_t write2end = f->write2end;

	if (--write2end == 0) {
		write2end = f->size;
		pwrite -= write2end;
	}

	f->write2end = write2end;
	f->pwrite = pwrite;

	f->count++;

	return 1;
}

static inline uint8_t
_inline_fifo_get(fifo_t *f) {
	uint8_t *pread = f->pread;
	uint8_t data = *(pread++);
	uint8_t read2end = f->read2end;

	if (--read2end == 0) {
		read2end = f->size;
		pread -= read2end;
	}

	f->pread = pread;
	f->read2end = read2end;

	f->count--;

	return data;
}

#endif /* FIFO_H */