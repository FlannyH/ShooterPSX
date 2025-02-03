#include "music.h"

void audio_load_soundbank(const char* path, soundbank_type_t type) {
    (void)path;
    (void)type;
}

void audio_init(void) {}

void audio_play_sound(int instrument, int pitch_wheel, int in_3d_space, vec3_t position, scalar_t max_distance) {
    (void)instrument;
    (void)pitch_wheel;
    (void)in_3d_space;
    (void)position;
    (void)max_distance;
}

void music_load_sequence(const char* path) {
    (void)path;
}

void music_play_sequence(uint32_t section) {
    (void)section;
}

void audio_tick(int delta_time) {
    (void)delta_time;
}

void music_set_volume(int volume) {
    (void)volume;
}

void music_stop(void) {}

void audio_update_listener(const vec3_t position, const vec3_t right) {
    (void)position;
    (void)right;
}