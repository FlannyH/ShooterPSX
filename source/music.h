#ifndef MUSIC_H
#define MUSIC_H

#include "vec3.h"

#include <stdint.h>

// Sound bank header
#define MAGIC_FSBK 0x4B425346
typedef struct {
    uint32_t file_magic;                // File magic: "FSBK"
    uint32_t n_samples;                 // Number of samples and regions (every region is linked to a unique sample)
    uint32_t offset_instrument_descs;   // Offset (bytes) to instrument description table, relative to the end of the header
    uint32_t offset_instrument_regions; // Offset (bytes) to instrument region table, relative to the end of the header
    uint32_t offset_sample_headers;     // Offset (bytes) to sample header table, relative to the end of the header
    uint32_t offset_sample_data;        // Offset (bytes) to raw sample data chunk.
    uint32_t length_sample_data;        // Number of bytes in the sample data chunk
} soundbank_header_t;

// Instrument description
typedef struct {
    uint16_t region_start_index; // First instrument region index
    uint16_t n_regions;          // Number of instrument regions
} instrument_description_t;

// Instrument Region header
typedef struct {
    uint16_t sample_index;  // Index into sample header array
    uint16_t delay;         // Delay stage length in milliseconds                                                |
    uint16_t attack;        // Attack stage length in milliseconds                                               |
    uint16_t hold;          // Hold stage length in milliseconds                                                 |
    uint16_t decay;         // Decay stage length in milliseconds                                                |
    uint16_t sustain;       // Sustain volume where 0 = 0.0 and 65535 = 1.0                                      |
    uint16_t release;       // Release stage length in milliseconds                                              |
    uint16_t volume;        // Q8.8 volume multiplier applied after the volume envelope                      |
    uint16_t panning;       // Panning for this region, 0 = left, 127 = middle, 254 = right                      |
    uint8_t key_min;        // Minimum MIDI key for this instrument region                                       |
    uint8_t key_max;        // Maximum MIDI key for this instrument region         
} instrument_region_header_t;

typedef struct {
    uint32_t sample_start;  // Offset (bytes) into sample data chunk. Can be written to SPU Sample Start Address
    uint32_t sample_rate;   // Sample rate (Hz) at MIDI key 60 (C5)
    uint32_t loop_start;    // Offset (bytes) relative to sample start to return to after the end of a sample
    uint16_t format;        // 0 = PSX SPU-ADPCM, 1 = Signed little-endian 16-bit PCM
    uint16_t reserved;      // (unused padding for now)
} sample_header_t;

// Dynamic Song Sequence header
#define MAGIC_FDSS 0x53534446
typedef struct {
    uint32_t file_magic;           // File magic: "FDSS"
    uint32_t n_sections;           // Number of sections in the song
    uint32_t offset_section_table; // Offset (bytes) to song section offset table data, relative to the end of the header
    uint32_t offset_section_data;  // Offset (bytes) to song section data, relative to the end of the header
} dyn_song_seq_header_t;

#define SPU_STATE_IDLE 0
#define SPU_STATE_SFX 1
#define SPU_STATE_PLAYING_NOTE 2
#define SPU_STATE_RELEASING_NOTE 3

#define MIDI_CHANNEL_SFX_2D 255
#define MIDI_CHANNEL_SFX_3D 254

typedef struct {
    uint8_t key;          // currently playing key
    uint8_t velocity;     // currently playing velocity
    uint8_t region;       // currently playing region
    uint8_t midi_channel; // the midi channel that spawned this channel
} spu_channel_t;

typedef struct {
    uint16_t stage_time; // Time in milliseconds how long we've been at this envelope stage
    uint16_t stage; // What stage we're at now
} volume_env_t;

#define SPU_STAGE_ON 1
#define SPU_STAGE_OFF 0
typedef struct {
    vec3_t position; // only if midi_channel == MIDI_CHANNEL_SFX_*
    scalar_t max_distance; // only if midi_channel == MIDI_CHANNEL_SFX_*
    uint32_t voice_start;
    uint16_t sample_rate;
    uint8_t key;
    uint8_t midi_channel;
    uint8_t region;
    uint8_t velocity;
    uint8_t panning;
} spu_stage_on_t;

typedef struct {
    uint8_t midi_channel;
    uint8_t key;
} spu_stage_off_t;

typedef struct {
    uint8_t volume;      // channel volume. 0 = 0%, 127 = 100%, 254 = 200%
    uint8_t panning;     // channel panning, 0 is left, 254 is right
    uint8_t instrument;  // channel instrument
    int16_t pitch_wheel; // channel pitch, 10 = 1 cent
} midi_channel_t;

typedef enum {
    SOUNDBANK_TYPE_MUSIC,
    SOUNDBANK_TYPE_SFX,
} soundbank_type_t;

typedef enum {
    ENV_STAGE_IDLE,
    ENV_STAGE_DELAY,
    ENV_STAGE_ATTACK,
    ENV_STAGE_HOLD,
    ENV_STAGE_DECAY,
    ENV_STAGE_SUSTAIN,
    ENV_STAGE_RELEASE,
    N_ENV_STAGES,
} env_stage_t;

typedef enum {
    sfx_ammo,
    sfx_armor_big,
    sfx_bullet_impact_concrete,
    sfx_door_close,
    sfx_door_open,
    sfx_door_unlock,
    sfx_footstep1,
    sfx_footstep2,
    sfx_footstep3,
    sfx_footstep4,
    sfx_footstep5,
    sfx_footstep6,
    sfx_footstep7,
    sfx_generic,
    sfx_gun,
    sfx_health_large,
    sfx_health_small,
    sfx_jump_land2,
    sfx_jump_land1,
    sfx_key,
    sfx_player_damage_01,
    sfx_player_damage_02,
    sfx_player_death,
    sfx_powerup,
    sfx_rev_alt_shot_01,
    sfx_rev_empty,
    sfx_rev_shot_1,
    sfx_rev_shot_2,
    sfx_rev_shot_3,
    sfx_rev_shot_4,
    sfx_shotgun_pickup,
} sfx_instrument_t;

// Common
void audio_init(void);
void audio_tick(int delta_time);
void audio_load_soundbank(const char* path, soundbank_type_t type);
void audio_play_sound(int instrument, int pitch_wheel, int in_3d_space, vec3_t position, scalar_t max_distance);

// Music
void music_load_sequence(const char* path);
void music_play_sequence(uint32_t section);
void music_set_volume(int volume);
void music_stop(void);

// Sound effects
void audio_update_listener(const vec3_t position, const vec3_t right);

#endif
