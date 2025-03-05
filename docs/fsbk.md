# Flan Sound Bank
## File header
| Type    | Name                      | Description                                                                       |
| ------- | ------------------------- | --------------------------------------------------------------------------------- |
| char[4] | file_magic                | File magic: "FSBK"                                                                |
| u32     | n_samples                 | Number of samples and regions (every region is linked to a unique sample)         |
| u32     | offset_instrument_descs   | Offset (bytes) to instrument description table, relative to the end of the header |
| u32     | offset_instrument_regions | Offset (bytes) to instrument region table, relative to the end of the header      |
| u32     | offset_sample_headers     | Offset (bytes) to sample header table, relative to the end of the header          |
| u32     | offset_sample_data        | Offset (bytes) to raw sample data chunk.                                          |
| u32     | len_sample_data           | Number of bytes in the sample data chunk                                          |

## Instrument Description
| Type | Name               | Description                   |
| ---- | ------------------ | ----------------------------- |
| u16  | region_start_index | First instrument region index |
| u16  | n_regions          | Number of instrument regions  |

## Instrument Region header
| Type | Name         | Description                                                  |
| ---- | ------------ | ------------------------------------------------------------ |
| u16  | sample_index | Index into sample header array                               |
| u16  | delay        | Delay stage length in milliseconds                           |
| u16  | attack       | Attack stage length in milliseconds                          |
| u16  | hold         | Hold stage length in milliseconds                            |
| u16  | decay        | Decay stage length in milliseconds                           |
| u16  | sustain      | Sustain volume where 0 = 0.0 and 65535 = 1.0                 |
| u16  | release      | Release stage in Q8.8 volume units per second                |
| u16  | volume       | Q8.8 volume multiplier applied after the volume envelope     |
| u16  | panning      | Panning for this region, 0 = left, 127 = middle, 254 = right |
| u8   | key_min      | Minimum MIDI key for this instrument region                  |
| u8   | key_max      | Maximum MIDI key for this instrument region                  |

# Sample header
| Type | Name          | Description                                                                    |
| ---- | ------------- | ------------------------------------------------------------------------------ |
| u32  | sample_start  | Offset (bytes) into sample data chunk. Can be written to SPU Sample Start Address |
| u32  | sample_length | Number of bytes in this sample. If `loop_start` is not equal to UINT32_MAX, this determines when to jump back to loop_start. |
| u32  | sample_rate   | Sample rate (Hz) at MIDI key 60 (C5) |
| u32  | loop_start    | Offset (bytes) relative to sample start to return to after the end of a sample.  |
| u32  | format        | 0 = PSX SPU-ADPCM, 1 = Signed little-endian 16-bit PCM |
