#include "music.h"

void mixer_init(void) {
    // todo
}

void mixer_upload_sample_data(const void* const sample_data, size_t n_bytes, soundbank_type_t soundbank_type) {
    // todo
}

void mixer_global_set_volume(scalar_t left, scalar_t right) {
    // todo
}

void mixer_set_music_tempo(uint32_t raw_tempo) {
    // todo
}

void mixer_channel_set_sample_rate(size_t channel_index, scalar_t sample_rate) {
    // todo
}

void mixer_channel_set_volume(size_t channel_index, scalar_t left, scalar_t right) {
    // todo
}

void mixer_channel_set_sample(size_t channel_index, size_t sample_offset, soundbank_type_t soundbank_type) {
    // todo
}

void mixer_channel_key_on(uint32_t channel_bits) {
    // todo
}

void mixer_channel_key_off(uint32_t channel_bits) {
    // todo
}

int mixer_channel_is_idle(size_t channel_index) {
    // todo
    return 0;
}
