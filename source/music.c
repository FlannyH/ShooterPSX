#include "music.h"

#include "fixed_point.h"
#include "memory.h"
#include "mixer.h"
#include "file.h"
#include "vec2.h"
#include "lut.h"

#include <stdio.h>
#include <string.h>

// Sample allocation
#define MAX_STAGED_NOTE_EVENTS (N_SPU_CHANNELS)
spu_stage_on_t staged_note_on_events[MAX_STAGED_NOTE_EVENTS] = {0};
spu_stage_off_t staged_note_off_events[MAX_STAGED_NOTE_EVENTS] = {0};
int n_staged_note_on_events = 0;
int n_staged_note_off_events = 0;

// Channels
#define N_MIDI_CHANNELS 16
spu_channel_t spu_channel[N_SPU_CHANNELS];
midi_channel_t midi_channel[N_MIDI_CHANNELS];
volume_env_t vol_envs[N_SPU_CHANNELS];
vec3_t sfx_origins[N_SPU_CHANNELS];
scalar_t sfx_max_distances[N_SPU_CHANNELS];

// Instruments
sample_header_t* music_sample_headers = NULL;
instrument_description_t* music_instruments = NULL;
instrument_region_header_t* music_instrument_regions = NULL;
size_t n_music_instrument_regions = 0;
sample_header_t* sfx_sample_headers = NULL;
instrument_description_t* sfx_instruments = NULL;
instrument_region_header_t* sfx_instrument_regions = NULL;
size_t n_sfx_instrument_regions = 0;

// Sequence
size_t music_stack_seq_offset = 0;
dyn_song_seq_header_t* curr_loaded_seq = NULL;
uint8_t* sequence_pointer = NULL;
uint8_t* loop_start = NULL;
uint8_t music_playing = 0;
uint8_t audio_ticking = 0;
int16_t wait_timer = 0;

vec3_t listener_pos;
vec3_t listener_forward;
vec3_t listener_right;

scalar_t ms_per_tick = 0;
scalar_t ms_precision_counter = 0;

void audio_load_soundbank(const char* path, soundbank_type_t type) {
	// Load the SBK file
	uint32_t* data;
	size_t size;
	file_read(path, &data, &size, 1, STACK_TEMP);
	
	// Validate header
	const soundbank_header_t* sbk_header = (soundbank_header_t*)data;
	if (sbk_header->file_magic != MAGIC_FSBK) {
		printf("[ERROR] Error loading sound bank '%s', file header is invalid!\n", path);
        return;
	}

	// First upload all the samples to audio RAM
	const uint32_t* sample_data = ((uint32_t*)(sbk_header+1)) + sbk_header->offset_sample_data / 4;
    mixer_upload_sample_data(sample_data, sbk_header->length_sample_data, type);

	// Copy the instruments and regions to another part in memory
	const uint8_t* sample_header_data = ((uint8_t*)(sbk_header+1)) + sbk_header->offset_sample_headers;
	const uint8_t* inst_data = ((uint8_t*)(sbk_header+1)) + sbk_header->offset_instrument_descs;
	const uint8_t* region_data = ((uint8_t*)(sbk_header+1)) + sbk_header->offset_instrument_regions;
	const size_t inst_size = sizeof(instrument_description_t) * 256;
	const size_t sample_header_size = sizeof(sample_header_t) * sbk_header->n_samples;
	const size_t region_size = sizeof(instrument_region_header_t) * sbk_header->n_samples;
	if (type == SOUNDBANK_TYPE_MUSIC) {
		music_sample_headers = (sample_header_t*)mem_stack_alloc(sample_header_size, STACK_MUSIC);
		music_instruments = (instrument_description_t*)mem_stack_alloc(inst_size, STACK_MUSIC);
		music_instrument_regions = (instrument_region_header_t*)mem_stack_alloc(region_size, STACK_MUSIC);
		n_music_instrument_regions = sbk_header->n_samples;
		memcpy(music_sample_headers, sample_header_data, sample_header_size);
		memcpy(music_instruments, inst_data, inst_size);
		memcpy(music_instrument_regions, region_data, region_size);
	}
	else {
		sfx_sample_headers = (sample_header_t*)mem_stack_alloc(sample_header_size, STACK_MUSIC);
		sfx_instruments = (instrument_description_t*)mem_stack_alloc(inst_size, STACK_MUSIC);
		sfx_instrument_regions = (instrument_region_header_t*)mem_stack_alloc(region_size, STACK_MUSIC);
		n_sfx_instrument_regions = sbk_header->n_samples;
		memcpy(sfx_sample_headers, sample_header_data, sample_header_size);
		memcpy(sfx_instruments, inst_data, inst_size);
		memcpy(sfx_instrument_regions, region_data, region_size);
	}
	music_stack_seq_offset = mem_stack_get_marker(STACK_MUSIC);
}

