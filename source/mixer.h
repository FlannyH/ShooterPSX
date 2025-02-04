#ifndef MIXER_H
#define MIXER_H

#include <stdint.h>
#include "fixed_point.h"

typedef enum {
    SOUNDBANK_TYPE_MUSIC,
    SOUNDBANK_TYPE_SFX,
} soundbank_type_t;

void mixer_init(void);
void mixer_upload_sample_data(const void* const sample_data, size_t n_bytes, soundbank_type_t soundbank_type);
void mixer_global_set_volume(scalar_t left, scalar_t right);
void mixer_set_music_tempo(uint32_t raw_tempo);
void mixer_channel_set_sample_rate(size_t channel_index, scalar_t sample_rate);
void mixer_channel_set_volume(size_t channel_index, scalar_t left, scalar_t right);
void mixer_channel_set_sample(size_t channel_index, size_t sample_offset, soundbank_type_t soundbank_type);
void mixer_channel_key_on(uint32_t channel_bits);
void mixer_channel_key_off(uint32_t channel_bits);
int mixer_channel_is_idle(size_t channel_index);

#endif