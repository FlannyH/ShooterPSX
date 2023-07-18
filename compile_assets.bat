mkdir assets
mkdir assets\models
mkdir assets\music
mkdir assets\music\sequence
tools\obj2psx.exe  --input ./assets_to_build/models/level.obj --output ./assets/models/level
tools\obj2psx.exe  --input ./assets_to_build/models/level_col.obj --collision --output ./assets/models/level
tools\obj2psx.exe  --input ./assets_to_build/models/test.obj --output ./assets/models/test
tools\obj2psx.exe  --input ./assets_to_build/models/test_col.obj --collision --output ./assets/models/test
tools\midi2psx.exe ./assets_to_build/music/sequence/level1.mid ./assets/music/sequence/level1.dss
tools\midi2psx.exe ./assets_to_build/music/sequence/subnivis.mid ./assets/music/sequence/subnivis.dss
tools\midi2psx.exe ./assets_to_build/music/sequence/level3.mid ./assets/music/sequence/level3.dss
tools\psx_soundfont_generator.exe ./assets_to_build/music/instruments.csv ./assets/music/instr.sbk