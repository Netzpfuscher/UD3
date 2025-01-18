#include "tsk_audio.h"


#include "FreeRTOS.h"
#include "qcw.h"
#include "task.h"
#include "queue.h"
#include "console.h"

#include "tasks/tsk_analog.h"
#include "semphr.h"
#include "telemetry.h"
#include "interrupter.h"
#include "cli_common.h"
#include "cli_basic.h"
#include "clock.h"
#include "SignalGenerator.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

const int audio_rate = 44100;
const int siggen_clock = 32000000;
const int ontime_clock = 1000000;
const int ns_per_second = 1000000000;
const int sg_clocks_per_ns = ns_per_second / siggen_clock;
const int sg_counts_per_sample = siggen_clock / audio_rate;
const int ot_counts_per_sample = ontime_clock / audio_rate;
const int buffers_per_second = 10;
const int buffer_size = audio_rate / buffers_per_second;

typedef struct {
    int16_t* active_buffer;
    int16_t* next_buffer;
    int last_pulse_clock;
    bool main_buffer_full;
    struct timespec buffer_scheduled_time;
} AudioBuffers;

void add_nanoseconds(struct timespec* time, int ns) {
    time->tv_nsec += ns;
    while (time->tv_nsec > ns_per_second) {
        time->tv_nsec -= ns_per_second;
        ++time->tv_sec;
    }
}

void add_pulse(AudioBuffers* buffers, SigGen_pulseData_t pulse) {
    // TODO handle pulses with more than a full buffer of delay. Then we can reduce buffer size again
    buffers->last_pulse_clock += pulse.period;

    int16_t* buffer_to_write;
    if (buffers->last_pulse_clock / sg_counts_per_sample >= buffer_size) {
        buffers->last_pulse_clock -= buffer_size * sg_counts_per_sample;
        assert(buffers->last_pulse_clock / sg_counts_per_sample < buffer_size);
        buffer_to_write = buffers->next_buffer;
        buffers->main_buffer_full = true;
    } else {
        buffer_to_write = buffers->active_buffer;
    }

    int next_index = buffers->last_pulse_clock / sg_counts_per_sample;
    while (pulse.onTime > 0) {
        double fraction_of_sample = fmin(pulse.onTime / (double) ot_counts_per_sample, 1);
        buffer_to_write[next_index] += (pulse.current << 4) * fraction_of_sample;
        ++next_index;
        pulse.onTime -= ot_counts_per_sample;
        if (next_index >= buffer_size) {
            if (buffer_to_write == buffers->active_buffer) {
                buffer_to_write = buffers->next_buffer;
                next_index -= buffer_size;
            } else {
                // wtf
                break;
            }
        }
    }
}

AudioBuffers buffers;
FILE* audio_pipe;

void tsk_audio_Start() {
    char const* audio_enabled = getenv("ud3sim_audio");
    if (audio_enabled && *audio_enabled == '1') {
        buffers.last_pulse_clock = 0;
        buffers.main_buffer_full = false;
        clock_gettime(CLOCK_REALTIME, &buffers.buffer_scheduled_time);
        buffers.active_buffer = calloc(buffer_size, sizeof(*buffers.active_buffer));
        buffers.next_buffer = calloc(buffer_size, sizeof(*buffers.next_buffer));
        audio_pipe = popen("aplay --rate=44100 --format=S16_LE", "w");
        //audio_pipe = fopen("temp.dat", "w");
        console_print("Audio init...\n");
    }
}

bool is_before(struct timespec a, struct timespec b) {
    if (a.tv_sec != b.tv_sec) {
        return a.tv_sec < b.tv_sec;
    } else {
        return a.tv_nsec < b.tv_nsec;
    }
}

void simulator_process_audio(SigGen_taskData_t* data, SigGen_pulseData_t* read_pulse) {
    if (!audio_pipe) { return; }
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    if (read_pulse->onTime > 0 && read_pulse->current > 0) {
        add_pulse(&buffers, *read_pulse);
        read_pulse->onTime = read_pulse->current = 0;
    }
    SigGen_pulseData_t next_pulse;
    while (!buffers.main_buffer_full&& RingBuffer_read(data->pulseBuffer, (void*)&next_pulse, 1) == 1) {
        data->bufferLengthInCounts -= next_pulse.period;
        add_pulse(&buffers, next_pulse);
    }
    if (is_before(buffers.buffer_scheduled_time, now)) {
        int16_t* buffer = buffers.active_buffer;
        fwrite(buffer, 1, sizeof(*buffer) * buffer_size, audio_pipe);
        fflush(audio_pipe);

        buffers.active_buffer = buffers.next_buffer;
        memset(buffer, 0, sizeof(*buffer) * buffer_size);
        buffers.next_buffer = buffer;

        add_nanoseconds(&buffers.buffer_scheduled_time, ns_per_second / buffers_per_second);
        buffers.main_buffer_full = false;
    }
}