void audio_init(void) {
	mixer_init();
	listener_pos = (vec3_t){0, 0, 0};
	listener_right = (vec3_t){ONE, 0, 0};
}

uint32_t calculate_channel_pitch(uint32_t base_sample_rate, int key, int pitch_wheel) {
	// Handle channel pitch
	int coarse_min, coarse_max;
	int64_t fine;
	if (pitch_wheel >= 0) {
		coarse_min = pitch_wheel / 1000;
		coarse_max = coarse_min + 1;
		fine = ((pitch_wheel * 256) / 1000) & 0xFF;
	} 
	else {
		coarse_max = (pitch_wheel + 1) / 1000;
		coarse_min = coarse_max - 1;
		fine = ((pitch_wheel * 256) / 1000) & 0xFF;
	}

	// Calculate A and B for lerp
	const uint64_t sample_rate_a = ((uint64_t)base_sample_rate * (uint64_t)lut_note_pitch[key + coarse_min]) >> 8;
	const uint64_t sample_rate_b = ((uint64_t)base_sample_rate * (uint64_t)lut_note_pitch[key + coarse_max]) >> 8;
	return (uint32_t)(((sample_rate_a * (255-fine)) + (sample_rate_b * (fine))) >> 4);
} 

void audio_stage_on(int instrument, int key, int pan, int velocity, int pitch_wheel, int midi_channel, vec3_t position, scalar_t max_distance) {
	if (sfx_instrument_regions == NULL || music_instrument_regions == NULL) return;
	if (sfx_instruments == NULL || music_instruments == NULL) return;
	
	PANIC_IF("note panning out of bounds!", pan < 0 || pan > 255);

	// Find first instrument region that fits, and start playing the note
	const int is_sfx = (midi_channel >= N_MIDI_CHANNELS);
	const instrument_description_t* const instr = (is_sfx) ? &sfx_instruments[instrument] : &music_instruments[instrument];
	const uint16_t n_regions = instr->n_regions;
	const uint16_t region_id_start = instr->region_start_index;

	if (is_sfx) PANIC_IF("region id will go out of bounds!", (region_id_start >= n_sfx_instrument_regions) || ((region_id_start + n_regions) > n_sfx_instrument_regions));
	else        PANIC_IF("region id will go out of bounds!", (region_id_start >= n_music_instrument_regions) || ((region_id_start + n_regions) > n_music_instrument_regions));
	const instrument_region_header_t* regions = (is_sfx) ? &sfx_instrument_regions[region_id_start] : &music_instrument_regions[region_id_start];
	const sample_header_t* samples = (is_sfx) ? &sfx_sample_headers[0] : &music_sample_headers[0];

	for (size_t i = 0; i < n_regions; ++i) {
		if (key >= regions[i].key_min 
		&& key <= regions[i].key_max) {
			const uint16_t sample_index = regions[i].sample_index;

			// Stage a note on event					
			staged_note_on_events[n_staged_note_on_events] = (spu_stage_on_t){
				.voice_start = samples[sample_index].sample_start,
				.sample_rate = calculate_channel_pitch(samples[sample_index].sample_rate, key, pitch_wheel),
				.loop_start = samples[sample_index].loop_start,
				.sample_length = samples[sample_index].sample_length,
				.midi_channel = midi_channel,
				.key = key,
				.region = i + instr->region_start_index,
				.velocity = velocity,
				.position = position,
				.max_distance = max_distance,
                .soundbank_type = (is_sfx) ? (SOUNDBANK_TYPE_SFX) : (SOUNDBANK_TYPE_MUSIC)
			};
			n_staged_note_on_events++;
		}
	}
}

