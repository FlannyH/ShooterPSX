#include "music.h"

#include "common.h"

void music_test_sound(void) {
    TODO_SOFT()
}

void music_test_instr_region(int region) {
    (void)region;
    TODO_SOFT()
}

void audio_load_soundbank(const char* path, soundbank_type_t type) {
    (void)path;
    (void)type;
    TODO_SOFT()
}

void audio_init(void) {}

void audio_play_sound(int instrument, scalar_t pitch_multiplier, int in_3d_space, vec3_t position, scalar_t max_distance) {
    (void)instrument;
    (void)pitch_multiplier;
    (void)in_3d_space;
    (void)position;
    (void)max_distance;
    TODO_SOFT()
}

void music_load_sequence(const char* path) {
    (void)path;
    TODO_SOFT()
}

void music_play_sequence(uint32_t section) {
    (void)section;
    TODO_SOFT()
}

void audio_tick(int delta_time) {
    (void)delta_time;
    TODO_SOFT()
}

void music_set_volume(int volume) {
    (void)volume;
    TODO_SOFT()
}

void music_stop(void) {
    TODO_SOFT()
}

void audio_update_listener(const vec3_t position, const vec3_t right) {
    (void)position;
    (void)right;
    TODO_SOFT()
}