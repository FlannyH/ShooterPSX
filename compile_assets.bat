del assets /Q
mkdir assets
mkdir assets\models
mkdir assets\music
mkdir assets\music\sequence
tools\obj2psx.exe  --input ./assets_to_build/models/level.obj --output ./assets/models/level
tools\midi2psx.exe ./assets_to_build/music/sequence/level1.mid ./assets/music/sequence/level1.dss
tools\midi2psx.exe ./assets_to_build/music/sequence/combust.mid ./assets/music/sequence/combust.dss
tools\midi2psx.exe ./assets_to_build/music/sequence/subnivis.mid ./assets/music/sequence/subnivis.dss
tools\psx_soundfont_generator.exe ./assets_to_build/music/instruments.csv ./assets/music/instr.sbk