void audio_play_sound(int instrument, int pitch_wheel, int in_3d_space, vec3_t position, scalar_t max_distance) {
	const int key = 60;
	int velocity = 127;
	int pan = 127;
	audio_stage_on(instrument, key, pan, velocity, pitch_wheel, in_3d_space ? MIDI_CHANNEL_SFX_3D : MIDI_CHANNEL_SFX_2D, position, max_distance);
};

void music_load_sequence(const char* path) {
	mem_stack_reset_to_marker(STACK_MUSIC, music_stack_seq_offset);

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

	curr_loaded_seq = (dyn_song_seq_header_t*)data;
}

void music_play_sequence(uint32_t section) {
	if (curr_loaded_seq == NULL) {
		printf("[ERROR] Attempt to play with no sequence loaded!");
		sequence_pointer = NULL;
		return;
	}

	// Is the section index valid?
	if (section >= curr_loaded_seq->n_sections) {
		printf("[ERROR] Attempt to play non-existent section!");
		sequence_pointer = NULL;
		return;
	}

	// It is! Find where the data is
	const uint32_t* header_end = (uint32_t*)(curr_loaded_seq + 1); // start of header + 1 whole struct's worth of bytes
	const uint32_t* section_table = header_end + (curr_loaded_seq->offset_section_table / 4); // find the section table and get the entry
	PANIC_IF("misaligned music data!", (((intptr_t)header_end) & 0x03) != 0);
	PANIC_IF("misaligned music section table!", (((intptr_t)section_table) & 0x03) != 0);
	sequence_pointer = ((uint8_t*)header_end) + curr_loaded_seq->offset_section_data + section_table[section];
	
	// Initialize variables
	music_playing = 1;
	wait_timer = 1;
	for (size_t i = 0; i < N_MIDI_CHANNELS; ++i) {
		midi_channel[i].volume = 127;
		midi_channel[i].panning = 127;
		midi_channel[i].pitch_wheel = 0;
	}
	memset(spu_channel, 0, sizeof(spu_channel));
	memset(vol_envs, 0, sizeof(vol_envs));

	n_staged_note_on_events = 0;
	n_staged_note_off_events = 0;
}

