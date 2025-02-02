# Flan Sound Bank
## File header
| Type    | Name                      | Description                                                                               |
| ------- | ------------------------- | ----------------------------------------------------------------------------------------- |
| char[4] | file_magic                | File magic: "FSBK"                                                                        |
| u32     | n_samples                 | Number of samples and regions (every region is linked to a unique sample)                 |
| u32     | offset_instrument_descs   | Offset (bytes) to instrument description table, relative to the end of the header         |
| u32     | offset_instrument_regions | Offset (bytes) to instrument region table, relative to the end of the header              |
| u32     | offset_sample_data        | Offset (bytes) to raw SPU-ADPCM data chunk. This will be uploaded straight to the SPU-RAM |
| u32     | len_sample_data           | Number of bytes in the SPU-ADPCM data chunk |

## Instrument Description
| Type | Name               | Description                   |
| ---- | ------------------ | ----------------------------- |
| u16  | region_start_index | First instrument region index |
| u16  | n_regions          | Number of instrument regions  |

## Instrument Region header
| Type | Name         | Description                                                  |
| ---- | ------------ | ------------------------------------------------------------ |
| u32  | sample_start | Offset (bytes) into sample data chunk.                       |
| u32  | sample_rate  | Sample rate (Hz) at MIDI key 60 (C5)                         |
| u16  | delay        | Delay stage length in milliseconds                           |
| u16  | attack       | Attack stage length in milliseconds                          |
| u16  | hold         | Hold stage length in milliseconds                            |
| u16  | decay        | Decay stage length in milliseconds                           |
| u16  | sustain      | Sustain volume where 0 = 0.0 and 65535 = 1.0                 |
| u16  | release      | Release stage length in milliseconds                         |
| u16  | volume       | Q8.8 volume multiplier applied after the volume envelope     |
| u16  | panning      | Panning for this region, 0 = left, 127 = middle, 254 = right |
| u8   | key_min      | Minimum MIDI key for this instrument region                  |
| u8   | key_max      | Maximum MIDI key for this instrument region                  |
