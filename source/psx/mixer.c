#include "mixer.h"
#include "music.h"
#include <hwregs_c.h>
#include <psxetc.h>
#include <psxapi.h>
#include <psxspu.h>

#define SBK_MUSIC_SIZE (333 * KiB)
#define SBK_MUSIC_OFFSET 0x01000
#define SBK_SFX_OFFSET (SBK_MUSIC_OFFSET + SBK_MUSIC_SIZE)

static void timer2_handler(void) {
    audio_tick(1);
}

void mixer_init(void) {
    SpuInit();

	TIMER_CTRL(2) = 
		(2 << 8) | // Use "System Clock / 8" source
		(1 << 4) | // Interrupt when hitting target value
		(1 << 6) | // Repeat this interrupt every time the timer reaches target
		(1 << 3);  // Reset counter to 0 after hitting target value
	TIMER_RELOAD(2) = 22050;

	EnterCriticalSection();
	InterruptCallback(IRQ_TIMER2, &timer2_handler);
	ExitCriticalSection();
}

void mixer_upload_sample_data(const void* const sample_data, size_t n_bytes, soundbank_type_t soundbank_type) {
	if (soundbank_type == SOUNDBANK_TYPE_MUSIC) SpuSetTransferStartAddr(SBK_MUSIC_OFFSET);
	if (soundbank_type == SOUNDBANK_TYPE_SFX)   SpuSetTransferStartAddr(SBK_SFX_OFFSET);
	SpuWrite(sample_data, n_bytes);
}

void mixer_global_set_volume(scalar_t left, scalar_t right) {
    SPU_MASTER_VOL_L = left * 4;
    SPU_MASTER_VOL_R = right * 4;
}

void mixer_set_music_tempo(uint32_t raw_tempo) {
    // Convert from raw tempo value to sysclock reload value
    TIMER_RELOAD(2) = (raw_tempo * 44100) / 512; 
}

void mixer_channel_set_sample_rate(size_t channel_index, scalar_t sample_rate) {
    SPU_CH_FREQ(channel_index) = (uint16_t)(sample_rate / 44100);
}

void mixer_channel_set_volume(size_t channel_index, scalar_t left, scalar_t right) {
    SPU_CH_VOL_L(channel_index) = left * 4;
    SPU_CH_VOL_R(channel_index) = right * 4;
}

void mixer_channel_set_sample(size_t channel_index, size_t sample_source, size_t loop_start, size_t sample_length, soundbank_type_t soundbank_type) {
    // (encoded into sample data on psx)
    (void)loop_start;
    (void)sample_length;

    uint32_t corrected_sample_offset = sample_offset;
    if (soundbank_type == SOUNDBANK_TYPE_SFX)   corrected_sample_offset += SBK_SFX_OFFSET;
    if (soundbank_type == SOUNDBANK_TYPE_MUSIC) corrected_sample_offset += SBK_MUSIC_OFFSET;

    SPU_CH_ADSR1(channel_index) = 0x00FF; // constant sustain
    SPU_CH_ADSR2(channel_index) = 0x0000;
    SPU_CH_ADDR(channel_index) = getSPUAddr(corrected_sample_offset);
}

void mixer_channel_key_on(uint32_t channel_bits) {
    SPU_KEY_ON1 = (uint16_t)(channel_bits);
    SPU_KEY_ON2 = (uint16_t)((channel_bits) >> 16);
}

void mixer_channel_key_off(uint32_t channel_bits) {
    SPU_KEY_OFF1 = (uint16_t)(channel_bits);
    SPU_KEY_OFF2 = (uint16_t)((channel_bits) >> 16);
}

int mixer_channel_is_idle(size_t channel_index) {
    return (SPU_CH_ADSR_VOL(channel_index) == 0);
}
