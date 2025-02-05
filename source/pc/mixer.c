#include "music.h"
#include "memory.h"
#include <portaudio.h>
#include <string.h>

typedef struct {
    double sample_rate; // in Hz
    double sample_offset; // sample index into either `music_samples` or `sfx_samples`, depending on `type`
    double loop_start;
    float volume_left;
    float volume_right;
    uint8_t type; // 0 = music, 1 = sfx
    uint8_t is_playing;
} mixer_channel_t;

PaStream* stream = NULL;
#define SAMPLE_RATE 44100

int16_t* music_samples = NULL;
int16_t* sfx_samples = NULL;

float global_volume_left = 0.0f;
float global_volume_right = 0.0f;

mixer_channel_t mixer_channel[24];

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

    memset(mixer_channel, 0, sizeof(mixer_channel));
}

void mixer_upload_sample_data(const void* const sample_data, size_t n_bytes, soundbank_type_t soundbank_type) {
    if (soundbank_type == SOUNDBANK_TYPE_MUSIC) {
        if (music_samples) mem_free(music_samples);
        music_samples = mem_alloc(n_bytes, MEM_CAT_AUDIO);
        memcpy(music_samples, sample_data, n_bytes);
        return;
    }    
    if (soundbank_type == SOUNDBANK_TYPE_SFX) {
        if (sfx_samples) mem_free(sfx_samples);
        sfx_samples = mem_alloc(n_bytes, MEM_CAT_AUDIO);
        memcpy(sfx_samples, sample_data, n_bytes);
        return;
    }
}

void mixer_global_set_volume(scalar_t left, scalar_t right) {
    global_volume_left  = (float)left  / (float)ONE;
    global_volume_right = (float)right / (float)ONE;
}

void mixer_set_music_tempo(uint32_t raw_tempo) {
    // todo
}

void mixer_channel_set_sample_rate(size_t channel_index, scalar_t sample_rate) {
    mixer_channel[channel_index].sample_rate = (double)sample_rate / (double)ONE;
}

void mixer_channel_set_volume(size_t channel_index, scalar_t left, scalar_t right) {
    mixer_channel[channel_index].volume_left  = (float)left  / (float)ONE;
    mixer_channel[channel_index].volume_right = (float)right / (float)ONE;
}

void mixer_channel_set_sample(size_t channel_index, size_t sample_offset, soundbank_type_t soundbank_type) {
    mixer_channel[channel_index].sample_offset = (double)sample_offset;
    mixer_channel[channel_index].type = soundbank_type;
}

void mixer_channel_key_on(uint32_t channel_bits) {
    for (int i = 0; i < N_SPU_CHANNELS; ++i) {
        if (channel_bits & (1 << i) != 0) {
            mixer_channel[i].is_playing = 1;
        }
    }
}

void mixer_channel_key_off(uint32_t channel_bits) {
    for (int i = 0; i < N_SPU_CHANNELS; ++i) {
        if (channel_bits & (1 << i) != 0) {
            mixer_channel[i].is_playing = 0;
        }
    }
}

int mixer_channel_is_idle(size_t channel_index) {
    return (mixer_channel[channel_index].is_playing == 0);
}
