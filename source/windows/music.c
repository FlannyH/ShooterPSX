#include "music.h"

void music_test_sound(void) {}

void music_test_instr_region(int region) {
    (void)region;
}

void audio_load_soundbank(const char* path, soundbank_type_t type) {
    (void)path;
}

void audio_play_sound(int instrument) {
    (void)instrument;
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
