# Flan Dynamic Song Sequence
## File header
| Type    | Name                      | Description                                                            |
| ------- | ------------------------- | ---------------------------------------------------------------------- |
| char[4] | file_magic                | File magic: "FDSS"                                                     |
| u32     | n_sections                | Number of sections in the song                                         |
| u32     | offset_section_table      | Offset (bytes) to song section offset table data, relative to the end of the header |
| u32     | offset_section_data       | Offset (bytes) to song section data, relative to the end of the header |

## Section Table Entry
| Type    | Name                      | Description                                                            |
| ------- | ------------------------- | ---------------------------------------------------------------------- |
| u32     | offset                    | Offset (bytes) to the start of a section, relative to section data start                                                     |

## Sequence Data Format
### Channel Command overview
| Command Code | Description            | Operands                              | Example |
| ------------ | ---------------------- | ------------------------------------- |------------------------------------------------------------------------------------------ |
| \$0x          | Release Note           | key (u8)                              | \$03 \$3C: Channel 3 release key 60                                                        |
| \$1x          | Play Note              | key (u8), velocity (u8), | \$13 \$3C \$3F: Channel 3 play key 60 at velocity 63 (~50%)   |
| \$2x          | Set Channel Volume     | volume (u8)                           | \$23 \$3F: Channel 3 set volume to 50%                                                     |
| \$3x          | Set Channel Panning    | panning (u8)                          | \$33 \$00 \$34 \$FF: Channel 3 set panning to left, Channel 4 set panning to right           |
| \$4x          | Set Channel Pitch      | pitch (i16)                           | \$43 \$F4 \$01: Channel 3 set pitch to +100 cents                                           |
| \$5x          | Set Channel Instrument | instrument index (u8)                 | \$53 \$06: Channel 3 set instrument to index 6                                             |
| \$6x          | Reserved               |                                       |                                                                                          |
| \$7x          | Reserved               |                                       |                                                                                          |

### Meta Command overview
| Command Code  | Description            | Operands                                                  | Example / Notes
| ------------- | ---------------------- | --------------------------------------------------------- | ------------------------------------
| \$8x          | Set Tempo              | tempo (u12) | \$83 \$C0: Set tick length. To convert this value to microseconds per tick, divide it by 49152.
| \$9x          | Reserved               |
| \$A0 - $BF    | Wait a number of ticks | (none) | Actual number of ticks to wait is fetched from a look-up table, see detailed description.
| \$Bx          | Reserved               |
| \$Cx          | Reserved               |
| \$Dx          | Reserved               |
| \$Ex          | Reserved               |
| \$FD          | Set Time Signature     | numerator (u8), denominator (u8) | \$FD \$0B \$08: Set time signature to 11/8
| \$FE          | Set Loop Start         | (none) | $FF: Set loop offset to current position in file
| \$FF          | Jump to Loop Start     | (none) | $FE: Jump to section loop offset

### Detailed descriptions
#### ($0x) - Release Note
Any SPU channel that is currently playing this key from this channel, the note gets released, eventually freeing up the channel. This does not take the channel instrument into account, only the key. This avoids perpetually playing samples after a channel instrument change.
| Parameter | Raw range | Unit range             | Examples
| --------- | --------- | ---------------------- | -------
| Key       | 0-127     | C0 - B10               | C5 = 60

#### ($1x) - Play Note
The driver will look for a free sampler channel, and start a note with the specific MIDI key and velocity.
| Parameter | Raw range | Unit range             | Examples
| --------- | --------- | ---------------------- | -------
| Key       | 0-127     | C0 - B10               | C5 = 60
| Velocity  | 0-255     | 0% - 200%              | 0% = 0, 100% = 127, 200% = 254

#### ($2x) - Set Channel Volume
The channel's volume level is set to the volume operand's value. Any new notes, as well as any currently playing notes, immediately get updated to fit the new channel volume, allowing for effects like volume fades.
| Parameter | Raw range | Unit range             | Examples
| --------- | --------- | ---------------------- | -------
| Velocity  | 0-255     | 0% - 200%              | 0% = 0, 100% = 127, 200% = 254

#### ($3x) - Set Channel Panning
The channel's panning is set to the panning operand's value. Any new notes, as well as any currently playing notes, immediately get updated to fit the new channel panning, allowing for effects like stereo fades
| Parameter | Raw range | Unit range             | Examples
| --------- | --------- | ---------------------- | -------
| Panning   | 0-255     | 100% left - 100% right | Left = 0, Center = 127, Right = 254

#### ($4x) - Set Channel Pitch
The channel's pitch wheel is set to the pitch operand's value. Any new notes, as well as any currently playing notes, immediately get updated to fit the new pitch value, allowing for effects like pitch bends, slides, vibrato, or a very basic chorus effect when combined with other channels.
| Parameter | Raw range       | Unit range                     | Examples
| --------- | --------------- | ------------------------------ | -------
| Pitch     | -32768 - +32767 | -3276.8 cents to +3276.7 cents | 100 cents = 1000

#### ($5x) - Set Channel Instrument
The channel's instrument is set to the instrument index operand's value. Any subsequent notes played on this channel will have the newly set instrument, but notes that were already playing use the same instrument.
| Parameter | Raw range       | Unit range                     | Examples
| --------- | --------------- | ------------------------------ | -------
| Instrument Index | 0 - 255 | Instrument 0 to 255 | Instrument 0 = 0

#### ($8x) - Set Tempo
Sets the new tick length. This is a 12-bit value (0 - 4095). To get the tick length in microseconds, divide this value by 49152. This value was chosen to have zero error for common tempos, namely: 30, 32, 40, 48, 60, 64, 80, 96, 120, 128, 160, 192, 240, and 256 BPM, and less than 1% error in any tempo inbetween.
The bit layout for this command is as follows:
```
1000TTTT TTTTTTTT
|   |
|   Bits 11-0: Tempo
Bits 15-12: Command code
```
| Parameter | Raw range       | Unit range                     | Examples
| --------- | --------------- | ------------------------------ | -------
| Tempo     | 0 - 4095        | 0 sec - 0.0833 sec             | 120 BPM (48 PPQ) = 512, 60 BPM (48 PPQ) = 1024

#### (\$A0 - \$BF) - Wait a Number of Ticks
Wait a number of ticks between the previous command and the next command.
The bit layout for this command is as follows:
```
101XXXXX
|  |
|  Bits 4-0: Index into tick count look-up table
Bits 7-5: Command code
```
The lookup table for the number of ticks is defined as follows:
```
1,      2,      3,      4,      6,      8,      12,     16,
20,     24,     28,     32,     40,     48,     56,     64,
80,     96,     112,    128,    160,    192,    224,    256,
320,    384,    448,    512,    640,    768,    896,    1024,
```

#### ($FD) - Set Time Signature
Set or update the current time signature to nominator / denominator. 
The bit layout for this command is as follows:
```
11111101 NNNNNNNN DDDDDDDD
|        |        |
|        |        Bits 7-0: Denominator
|        Bits 15-8: Nominator
Bits 23-16: Command code
```
| Parameter   | Raw range      | Unit range                     | Examples
| ----------- | -------------- | ------------------------------ | -------
| Nominator   | 0 - 255        | 0 - 255          | 11/8 -> 11
| Denominator | 0 - 255        | 0 - 255          | 11/8 -> 8

#### ($FE) - Set Loop Start
Mark the current position in the sequence data as the loop point. This command does not have any parameters

#### ($FF) - Jump to Loop Start
Jump to the loop point set with the ($FE) Set Loop Start command