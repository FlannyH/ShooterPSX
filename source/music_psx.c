#include "music.h"
#include <stdio.h>
#include <psxspu.h>
#include <psxgte.h>
#include <string.h>
#include "lut.h"
#include "fixed_point.h"
#include "vec2.h"
#include "file.h"

// Channels
spu_channel_t spu_channel[24];
midi_channel_t midi_channel[16];

// Instruments
instrument_description_t* instruments = NULL;
instrument_region_header_t* instrument_regions = NULL;

// Sequence
dyn_song_seq_header_t* curr_loaded_seq = NULL;
uint8_t* sequence_pointer = NULL;
uint8_t* loop_start = NULL;
uint8_t music_playing = 0;
int16_t wait_timer = 0;
uint16_t tempo = 0; // Tempo in BPM, in @9.3 fixed point. Values go from 0.0 to 511.875

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
	SPU_CH_ADSR1(0) = 0xC0ff;
	SPU_CH_ADSR2(0) = 0x0000;
	SPU_CH_VOL_L(0) = 0x3fff;
	SPU_CH_VOL_R(0) = 0x3fff;
	SpuSetKey(1, 1 << 0);
}

void music_load_soundbank(const char* path) {
	// Load the SBK file
	uint32_t* data;
	size_t size;
	file_read(path, &data, &size);
	
	// Validate header
	soundbank_header_t* sbk_header = (soundbank_header_t*)data;
	if (sbk_header->file_magic != MAGIC_FSBK) {
		printf("[ERROR] Error loading sound bank '%s', file header is invalid!\n", path);
        return;
	}

	// First upload all the samples to audio RAM
	uint8_t* sample_data = ((uint8_t*)(sbk_header+1)) + sbk_header->offset_sample_data;
	SpuSetTransferStartAddr(0x01000);
	SpuWrite(sample_data, sbk_header->length_sample_data);

	// Copy the instruments and regions to another part in memory
	if (instruments) free(instruments);
	if (instrument_regions) free(instrument_regions);
	uint8_t* inst_data = ((uint8_t*)(sbk_header+1)) + sbk_header->offset_instrument_descs;
	uint8_t* region_data = ((uint8_t*)(sbk_header+1)) + sbk_header->offset_instrument_regions;
	size_t inst_size = sizeof(instrument_description_t) * 256;
	size_t region_size = sizeof(instrument_description_t) * sbk_header->n_samples;
	instruments = (instrument_description_t*)malloc(inst_size);
	instrument_regions = (instrument_region_header_t*)malloc(region_size);
	memcpy(instruments, inst_data, inst_size);
	memcpy(instrument_regions, region_data, region_size);

	// Free the original file
	free(sbk_header);
}

void music_load_sequence(const char* path) {
	// Load the DSS file
	uint32_t* data;
	size_t size;
	file_read(path, &data, &size);

	// This one is now loaded
	if (((dyn_song_seq_header_t*)data)->file_magic != MAGIC_FDSS) {
		curr_loaded_seq = NULL;
		sequence_pointer = NULL;
		printf("[ERROR] Error loading dynamic music sequence '%s', file header is invalid!\n", path);
        return;
	}

	if (curr_loaded_seq) free(curr_loaded_seq);
	curr_loaded_seq = (dyn_song_seq_header_t*)data; // curr_loaded_seq now owns this memory

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
		if ((command & 0xF0) == 0x00) {
			// Get parameters
			const uint8_t key = *sequence_pointer++;

			// Find all notes in this channel with this key
			midi_channel_t* midi_chn = &midi_channel[command & 0x0F];
			for (size_t i = 0; i < 8; ++i) {
				const int8_t spu = midi_chn->active_spu_channels[i];
				if (spu != -1) {
					if (spu_channel[spu].key == key) {
						midi_chn->active_spu_channels[i] = -1;
						spu_channel[spu].state = SPU_STATE_RELEASING_NOTE;
						SpuSetKey(0, 1 << spu);
					}
				}
			}
		}

		// Play Note
		else if ((command & 0xF0) == 0x10) {
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
					scalar_t s_velocity = ((scalar_t)velocity) * ONE;
					scalar_t s_channel_volume = ((scalar_t)midi_chn->volume) * ONE;
					s_velocity = scalar_mul(s_velocity, s_channel_volume);
					vec2_t stereo_volume = {
						scalar_mul((uint32_t)lut_panning[255 - midi_chn->panning], s_velocity),
						scalar_mul((uint32_t)lut_panning[midi_chn->panning], s_velocity),
					};
					scalar_t sample_rate = scalar_mul(regions[i].sample_rate * ONE, (uint32_t)lut_note_pitch[key]);

					// Start playing the note on the SPU
					SpuSetVoiceStartAddr(spu_channel_id, getSPUAddr(0x01000 + regions[i].sample_start));
					SpuSetVoicePitch(spu_channel_id, sample_rate / 44100);
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

		// Set Channel Volume
		else if ((command & 0xF0) == 0x20) {
			midi_channel_t* midi_chn = &midi_channel[command & 0x0F];
			const uint8_t volume = *sequence_pointer++;
			midi_chn->volume = volume;
		}

		// Set Channel Panning
		else if ((command & 0xF0) == 0x30) {
			midi_channel_t* midi_chn = &midi_channel[command & 0x0F];
			const uint8_t panning = *sequence_pointer++;
			midi_chn->panning = panning;
		}

		// Set Channel Pitch
		else if ((command & 0xF0) == 0x40) {
			midi_channel_t* midi_chn = &midi_channel[command & 0x0F];
			const int16_t pitch_wheel = *(int16_t*)sequence_pointer;
			midi_chn->pitch_wheel = pitch_wheel;
			sequence_pointer += 2;
		}

		// Set Channel Instrument
		else if ((command & 0xF0) == 0x50) {
			midi_channel_t* midi_chn = &midi_channel[command & 0x0F];
			const uint8_t instrument = *sequence_pointer++;
			midi_chn->instrument = instrument;
		}

		// Set tempo
		else if ((command & 0xF0) == 0x80) {
			// Upper 4 bits of 12 bit value are stored in the command code. we need to backtrack to get there
			sequence_pointer--;

			// And then filter the command code out
			tempo = (*(uint16_t*)sequence_pointer) & 0x0FFF; 
			sequence_pointer += 2;
		}

		// Wait a number of ticks
		else if ((command >= 0xA0) && (command <= 0xBF)) {
			wait_timer += lut_wait_times[command & 0x1F];
		}

		// Set time signature, we ignore for now
		else if (command == 0xFD) {
			const uint8_t num = *sequence_pointer++;
			const uint8_t denom = *sequence_pointer++;
			(void)num;
			(void)denom;
		}

		// Set Loop Start		
		else if (command == 0xFE) {
			loop_start = sequence_pointer;
		}

		// Jump to Loop Start
		else if (command == 0xFF) {
			sequence_pointer = loop_start;
		}
	}
}