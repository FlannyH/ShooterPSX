mkdir assets
mkdir assets\models
mkdir assets\music
mkdir assets\music\sequence
rem levels
tools\obj2psx.exe  --input ./assets_to_build/models/level.obj --output ./assets/models/level
tools\obj2psx.exe  --input ./assets_to_build/models/level_col.obj --collision --output ./assets/models/level
tools\psx_vislist_generator.exe ./assets/models/level.msh ./assets/models/level.col ./assets/models/level.vis
tools\psx_vislist_generator.exe ./assets/models/test.msh ./assets/models/test.col ./assets/models/test.vis
tools\obj2psx.exe  --input ./assets_to_build/models/test.obj --output ./assets/models/test
tools\obj2psx.exe  --input ./assets_to_build/models/test_col.obj --collision --output ./assets/models/test
rem entities
tools\obj2psx.exe  --input ./assets_to_build/models/entity.obj --output ./assets/models/entity --page
rem music
tools\midi2psx.exe ./assets_to_build/music/sequence/level1.mid ./assets/music/sequence/level1.dss
tools\midi2psx.exe ./assets_to_build/music/sequence/subnivis.mid ./assets/music/sequence/subnivis.dss
tools\midi2psx.exe ./assets_to_build/music/sequence/level3.mid ./assets/music/sequence/level3.dss
tools\psx_soundfont_generator.exe ./assets_to_build/music/instruments.csv ./assets/music/instr.sbk