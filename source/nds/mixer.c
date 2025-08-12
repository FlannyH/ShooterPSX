#include "music.h"

#include "../common.h"

void mixer_init(void) {
    // todo
}

void mixer_upload_sample_data(const void* const sample_data, size_t n_bytes, soundbank_type_t soundbank_type) {
    // todo
    (void)sample_data;
    (void)n_bytes;
    (void)soundbank_type;
}

void mixer_global_set_volume(scalar_t left, scalar_t right) {
    // todo
    (void)left;
    (void)right;
}

void mixer_set_music_tempo(uint32_t raw_tempo) {
    // todo
    (void)raw_tempo;
}

void mixer_channel_set_sample_rate(size_t channel_index, scalar_t sample_rate) {
    // todo
    (void)channel_index;
    (void)sample_rate;
}

void mixer_channel_set_volume(size_t channel_index, scalar_t left, scalar_t right) {
    // todo
    (void)channel_index;
    (void)left;
    (void)right;
}

void mixer_channel_set_sample(size_t channel_index, size_t sample_source, size_t loop_start, size_t sample_length, soundbank_type_t soundbank_type) {
    // todo
    (void)channel_index;
    (void)sample_source;
    (void)loop_start;
    (void)sample_length;
    (void)soundbank_type;
}

void mixer_channel_key_on(uint32_t channel_bits) {
    // todo
    (void)channel_bits;
}

void mixer_channel_key_off(uint32_t channel_bits) {
    // todo
    (void)channel_bits;
}

int mixer_channel_is_idle(size_t channel_index) {
    // todo
    (void)channel_index;
    return 0;
}
