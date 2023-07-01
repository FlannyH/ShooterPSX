# Flan Sound Bank
## File header
| Type    | Name                      | Description                                                                               |
| ------- | ------------------------- | ----------------------------------------------------------------------------------------- |
| char[4] | file_magic                | File magic: "FSBK"                                                                        |
| u32     | n_samples                 | Number of samples and regions (every region is linked to a unique sample)                                                                         |
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
| Type | Name              | Description                                                                       |
| ---- | ----------------- | --------------------------------------------------------------------------------- |
| u8   | key_min           | Minimum MIDI key for this instrument region                                       |
| u8   | key_max           | Maximum MIDI key for this instrument region                                       |
| u16  | sample_loop_start | Sample loop start (samples)                                                       |
| u32  | sample_start      | Offset (bytes) into sample data chunk. Can be written to SPU Sample Start Address |
| u32  | sample_rate       | Sample rate (Hz) at MIDI key 60 (C5)                                              |
| u16  | reg_adsr1         | Raw data to be written to SPU_CH_ADSR1 when enabling a note                       |
| u16  | reg_adsr2         | Raw data to be written to SPU_CH_ADSR2 when enabling a note                       |