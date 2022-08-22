#include "tsk_audio.h"


#include "FreeRTOS.h"
#include "SignalGeneratorSID.h"
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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

const int audio_rate = 44100;
const int interrupter_clock = 1000000;
const int pwm_counts_per_sample = interrupter_clock / audio_rate;

bool is_active(size_t old_inter, size_t new_inter, int prd) {
    return new_inter / prd > old_inter / prd;
}

int get_tr_volume(size_t old_inter, size_t new_inter) {
    if (!(interrupter1_control_Control & 2)) {
        return 0;
    } else {
        return (1 << 12) * is_active(old_inter, new_inter, int1_prd);
    }
}

bool add_volume(
    size_t pwm_counter, int16_t* audio_buffer, size_t buffer_size, size_t prd, int16_t pw, bool noise
) {
    int volume = (1 << 14) * pw / configuration.max_tr_pw;
    size_t prd_buffer = prd / pwm_counts_per_sample;
    size_t offset = (prd - pwm_counter % prd) / pwm_counts_per_sample;
    int num_changed = 0;
    for (size_t i = offset; i < buffer_size; i += prd_buffer) {
        size_t index_to_increase = i;
        if (noise) {
            size_t noise_range = prd_buffer / 2;
            size_t noise = rand() % noise_range;
            index_to_increase += noise;
            if (index_to_increase >= buffer_size) {
                index_to_increase = buffer_size - 1;
            }
        }
        audio_buffer[index_to_increase] += volume;
        ++num_changed;
    }
    return num_changed > 0;
}

// If TR is briefly turned on (e.g. burst mode) at low frequencies, we want to play at least one cycle, even if that
// falls outside the window where TR was enabled
bool force_tr_next_cycle = false;

void add_tr_volume(size_t pwm_counter, int16_t* audio_buffer, size_t samples_in_batch) {
    if (force_tr_next_cycle || (interrupter1_control_Control & 2)) {
        force_tr_next_cycle = !add_volume(
            pwm_counter, audio_buffer, samples_in_batch, int1_prd, int1_prd - int1_cmp, false
        );
    }
}

void add_total_dds_volume(size_t pwm_counter, int16_t* audio_buffer, size_t buffer_size) {
    for (int channel = 0; channel < 4; ++channel) {
        if (DDS32_en[channel]) {
            int prd = interrupter_clock / (DDS32_freq[channel] >> 8);
            int pw = ch_prd[channel] - ch_cmp[channel];
            add_volume(pwm_counter, audio_buffer, buffer_size, prd, pw,  DDS32_noise[channel] != 0);
        }
    }
}

void tsk_audio(void *pvParameters) {
    FILE* audio_pipe = popen("aplay --rate=44100 --format=S16_LE", "w");
    const int batches_per_second = 50;
    const int ms_per_audio_batch = 1000 / batches_per_second;
    const TickType_t batch_delay = ms_per_audio_batch / portTICK_PERIOD_MS;

    const int ns_per_sample = 1e9 / audio_rate;
    size_t pwm_counter = 0;
    int next_channel = 0;
    struct timespec last_time;
    clock_gettime(CLOCK_REALTIME, &last_time);
    int16_t* audio_buffer = NULL;
    while(1){
        vTaskDelay(batch_delay);

        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        // Keep ahead by 10 ms
        now.tv_nsec += 10e6;
        const size_t ns_between = 1e9 * (now.tv_sec - last_time.tv_sec) + now.tv_nsec - last_time.tv_nsec;
        const int samples_in_batch = ns_between / ns_per_sample;
        last_time = now;

        size_t buffer_bytes = sizeof(*audio_buffer) * samples_in_batch;
        audio_buffer = realloc(audio_buffer, buffer_bytes);
        memset(audio_buffer, 0, buffer_bytes);
        add_tr_volume(pwm_counter, audio_buffer, samples_in_batch);
        add_total_dds_volume(pwm_counter, audio_buffer, samples_in_batch);
        pwm_counter += pwm_counts_per_sample * samples_in_batch;
        fwrite(audio_buffer, sizeof(*audio_buffer), samples_in_batch, audio_pipe);
        fflush(audio_pipe);
    }
}

xTaskHandle tsk_audio_TaskHandle;

void tsk_audio_Start() {
    char const* audio_enabled = getenv("ud3sim_audio");
    if (audio_enabled && *audio_enabled == '1') {
        console_print("Audio init...\n");
        xTaskCreate(tsk_audio, "AUDIO", 1024, NULL, 3, &tsk_audio_TaskHandle);
    }
}
