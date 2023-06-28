#include "music.h"
#include <stdio.h>
#include <psxspu.h>
#include <psxgte.h>
#include "fixed_point.h"
#include "vec2.h"
#include "file.h"

// Channels
spu_channel_t spu_channel[24];
midi_channel_t midi_channel[16];
uint16_t* pan_lut = NULL;
uint16_t* note_lut = NULL;

// Instruments
instrument_description_t* instruments = NULL;
instrument_region_header_t* instrument_regions = NULL;

// Sequence
dyn_song_seq_header_t* curr_loaded_seq = NULL;
uint8_t* sequence_pointer = NULL;
uint8_t music_playing = 0;
int16_t wait_timer = 0;

void music_test_sound() {
	// Load sound
	uint32_t* data;
	size_t size;
	file_read("\\ASSETS\\AMEN.BIN;1", &data, &size);
	SpuSetTransferStartAddr(0x01000);
	SpuWrite(data, size);

	// Play sound
    SpuSetVoiceStartAddr(0, getSPUAddr(0x01000));
	SpuSetVoicePitch(0, getSPUSampleRate(18900));
	SPU_CH_ADSR1(0) = 0x00ff;
	SPU_CH_ADSR2(0) = 0x0000;
	SPU_CH_VOL_L(0) = 0x3fff;
	SPU_CH_VOL_R(0) = 0x3fff;
	SpuSetKey(1, 1 << 0);
}

void music_load_sequence(const char* path) {
	// Load the DSS file
	uint32_t* data;
	size_t size;
	file_read(path, &data, &size);

	// This one is now loaded
	curr_loaded_seq = (dyn_song_seq_header_t*)data; // curr_loaded_seq now owns this memory
	if (curr_loaded_seq->file_magic != MAGIC_FDSS) {
		curr_loaded_seq = NULL;
		sequence_pointer = NULL;
		printf("[ERROR] Error loading dynamic music sequence '%s', file header is invalid!\n", path);
        return;
	}
}

void music_play_sequence(int section) {
	// Is the section index valid?
	if (section >= curr_loaded_seq->n_sections) {
		printf("[ERROR] Attempt to play non-existent section!");
		sequence_pointer = NULL;
		return;
	}

	// It is! Find where the data is
	uint32_t* header_end = (uint32_t*)(curr_loaded_seq + 1); // start of header + 1 whole struct's worth of bytes
	uint32_t* section_table = header_end + (curr_loaded_seq->offset_section_table / 4); // find the section table and get the entry
	sequence_pointer = ((uint8_t*)header_end) + curr_loaded_seq->offset_section_data + section_table[section];
	
	// Initialize variables
	music_playing = 1;
	wait_timer = 1;
	for (size_t i = 0; i < 16; ++i) {
		memset(midi_channel[i].active_spu_channels, -1, 8);
		midi_channel[i].volume = 127;
		midi_channel[i].panning = 127;
		midi_channel[i].pitch_wheel = 0;
	}
	memset(spu_channel, 0, sizeof(spu_channel));

	// Calculate panning lut
	if (pan_lut == NULL) {
		pan_lut = (uint16_t*)malloc(sizeof(uint16_t) * 256);
		for (size_t i = 0; i < 256; ++i) {
			pan_lut[i] = isin(i * 128);
		}
	}
}

void music_tick() {
	if (music_playing == 0) {
		return;
	}

	// Handle commands
	--wait_timer;
	while (wait_timer < 0) {
		const uint8_t command = *sequence_pointer++;

		// Release Note
		if (command & 0xF0 == 0x00) {

		}
		// Play Note
		if (command & 0xF0 == 0x10) {
			// Get parameters
			const uint8_t key = *sequence_pointer++;
			const uint8_t velocity = *sequence_pointer++;
			
			// Find free SPU channel
			size_t spu_channel_id = 0;
			for (spu_channel_id = 0; spu_channel_id < 24; ++spu_channel_id) {
				// If this one is free, cut the loop and use this index
				if (spu_channel[spu_channel_id].state == 0) {
					break;
				}
			}

			// No channel available? too bad, this note won't play
			if (spu_channel_id == 24) continue;

			// These are the channels we'll be updating
			spu_channel_t* spu_chn = &spu_channel[spu_channel_id];
			midi_channel_t* midi_chn = &midi_channel[command & 0x0F];

			// Find first instrument region that fits, and start playing the note
			const uint16_t n_regions = instruments[midi_chn->instrument].n_regions;
			const instrument_region_header_t* regions = &instrument_regions[instruments[midi_chn->instrument].region_start_index];
			for (size_t i = 0; i < n_regions; ++i) {
				if (key >= regions[i].key_min 
				&& key <= regions[i].key_max) {
					// Calculate sample rate and velocity
					scalar_t s_velocity = velocity * ONE;
					vec2_t stereo_volume = {
						scalar_mul((uint32_t)pan_lut[255 - midi_chn->panning], velocity),
						scalar_mul((uint32_t)pan_lut[midi_chn->panning], velocity),
					};
					scalar_t sample_rate = scalar_mul(regions[i].sample_rate * ONE, );

					// Start playing the note on the SPU
					SpuSetVoiceStartAddr(spu_channel_id, getSPUAddr(0x01000));
					SpuSetVoicePitch(spu_channel_id, getSPUSampleRate(18900));
					SPU_CH_ADSR1(spu_channel_id) = regions[i].reg_adsr1;
					SPU_CH_ADSR2(spu_channel_id) = regions[i].reg_adsr2;
					SPU_CH_VOL_L(spu_channel_id) = stereo_volume.x * 2;
					SPU_CH_VOL_R(spu_channel_id) = stereo_volume.y * 2;
					SpuSetKey(1, 1 << spu_channel_id);

					// Mark the channel as playing
					for (size_t i = 0; i < 8; ++i) {
						if (midi_chn->active_spu_channels[i] == -1) {
							midi_chn->active_spu_channels[i] = spu_channel_id;
						}
					}
					spu_chn->key = key;
					spu_chn->velocity = velocity;
					spu_chn->state = SPU_STATE_PLAYING_NOTE;
					spu_chn->instrument = midi_chn->instrument;
					break;
				}
			}
		}
	}
}