void audio_tick(int delta_time) {
	audio_ticking = 1;
	if (music_playing != 0) {
		// Handle commands
		wait_timer -= delta_time;
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

				// Bounds check so our note queue doesn't overflow and write out of bounds
				if (n_staged_note_off_events >= MAX_STAGED_NOTE_EVENTS) {
					wait_timer++;
				}
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
				audio_stage_on(midi_chn->instrument, key, midi_chn->panning, velocity, midi_chn->pitch_wheel, command & 0x0F, vec3_from_scalar(0), 0);
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
				const uint8_t pitch_wheel_low = *sequence_pointer++;
				const uint8_t pitch_wheel_high = *sequence_pointer++;
				const uint16_t pitch_wheel_temp = ((uint16_t)pitch_wheel_low) + (((uint16_t)pitch_wheel_high) << 8);
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
				mixer_set_music_tempo(value);
				ms_per_tick = (value * ONE * 1000) / 49152;
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
	}
	
	// Handle staged events
	uint32_t note_on = 0;
	uint32_t note_off = 0;
	uint32_t note_cut = 0;
	for (int stage_i = 0; stage_i < n_staged_note_off_events; ++stage_i) {
		// Find channels that have this key and this channel
		for (int spu_i = 0; spu_i < N_SPU_CHANNELS; ++spu_i) {
			// If the MIDI key and channel match, kill this note
			if (staged_note_off_events[stage_i].key == spu_channel[spu_i].key
			&& staged_note_off_events[stage_i].midi_channel == spu_channel[spu_i].midi_channel) {
				note_off |= 1 << spu_i;
				spu_channel[spu_i].key = 255;
			}
		}
	}

	for (int i = 0; i < n_staged_note_on_events; ++i) {
		const spu_stage_on_t* const stage_on = &staged_note_on_events[i];

		// Find free channel
		int channel_id = -1;
		int last_resort = -1;
		int max_release_stage_time = -1;
		int max_decay_stage_time = -1;
		for (int j = 0; j < N_SPU_CHANNELS; ++j) {
			if (note_on & (1 << j)) continue;
			if (note_off & (1 << j)) continue;
			if (vol_envs[j].stage == ENV_STAGE_IDLE || mixer_channel_is_idle(j)) {
				channel_id = j;
				break;
			}
            // todo: maybe select based on volume, since an envelope in decay or release at like 2% volume you wont hear that
			if (vol_envs[j].stage == ENV_STAGE_RELEASE && vol_envs[j].stage_time > max_release_stage_time) {
				max_release_stage_time = vol_envs[j].stage_time;
				channel_id = j;
				continue;
			}
			if (vol_envs[j].stage == ENV_STAGE_DECAY && vol_envs[j].stage_time > max_decay_stage_time) {
				last_resort = j;
			}
		}

		if (channel_id < 0 && last_resort >= 0) channel_id = last_resort;
		if (channel_id < 0) break;
		if (channel_id >= N_SPU_CHANNELS) break;

		// Trigger note on that channel
		mixer_channel_set_sample(channel_id, stage_on->voice_start, stage_on->loop_start, stage_on->sample_length, stage_on->soundbank_type);
		mixer_channel_set_sample_rate(channel_id, stage_on->sample_rate);
		note_on |= 1 << channel_id;

		// Store some metadata
		spu_channel[channel_id].key = staged_note_on_events[i].key;
		spu_channel[channel_id].midi_channel = staged_note_on_events[i].midi_channel;
		spu_channel[channel_id].region = staged_note_on_events[i].region;
		spu_channel[channel_id].velocity = staged_note_on_events[i].velocity;
		sfx_origins[channel_id] = staged_note_on_events[i].position;
		sfx_max_distances[channel_id] = staged_note_on_events[i].max_distance;
		vol_envs[channel_id].stage_time = 0;
		vol_envs[channel_id].stage = ENV_STAGE_DELAY;
	}

	// Update channels
	ms_precision_counter += ms_per_tick * delta_time;
	int delta_stage_time = ms_precision_counter / ONE;
	ms_precision_counter -= delta_stage_time * ONE;

	for (int i = 0; i < N_SPU_CHANNELS; ++i) {
		const spu_channel_t* spu_ch = &spu_channel[i];
		instrument_region_header_t* region = NULL;

		scalar_t channel_volume = 127;
		scalar_t channel_panning = 127;
		if (spu_ch->midi_channel < N_MIDI_CHANNELS) {
			const midi_channel_t* midi_ch = &midi_channel[spu_ch->midi_channel];
			channel_volume = midi_ch->volume;
			channel_panning = midi_ch->panning;
			region = &music_instrument_regions[spu_ch->region];
		}
		else {
			region = &sfx_instrument_regions[spu_ch->region];
		}

		// This code may run before the soundbanks are loaded, so if the instrument region is null, skip this channel
		if (region == NULL) break;
		
		// Update volume envelope
		vol_envs[i].stage_time += delta_stage_time;

		if (vol_envs[i].stage == ENV_STAGE_IDLE) {
			vol_envs[i].adsr_volume = 0;
		}

		else if (note_off & (1 << i)) {
			vol_envs[i].stage = ENV_STAGE_RELEASE;
		}

		if (vol_envs[i].stage == ENV_STAGE_DELAY) {
			vol_envs[i].adsr_volume = 0;
			if (vol_envs[i].stage_time >= region->delay) {
				vol_envs[i].stage_time -= region->delay;
				vol_envs[i].stage = ENV_STAGE_ATTACK;
			} 
		}
		if (vol_envs[i].stage == ENV_STAGE_ATTACK) {
			if (region->attack == 0) {
				vol_envs[i].stage = ENV_STAGE_HOLD;
			}
			else {
				vol_envs[i].adsr_volume = scalar_lerp(0, ADSR_VOLUME_ONE, (vol_envs[i].stage_time * ONE) / region->attack);
				if (vol_envs[i].stage_time >= region->attack) {
					vol_envs[i].stage_time -= region->attack;
					vol_envs[i].stage = ENV_STAGE_HOLD;
				}
			}
		}
		if (vol_envs[i].stage == ENV_STAGE_HOLD) {
			vol_envs[i].adsr_volume = ADSR_VOLUME_ONE;
			if (vol_envs[i].stage_time >= region->hold) {
				vol_envs[i].stage_time -= region->hold;
				vol_envs[i].stage = ENV_STAGE_DECAY;
			} 
		}
		if (vol_envs[i].stage == ENV_STAGE_DECAY) {
			if (region->decay == 0) {
				vol_envs[i].stage = ENV_STAGE_SUSTAIN;
			}
			else {
				vol_envs[i].adsr_volume = scalar_lerp(ADSR_VOLUME_ONE, region->sustain, (vol_envs[i].stage_time * ONE) / region->decay);
				if (vol_envs[i].stage_time >= region->decay) {
					vol_envs[i].stage_time -= region->decay;
					vol_envs[i].stage = ENV_STAGE_SUSTAIN;
				} 
			}
		}
		if (vol_envs[i].stage == ENV_STAGE_SUSTAIN) {
			vol_envs[i].adsr_volume = region->sustain * (ADSR_VOLUME_ONE / ADSR_SUSTAIN_MAX);
			if (region->sustain == 0) {
				vol_envs[i].stage_time = 0;
				vol_envs[i].stage = ENV_STAGE_RELEASE;
			} 
		}
		if (vol_envs[i].stage == ENV_STAGE_RELEASE) {
			if (region->release == 0) {
				vol_envs[i].stage = ENV_STAGE_IDLE;
				note_cut |= (1 << i);
				vol_envs[i].adsr_volume = 0;
			}
			else {
				const uint32_t n_ticks_from_one_to_zero = 256000 / region->release;
				vol_envs[i].adsr_volume -= (INT32_MAX / n_ticks_from_one_to_zero) * delta_stage_time;

				if (vol_envs[i].adsr_volume <= 0) {
					vol_envs[i].stage = ENV_STAGE_IDLE;
					note_cut |= (1 << i);
					vol_envs[i].adsr_volume = 0;
				}
			}
		}
		if (vol_envs[i].stage >= N_ENV_STAGES) {
			vol_envs[i].stage = ENV_STAGE_IDLE;
		}

		// Volume 
		scalar_t s_velocity = (((scalar_t)spu_ch->velocity) * ONE) / 127;
		const scalar_t s_channel_volume = (channel_volume * region->volume) / 8;
		s_velocity = scalar_mul(s_velocity, s_channel_volume);
		s_velocity = scalar_mul(s_velocity, vol_envs[i].adsr_volume / (ADSR_VOLUME_ONE / ONE));
		s_velocity = scalar_mul(s_velocity, s_velocity);

		PANIC_IF("note panning out of bounds!", channel_panning < 0 || channel_panning > 255);
		PANIC_IF("region panning out of bounds!", (region->panning < 0) || (region->panning > 255));

		const vec2_t stereo_volume = vec2_mul(
			(vec2_t){
				scalar_mul((uint32_t)lut_panning[254 - channel_panning], s_velocity),
				scalar_mul((uint32_t)lut_panning[channel_panning], s_velocity),
			},
			(vec2_t){
				(uint32_t)lut_panning[254 - region->panning],
				(uint32_t)lut_panning[region->panning],
			}
		);

		mixer_channel_set_volume(i, stereo_volume.x, stereo_volume.y);

		// Handle channel pitch
		if (spu_ch->key < 128 && spu_ch->midi_channel < N_MIDI_CHANNELS && vol_envs[i].stage != ENV_STAGE_IDLE) {
			const uint16_t sample_index = music_instrument_regions[spu_ch->region].sample_index;
			const uint32_t inst_sample_rate = music_sample_headers[sample_index].sample_rate;
			const midi_channel_t* midi_ch = &midi_channel[spu_ch->midi_channel];
			mixer_channel_set_sample_rate(i, calculate_channel_pitch(inst_sample_rate, spu_ch->key, midi_ch->pitch_wheel));
		}
	}

	WARN_IF("note_off and note_on staged on same channel!", (note_off & note_on) != 0);
	mixer_channel_key_off(note_cut);
	mixer_channel_key_on(note_on);

	n_staged_note_on_events = 0;
	n_staged_note_off_events = 0;
	audio_ticking = 0;
}

void music_set_volume(int volume) {
	if (volume < 0) volume = 0;
	if (volume > 255) volume = 255;
	volume = (volume * volume) / (65536/ONE);
	mixer_global_set_volume(volume, volume);
}

void music_stop(void) {
	music_playing = 0;
	mixer_channel_key_off(0xFFFFFF); // release all keys
	while (audio_ticking) {}
}

void audio_update_listener(const vec3_t position, const vec3_t right) {
	listener_pos = position;
	listener_right = right;
}
