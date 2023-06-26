#ifndef MUSIC_H
#define MUSIC_H

// File header
typedef struct {
    char file_magic[4];               // File magic: "FSBK"
    uint32_t n_samples;               // Number of samples
    uint32_t offset_instrument_descs; // Offset (bytes) to instrument description table, relative to the end of the header
    uint32_t offset_instrument_regions; // Offset (bytes) to instrument region table, relative to the end of the header
    uint32_t offset_sample_data;      // Offset (bytes) to raw SPU-ADPCM data chunk
} FlanSoundBankHeader;

// Instrument Description
typedef struct {
    uint16_t region_start_index; // First instrument region index
    uint16_t n_regions;          // Number of instrument regions
} InstrumentDescription;

// Instrument Region header
typedef struct {
    uint8_t key_min;               // Minimum MIDI key for this instrument region
    uint8_t key_max;               // Maximum MIDI key for this instrument region
    uint16_t sample_loop_start;    // Sample loop start (samples)
    uint32_t sample_start;        // Offset (bytes) into sample data chunk. Can be written to SPU Sample Start Address
    uint32_t sample_rate;          // Sample rate (Hz) at MIDI key 60 (C5)
    uint16_t reg_adsr1;            // Raw data to be written to SPU_CH_ADSR1 when enabling a note
    uint16_t reg_adsr2;            // Raw data to be written to SPU_CH_ADSR2 when enabling a note
} InstrumentRegionHeader;

void music_test_sound();


#endif