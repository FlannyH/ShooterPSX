#include "music.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psxetc.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <psxapi.h>
#include <psxpad.h>
#include <psxsio.h>
#include <psxspu.h>
#include <psxcd.h>

CdlFILE xa_file;
CdlFILTER xa_filter;
CdlLOC xa_location;
char xa_sector_buffer[2048];
volatile int num_loops=0;
volatile int xa_play_channel;

typedef struct SECTOR_HEAD
{
	uint16_t id;
	uint16_t chan;
	uint8_t  pad[28];
} SECTOR_HEAD;

void xa_callback(CdlIntrResult intr, unsigned char *result)
{
	SECTOR_HEAD *sec;

	// We want this callback to respond to data ready events
	if (intr == CdlDataReady)
	{
		// Get data sector
		CdGetSector(&xa_sector_buffer, 512);
		/* Check if sector belongs to the currently playing channel */
		sec = (SECTOR_HEAD*)xa_sector_buffer;
		
		if( sec->id == 352 )
		{
			// Debug
			//printf("ID=%d CHAN=%d PL=%d\n", sec->id, (sec->chan>>10)&0xF, xa_play_channel);
		
			/* Check if sector is of the currently playing channel */
			if( ((sec->chan>>10)&0xF) == xa_play_channel ) 
			{
				num_loops++;
			
				/* Retry playback by seeking to start of XA data and stream */
				CdControlF(CdlReadS, &xa_location);
			
				/* Stop playback */
				//CdControlF(CdlPause, 0);
			}
		}
	}
}

void music_play_file(const char* path) {
    // Find the XA file
    if (!CdSearchFile(&xa_file, path)) {
        printf("Can't find file %s!", path);
        return;
    }

    // Get the location on disc
    int xa_pos = CdPosToInt(&xa_file.pos);
    printf("XA located at sector %d size %d.\n", xa_pos, xa_file.size);
    xa_location = xa_file.pos;

    // Hook XA callback function into CdReadCallback
	EnterCriticalSection();
	CdReadyCallback(xa_callback);
	ExitCriticalSection();

    // Set flags for CD for XA streaming
	int flags = CdlModeSpeed|CdlModeRT|CdlModeSF;
	CdControl(CdlSetmode, &flags, 0);

    // Select file 1
	xa_filter.file = 1;
    xa_filter.chan = 1;
    CdControl(CdlSetfilter, &xa_filter, 0);
    CdControl(CdlReadS, &xa_location, 0);
    printf("start playing xa file allegedly\n");
    xa_play_channel = 0;
}