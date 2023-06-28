#include "music.h"
#include <stdint.h>
#include <stdio.h>
#include <psxspu.h>

dyn_song_seq_header_t* curr_loaded_seq = NULL;

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

void music_play_sequence(const char* path) {
	// Load the DSS file
	uint32_t* data;
	size_t size;
	file_read(path, &data, &size);

	// This one is now loaded
	curr_loaded_seq = (dyn_song_seq_header_t*)data;
	if (curr_loaded_seq->file_magic != MAGIC_FDSS) {
		curr_loaded_seq = NULL;       
		printf("[ERROR] Error loading dynamic music sequence '%s', file header is invalid!\n", path);
        return;
	}

	
}