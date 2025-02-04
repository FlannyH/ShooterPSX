#include "music.h"
#include <portaudio.h>

PaStream* stream = NULL;
#define SAMPLE_RATE 44100

int pa_callback(const void*, void* output_buffer, unsigned long frames_per_buffer, const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags flags, void* user_data) {
    const double sample_length = (float)1.0 / (float)SAMPLE_RATE;
    float* output = (float*)output_buffer;

    for (int i = 0; i < frames_per_buffer; ++i) {
        // output[i] = something;
    } 

    return paContinue;
}

void pa_stream_finished(void* user_data) {
    return;
}

void mixer_init(void) {
    Pa_Initialize();

    const int device_index = Pa_GetDefaultOutputDevice();
    if (device_index == paNoDevice) { 
        printf("Failed to get audio output device\n");
        return; 
    }

    const PaStreamParameters output_parameters = {
        device_index,
        2,
        paFloat32,
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
    if (error != paNoError || stream == NULL) {
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

    error = Pa_StartStream(stream);
    if (error != paNoError) {
        printf("Error setting up audio stream!\n");
        Pa_CloseStream(stream);
        stream = NULL;
        return;
    }
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
