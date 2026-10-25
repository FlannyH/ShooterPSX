# Flan Sound Bank
## File header
| Type    | Name               | Description                                                              |
| ------- | ------------------ | -------------------------------------------------------------------------|
| char[4] | file_magic         | File magic: "FSBK"                                                       |
| u32     | n_regions          | Number of regions in the instrument region table                         |
| u32     | len_sample_data    | Number of bytes in the sample data chunk                                 |
| u32     | offset_sample_data | Offset (bytes) to raw sample data chunk, relative to end of header.      |
| u32     | offset_regions     | Offset (bytes) to instrument region table, relative to end of the header |
| u32     | offset_instruments | Offset (bytes) to instrument table, relative to end of the header        |

## Region
| Type | Name          | Description                                                                                                                  |
| ---- | ------------- | ---------------------------------------------------------------------------------------------------------------------------- |
| u32  | sample_start  | Offset (bytes) into sample data chunk.                                                                                       |
| u32  | sample_length | Number of bytes in this sample. If `loop_start` is not equal to UINT32_MAX, this determines when to jump back to loop_start. |
| u32  | sample_rate   | Sample rate (Hz) at MIDI key 60 (C5)                                                                                         |
| s32  | loop_start    | Relative offset (bytes) to `sample_start`, to return to after the end of a sample.                                           |
| u32  | format        | 0 = PSX SPU-ADPCM, 1 = Signed little-endian 16-bit PCM                                                                       |
| u16  | delay         | Delay stage length in milliseconds                                                                                           |
| u16  | attack        | Attack stage length in milliseconds                                                                                          |
| u16  | hold          | Hold stage length in milliseconds                                                                                            |
| u16  | decay         | Decay stage length in milliseconds                                                                                           |
| u16  | sustain       | Sustain volume where 0 = 0.0 and 65535 = 1.0                                                                                 |
| u16  | release       | Release stage in Q8.8 volume units per second                                                                                |
| u16  | volume        | Q8.8 volume multiplier applied after the volume envelope                                                                     |
| u16  | panning       | Panning for this region, 0 = left, 127 = middle, 254 = right                                                                 |
| u8   | key_min       | Minimum MIDI key for this instrument region                                                                                  |
| u8   | key_max       | Maximum MIDI key for this instrument region                                                                                  |

## Instrument
| Type | Name                     | Description                                               |
| ---- | ------------------------ | --------------------------------------------------------- |
| u16  | region_list_start_offset | Offset to the first entry of the instrument region list   |
| u16  | n_regions                | Number of consecutive instrument regions to use           |
