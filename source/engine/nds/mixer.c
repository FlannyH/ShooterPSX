#include "music.h"

#include "../common.h"

void mixer_init(void) {
    // todo(mixer_init): desc: mixer_init()
}

void mixer_upload_sample_data(const void* const sample_data, size_t n_bytes, soundbank_type_t soundbank_type) {
    // todo(mixer_upload_sample_data): desc: mixer_upload_sample_data()
    (void)sample_data;
    (void)n_bytes;
    (void)soundbank_type;
}

void mixer_global_set_volume(scalar_t left, scalar_t right) {
    // todo(mixer_global_set_volume): desc: mixer_global_set_volume()
    (void)left;
    (void)right;
}

void mixer_set_music_tempo(uint32_t raw_tempo) {
    // todo(mixer_set_music_tempo): desc: mixer_set_music_tempo()
    (void)raw_tempo;
}

void mixer_channel_set_sample_rate(size_t channel_index, scalar_t sample_rate) {
    // todo(mixer_channel_set_sample_rate): desc: mixer_channel_set_sample_rate()
    (void)channel_index;
    (void)sample_rate;
}

void mixer_channel_set_volume(size_t channel_index, scalar_t left, scalar_t right) {
    // todo(mixer_channel_set_volume): desc: mixer_channel_set_volume()
    (void)channel_index;
    (void)left;
    (void)right;
}

void mixer_channel_set_sample(size_t channel_index, size_t sample_source, size_t loop_start, size_t sample_length, soundbank_type_t soundbank_type) {
    // todo(mixer_channel_set_sample): desc: mixer_channel_set_sample()
    (void)channel_index;
    (void)sample_source;
    (void)loop_start;
    (void)sample_length;
    (void)soundbank_type;
}

void mixer_channel_key_on(uint32_t channel_bits) {
    // todo(mixer_channel_key_on): desc: mixer_channel_key_on()
    (void)channel_bits;
}

void mixer_channel_key_off(uint32_t channel_bits) {
    // todo(mixer_channel_key_off): desc: mixer_channel_key_off()
    (void)channel_bits;
}

int mixer_channel_is_idle(size_t channel_index) {
    // todo(mixer_channel_is_idle): desc: mixer_channel_is_idle()
    (void)channel_index;
    return 0;
}
