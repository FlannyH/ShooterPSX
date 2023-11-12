cd tools
cd obj2psx
cargo build --release
cd ..
cd midi2psx
cargo build --release
cd ..
cd psx_vislist_generator
cargo build --release
cd ..
cd ..
copy .\tools\obj2psx\target\release\deps\obj2psx.exe .\tools\obj2psx.exe /Y
copy .\tools\midi2psx\target\release\deps\midi2psx.exe .\tools\midi2psx.exe /Y
copy .\tools\psx_vislist_generator\target\release\deps\psx_vislist_generator.exe .\tools\psx_vislist_generator.exe /Y