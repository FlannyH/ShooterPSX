#include "music.h"
#include <portaudio.h>

PaStream* stream = NULL;
#define SAMPLE_RATE 44100

void audio_load_soundbank(const char* path, soundbank_type_t type) {
    (void)path;
    (void)type;
}
int pa_callback(const void*, void* output_buffer, unsigned long frames_per_buffer, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* user_data) {
    printf("pa: %i frames\n", frames_per_buffer);
    return paContinue;
}

void pa_stream_finished(void*){
    return;
}

void audio_init(void) {
    Pa_Initialize();

    const int device_index = Pa_GetDefaultOutputDevice();
    if (device_index == paNoDevice) { 
        printf("Failed to get audio output device\n");
        return; 
    }

    const PaStreamParameters output_parameters = {
        device_index,
        2,
        paInt8,
        Pa_GetDeviceInfo(device_index)->defaultLowOutputLatency,
        NULL,
    };

    const PaDeviceInfo* device_info = Pa_GetDeviceInfo(device_index);
    if (device_info != NULL) {
        printf("Audio output device: \"%s\"\n", device_info->name);
    }

    PaError error = Pa_OpenStream(
        &stream,
        NULL,
        &output_parameters,
        SAMPLE_RATE,
        paFramesPerBufferUnspecified,
        paClipOff,
        &pa_callback,
        NULL // todo: userdata?
    );
    if (error != paNoError) {
        printf("Failed to open audio stream!\n");
        return;
    }

    //Set stream finished callback
    error = Pa_SetStreamFinishedCallback(stream, &pa_stream_finished);
    if (error != paNoError) {
        printf("Error setting up audio stream!\n");
        Pa_CloseStream(stream);
        stream = NULL;
        return;
    }
}

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