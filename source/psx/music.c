#include "music.h"

#include <stdio.h>
#include <psxspu.h>
#include <string.h>

#include "lut.h"
#include "vec2.h"
#include "file.h"
#include "memory.h"
#include "fixed_point.h"

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

void music_load_soundbank(const char* path) {
	// Load the SBK file
	uint32_t* data;
	size_t size;
	file_read(path, &data, &size, 1, STACK_TEMP);
	
	// Validate header
	soundbank_header_t* sbk_header = (soundbank_header_t*)data;
	if (sbk_header->file_magic != MAGIC_FSBK) {
		printf("[ERROR] Error loading sound bank '%s', file header is invalid!\n", path);
        return;
	}

	// First upload all the samples to audio RAM
	uint32_t* sample_data = ((uint32_t*)(sbk_header+1)) + sbk_header->offset_sample_data / 4;
	SpuSetTransferStartAddr(0x01000);
	SpuWrite(sample_data, sbk_header->length_sample_data);

	// Copy the instruments and regions to another part in memory
	uint8_t* inst_data = ((uint8_t*)(sbk_header+1)) + sbk_header->offset_instrument_descs;
	uint8_t* region_data = ((uint8_t*)(sbk_header+1)) + sbk_header->offset_instrument_regions;
	size_t inst_size = sizeof(instrument_description_t) * 256;
	size_t region_size = sizeof(instrument_region_header_t) * sbk_header->n_samples;
	instruments = (instrument_description_t*)mem_stack_alloc(inst_size, STACK_MUSIC);
	instrument_regions = (instrument_region_header_t*)mem_stack_alloc(region_size, STACK_MUSIC);
	memcpy(instruments, inst_data, inst_size);
	memcpy(instrument_regions, region_data, region_size);
}

void music_load_sequence(const char* path) {
	// Load the DSS file
	uint32_t* data;
	size_t size;
	file_read(path, &data, &size, 1, STACK_MUSIC);

	// This one is now loaded
	if (((dyn_song_seq_header_t*)data)->file_magic != MAGIC_FDSS) {
		curr_loaded_seq = NULL;
		sequence_pointer = NULL;
		printf("[ERROR] Error loading dynamic music sequence '%s', file header is invalid!\n", path);
        return;
	}

	if (curr_loaded_seq) mem_free(curr_loaded_seq);
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
	PANIC_IF("misaligned data!", (((intptr_t)header_end) & 0x03) != 0);
	PANIC_IF("misaligned data!", (((intptr_t)section_table) & 0x03) != 0);
	sequence_pointer = ((uint8_t*)header_end) + curr_loaded_seq->offset_section_data + section_table[section];
	
	// Initialize variables
	music_playing = 1;
	wait_timer = 1;
	for (size_t i = 0; i < 16; ++i) {
		midi_channel[i].volume = 127;
		midi_channel[i].panning = 127;
		midi_channel[i].pitch_wheel = 0;
	}
	memset(spu_channel, 0, sizeof(spu_channel));
}

