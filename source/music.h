#ifndef MUSIC_H
#define MUSIC_H

#include <stdint.h>

// Sound bank header
#define MAGIC_FSBK 0x4B425346
typedef struct {
    uint32_t file_magic;                // File magic: "FSBK"
    uint32_t n_samples;                 // Number of samples
    uint32_t offset_instrument_descs;   // Offset (bytes) to instrument description table, relative to the end of the header
    uint32_t offset_instrument_regions; // Offset (bytes) to instrument region table, relative to the end of the header
    uint32_t offset_sample_data;        // Offset (bytes) to raw SPU-ADPCM data chunk
    uint32_t length_sample_data;        // Number of bytes in the SPU-ADPCM data chunk
} soundbank_header_t;

// Instrument description
typedef struct {
    uint16_t region_start_index; // First instrument region index
    uint16_t n_regions;          // Number of instrument regions
} instrument_description_t;

// Instrument Region header
typedef struct {
    uint8_t key_min;            // Minimum MIDI key for this instrument region
    uint8_t key_max;            // Maximum MIDI key for this instrument region
    uint16_t volume_multiplier; // @8.8 fixed point volume multiplier
    uint32_t sample_start;      // Offset (bytes) into sample data chunk. Can be written to SPU Sample Start Address
    uint32_t sample_rate;       // Sample rate (Hz) at MIDI key 60 (C5)
    uint16_t reg_adsr1;         // Raw data to be written to SPU_CH_ADSR1 when enabling a note
    uint16_t reg_adsr2;         // Raw data to be written to SPU_CH_ADSR2 when enabling a note
    uint8_t panning;            // Panning for this region, 0 = left, 127 = middle, 254 = right
    uint8_t pad[3];
} instrument_region_header_t;

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

typedef struct {
    uint8_t key;          // currently playing key
    uint8_t velocity;     // currently playing velocity
    uint8_t region;       // currently playing region
    uint8_t midi_channel; // the midi channel that spawned this channel
} spu_channel_t;

#define SPU_STAGE_ON 1
#define SPU_STAGE_OFF 0
typedef struct {
    uint32_t voice_start;
    uint16_t sample_rate;
    uint16_t adsr1;
    uint16_t adsr2;
    uint16_t vol_l;
    uint16_t vol_r;
    uint8_t key;
    uint8_t midi_channel;
    uint8_t region;
    uint8_t velocity;
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

// Common
void audio_tick(int delta_time);
void audio_load_soundbank(const char* path, soundbank_type_t type);

// Music
void music_test_sound(void);
void music_test_instr_region(int region);
void music_load_sequence(const char* path);
void music_play_sequence(uint32_t section);
void music_set_volume(int volume);
void music_stop(void);

// Sound effects

#endif
