#include "music.h"
#include "memory.h"
#include <portaudio.h>
#include <string.h>
#include <math.h>

typedef struct {
    double sample_rate; // how much to increment `sample_offset` every audio frame
    double sample_source; // sample index into either `music_samples` or `sfx_samples`, depending on `type`
    double sample_offset; // to be added to sample_source when sampling
    float loop_start; // sample index into either `music_samples` or `sfx_samples`, depending on `type`
    float sample_length; // sample index into either `music_samples` or `sfx_samples`, depending on `type`
    float volume_left;
    float volume_right;
    uint8_t type; // 0 = music, 1 = sfx
    uint8_t is_playing;
} mixer_channel_t;

PaStream* stream = NULL;
#define MIXER_SAMPLE_RATE 44100

int16_t* music_samples = NULL;
int16_t* sfx_samples = NULL;

float global_volume_left = 0.0f;
float global_volume_right = 0.0f;

double sec_per_tick = 0.0f;

mixer_channel_t mixer_channel[N_SPU_CHANNELS];

int pa_callback(const void*, void* output_buffer, unsigned long frames_per_buffer, const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags flags, void* user_data) {
    (void)user_data;
    (void)flags;
    (void)time_info;
    
    const double sample_length = (float)1.0 / (float)MIXER_SAMPLE_RATE;
    float* output = (float*)output_buffer;

    for (size_t i = 0; i < frames_per_buffer; ++i) {
        // Tick audio
        static double counter = 0.0f;
        counter += sample_length;
        if (counter >= sec_per_tick) {
            counter -= sec_per_tick;
            audio_tick(1);
        }

        // Update mixer channels
        float vol_l = 0.0;
        float vol_r = 0.0;

        for (int channel_index = 0; channel_index < N_SPU_CHANNELS; ++channel_index) {
            mixer_channel_t* mixer_ch = &mixer_channel[channel_index];
            
            if (mixer_ch->is_playing == 0) {
                continue;
            }

            mixer_ch->sample_offset += mixer_ch->sample_rate;
            if (mixer_ch->sample_offset >= mixer_ch->sample_length) {
                if (mixer_ch->loop_start < 0.0f) {
                    mixer_ch->is_playing = 0;
                    continue;
                }
                else {
                    mixer_ch->sample_offset -= (mixer_ch->sample_length - mixer_ch->loop_start);
                }
            }

            size_t sample_index = (size_t)(mixer_ch->sample_source + mixer_ch->sample_offset);
            float sample = 0.0f;
            if (mixer_ch->type == SOUNDBANK_TYPE_MUSIC) {
                sample = ((float)music_samples[sample_index]) / INT16_MAX;
            }
            else if (mixer_ch->type == SOUNDBANK_TYPE_SFX) {
                sample = ((float)sfx_samples[sample_index]) / INT16_MAX;
            }

            // printf("s_o=%8.3f\t", mixer_ch->sample_offset);
            // printf("s_l=%8.3f\t", mixer_ch->sample_length);
            // printf("l_sta=%8.3f\n", mixer_ch->loop_start);
            // printf("l_str=%8.3f\n", mixer_ch->sample_length - mixer_ch->loop_start);
            vol_l += sample * mixer_ch->volume_left;
            vol_r += sample * mixer_ch->volume_right;
        }

        output[i * 2 + 0] = vol_l;
        output[i * 2 + 1] = vol_r;
    } 

    return paContinue;
}

void pa_stream_finished(void* user_data) {
    (void)user_data;
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
        MIXER_SAMPLE_RATE,
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
    sec_per_tick = (double)raw_tempo / 49152.0;
}

void mixer_channel_set_sample_rate(size_t channel_index, scalar_t sample_rate) {
    if (channel_index >= N_SPU_CHANNELS) {
        printf("channel_index out of bounds!\n");
        return;
    }
    mixer_channel[channel_index].sample_rate = ((double)sample_rate / (double)ONE) / MIXER_SAMPLE_RATE;
}

void mixer_channel_set_volume(size_t channel_index, scalar_t left, scalar_t right) {
    if (channel_index >= N_SPU_CHANNELS) {
        printf("channel_index out of bounds!\n");
        return;
    }
    mixer_channel[channel_index].volume_left  = (float)left  / (float)ONE;
    mixer_channel[channel_index].volume_right = (float)right / (float)ONE;
}

void mixer_channel_set_sample(size_t channel_index, size_t sample_source, size_t loop_start, size_t sample_length, soundbank_type_t soundbank_type) {
    mixer_channel[channel_index].sample_source = ((double)sample_source) / sizeof(int16_t);
    mixer_channel[channel_index].sample_offset = 0.0;
    mixer_channel[channel_index].type = soundbank_type;
    if (loop_start < 0xF0000000) {
        mixer_channel[channel_index].loop_start = (float)loop_start / sizeof(int16_t);
    }
    else {
        mixer_channel[channel_index].loop_start = -1.0f;
    }
    mixer_channel[channel_index].sample_length = (float)sample_length / sizeof(int16_t);

    printf("sh sample_offset = %8i\t", sample_source);
    printf("sh loop_start    = %8i\t", loop_start);
    printf("sh sample_length = %8i\t", sample_length);
    printf("mx sample_source = %8.3f\t", mixer_channel[channel_index].sample_source);
    printf("mx loop_start    = %8.3f\t", mixer_channel[channel_index].loop_start);
    printf("mx sample_length = %8.3f\n", mixer_channel[channel_index].sample_length);
}

void mixer_channel_key_on(uint32_t channel_bits) {
    for (int i = 0; i < N_SPU_CHANNELS; ++i) {
        if ((channel_bits & (1 << i)) != 0) {
            mixer_channel[i].is_playing = 1;
        }
    }
}

void mixer_channel_key_off(uint32_t channel_bits) {
    for (int i = 0; i < N_SPU_CHANNELS; ++i) {
        if ((channel_bits & (1 << i)) != 0) {
            mixer_channel[i].is_playing = 0;
        }
    }
}

int mixer_channel_is_idle(size_t channel_index) {
    if (channel_index >= N_SPU_CHANNELS) {
        printf("channel_index out of bounds!\n");
        return 0;
    }
    return (mixer_channel[channel_index].is_playing == 0);
}