spu_stage_on_t staged_note_on_events[24] = {0};
spu_stage_off_t staged_note_off_events[24] = {0};
int n_staged_note_on_events = 0;
int n_staged_note_off_events = 0;
void music_tick(int delta_time) {
	if (music_playing == 0) {
		return;
	}

	// Handle commands
	wait_timer -= delta_time;
	n_staged_note_on_events = 0;
	n_staged_note_off_events = 0;
	while (wait_timer < 0) {
		const uint8_t command = *sequence_pointer++;

		// Release Note
		if ((command & 0xF0) == 0x00) {
			// Get parameters
			const uint8_t key = *sequence_pointer++;

			// Stage key off event
			staged_note_off_events[n_staged_note_off_events++] = (spu_stage_off_t){
				.key = key,
				.midi_channel = (command & 0x0F),
			};
		}

		// Play Note
		else if ((command & 0xF0) == 0x10) {
			// Get parameters
			const uint8_t key = *sequence_pointer++;
			const uint8_t velocity = *sequence_pointer++;

			// todo: make this cleaner
			// Quick hack to add in looping support via the drum channel
			if ((command & 0x0F) == 9) {
				// Loop start
				if (key == 0) {
					loop_start = sequence_pointer;
				}
				// Loop end
				if (key == 1) {
					sequence_pointer = loop_start;
				}
			}

			// These are the channels we'll be updating
			midi_channel_t* midi_chn = &midi_channel[command & 0x0F];

			// Find first instrument region that fits, and start playing the note
			const uint16_t n_regions = instruments[midi_chn->instrument].n_regions;
			const instrument_region_header_t* regions = &instrument_regions[instruments[midi_chn->instrument].region_start_index];
			for (size_t i = 0; i < n_regions; ++i) {
				if (key >= regions[i].key_min 
				&& key <= regions[i].key_max) {
					// Calculate sample rate and velocity
					scalar_t s_velocity = ((scalar_t)velocity) * ONE;
					scalar_t s_channel_volume = ((scalar_t)midi_chn->volume) * (ONE / 256) * regions[i].volume_multiplier;
					s_velocity = scalar_mul(s_velocity, s_channel_volume);
					int panning = ((int)midi_chn->panning + (int)regions[i].panning) / 2;
					vec2_t stereo_volume = {
						scalar_mul((uint32_t)lut_panning[255 - panning], s_velocity),
						scalar_mul((uint32_t)lut_panning[panning], s_velocity),
					};
					
					// Handle channel pitch
					int coarse_min, coarse_max, fine;
					if (midi_chn->pitch_wheel >= 0) {
						coarse_min = midi_chn->pitch_wheel / 1000;
						coarse_max = coarse_min + 1;
						fine = ((midi_chn->pitch_wheel * 256) / 1000) & 0xFF;
					} else {
						coarse_max = (midi_chn->pitch_wheel + 1) / 1000;
						coarse_min = coarse_max - 1;
						fine = ((midi_chn->pitch_wheel * 256) / 1000) & 0xFF;
					}

					// Calculate A and B for lerp
					const uint32_t sample_rate_a = ((uint32_t)regions[i].sample_rate * (uint32_t)lut_note_pitch[key + coarse_min]) >> 8;
					const uint32_t sample_rate_b = ((uint32_t)regions[i].sample_rate * (uint32_t)lut_note_pitch[key + coarse_max]) >> 8;
					uint32_t sample_rate = (uint32_t)(((sample_rate_a * (255-fine)) + (sample_rate_b * (fine)))) >> 4;
					
					// Stage a note on event
					staged_note_on_events[n_staged_note_on_events] = (spu_stage_on_t){
						.voice_start = 0x01000 + regions[i].sample_start,
						.sample_rate = sample_rate / 44100,
						.adsr1 = regions[i].reg_adsr1,
						.adsr2 = regions[i].reg_adsr2,
						.vol_l = stereo_volume.x >> 12,
						.vol_r = stereo_volume.y >> 12,
						.midi_channel = command & 0x0F,
						.key = key,
						.region = i + instruments[midi_chn->instrument].region_start_index,
						.velocity = velocity
					};
					n_staged_note_on_events++;
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

			// bleh. can't guarantee alignment so i have no choice.
			uint8_t pitch_wheel_low = *sequence_pointer++;
			uint8_t pitch_wheel_high = *sequence_pointer++;
			uint16_t pitch_wheel_temp = ((uint16_t)pitch_wheel_low) + (((uint16_t)pitch_wheel_high) << 8);
			midi_chn->pitch_wheel = *(int16_t*)&pitch_wheel_temp;
		}

		// Set Channel Instrument
		else if ((command & 0xF0) == 0x50) {
			midi_channel_t* midi_chn = &midi_channel[command & 0x0F];
			const uint8_t instrument = *sequence_pointer++;
			midi_chn->instrument = instrument;
		}

		// Set tempo
		else if ((command & 0xF0) == 0x80) {
			uint32_t value = ((((uint32_t)command) << 8) + ((int32_t)(*sequence_pointer++))) & 0xFFF; // Raw value from song data
			value = (value * 44100) / 512; // Convert from raw value to sysclock
			TIMER_RELOAD(2) = (uint16_t)value;
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

		// Unknown command 
		else {
			printf("Unknown FDSS command %02X\n", command);
		}
	}
	
	// Handle staged events
	uint32_t note_on = 0;
	uint32_t note_off = 0;
	for (int stage_i = 0; stage_i < n_staged_note_off_events; ++stage_i) {
		// Find channels that have this key and this channel
		for (int spu_i = 0; spu_i < 24; ++spu_i) {
			// If the MIDI key and channel match, kill this note
			if (staged_note_off_events[stage_i].key == spu_channel[spu_i].key
			&& staged_note_off_events[stage_i].midi_channel == spu_channel[spu_i].midi_channel) {
				note_off |= 1 << spu_i;
				spu_channel[spu_i].key = 255;
			}
		}
	}
	for (int i = 0; i < n_staged_note_on_events; ++i) {
		// Find free channel
		int channel_id = 0;
		for (channel_id = 0; channel_id < 24; ++channel_id) {
			if (SPU_CH_ADSR_VOL(channel_id) != 0) continue;
			if (note_on & (1 << channel_id)) continue;
			if (note_off & (1 << channel_id)) continue;
			break;
		}

		if (channel_id >= 24) break;

		// Trigger note on that channel
		SpuSetVoiceStartAddr(channel_id, staged_note_on_events[i].voice_start);
		SpuSetVoicePitch(channel_id, staged_note_on_events[i].sample_rate);
		SPU_CH_ADSR1(channel_id) = staged_note_on_events[i].adsr1;
		SPU_CH_ADSR2(channel_id) = staged_note_on_events[i].adsr2;
		SPU_CH_VOL_L(channel_id) = staged_note_on_events[i].vol_l;
		SPU_CH_VOL_R(channel_id) = staged_note_on_events[i].vol_r;
		note_on |= 1 << channel_id;

		// Store some metadata
		spu_channel[channel_id].key = staged_note_on_events[i].key;
		spu_channel[channel_id].midi_channel = staged_note_on_events[i].midi_channel;
		spu_channel[channel_id].region = staged_note_on_events[i].region;
		spu_channel[channel_id].velocity = staged_note_on_events[i].velocity;
	}


	//FntPrint(-1, "spu: ");
	//for (int i = 0; i < 24; ++i) FntPrint(-1, "%i", SPU_CH_ADSR_VOL(i) != 0);
	//FntPrint(-1, "\n");
	//FntPrint(-1, "on:  ");
	//for (int i = 0; i < 24; ++i) FntPrint(-1, "%i", (note_on & (1 << i)) != 0);
	//FntPrint(-1, "\n");
	//FntPrint(-1, "off: ");
	//for (int i = 0; i < 24; ++i) FntPrint(-1, "%i", (note_off & (1 << i)) != 0);
	//FntPrint(-1, "\n");

	WARN_IF("note_off and note_on staged on same channel!", (note_off & note_on) != 0);
	SpuSetKey(0, note_off);
	SpuSetKey(1, note_on);

	// Handle channel volumes
	for (size_t i = 0; i < 24; ++i) {
		// Calculate velocity
		spu_channel_t* spu_ch = &spu_channel[i];
		midi_channel_t* midi_ch = &midi_channel[spu_channel[i].midi_channel];
		if (SPU_CH_ADSR_VOL(i) == 0) continue;
		if (note_on & (1 << i)) continue;
		if (note_off & (1 << i)) continue;
		if (spu_ch->key == 255) continue;

		scalar_t s_velocity = ((scalar_t)spu_ch->velocity) * ONE;
		scalar_t s_channel_volume = ((scalar_t)midi_ch->volume) * (ONE / 256) * instrument_regions[spu_ch->region].volume_multiplier;
		s_velocity = scalar_mul(s_velocity, s_channel_volume);
		int panning = ((int)midi_ch->panning + (int)instrument_regions[spu_ch->region].panning) / 2;
		vec2_t stereo_volume = {
			scalar_mul((uint32_t)lut_panning[255 - panning], s_velocity),
			scalar_mul((uint32_t)lut_panning[panning], s_velocity),
		};
		SPU_CH_VOL_L(i) = stereo_volume.x >> 12;
		SPU_CH_VOL_R(i) = stereo_volume.y >> 12;
	}

	// Handle channel pitches
	for (size_t i = 0; i < 24; ++i) {
		spu_channel_t* spu_ch = &spu_channel[i];
		midi_channel_t* midi_ch = &midi_channel[spu_channel[i].midi_channel];

		// We only want to update channels that have been playing and aren't going to stop soon, otherwise we have timing issues.
		if (SPU_CH_ADSR_VOL(i) == 0) continue;
		if (note_on & (1 << i)) continue;
		if (note_off & (1 << i)) continue;
		if (spu_ch->key == 255) continue;
 
		// Handle channel pitch
		int coarse_min, coarse_max, fine;
		if (midi_ch->pitch_wheel >= 0) {
			coarse_min = midi_ch->pitch_wheel / 1000;
			coarse_max = coarse_min + 1;
			fine = ((midi_ch->pitch_wheel * 256) / 1000) & 0xFF;
		} else {
			coarse_max = (midi_ch->pitch_wheel + 1) / 1000;
			coarse_min = coarse_max - 1;
			fine = ((midi_ch->pitch_wheel * 256) / 1000) & 0xFF;
		}

		// Calculate A and B for lerp
		const uint32_t sample_rate_a = ((uint32_t)instrument_regions[spu_ch->region].sample_rate * (uint32_t)lut_note_pitch[spu_ch->key + coarse_min]) >> 8;
		const uint32_t sample_rate_b = ((uint32_t)instrument_regions[spu_ch->region].sample_rate * (uint32_t)lut_note_pitch[spu_ch->key + coarse_max]) >> 8;
		uint32_t sample_rate = (uint32_t)(((sample_rate_a * (255-fine)) + (sample_rate_b * (fine)))) >> 4;
		SpuSetVoicePitch(i, sample_rate / 44100);
	}
}

void music_set_volume(int volume) {
	if (volume < 0) volume = 0;
	if (volume > 255) volume = 255;
	volume = (volume * volume) / 4;
	SpuSetCommonMasterVolume(volume, volume);
}

// We get a warning from the way the PSn00bSDK made this header. We can safely ignore it
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
void music_stop(void) {
	music_playing = 0;
	SpuSetKey(0, 0xFFFFFF); // release all keys
}
#pragma GCC diagnostic pop