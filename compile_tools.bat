D:

cd D:/Projects/Git/obj2psx
cargo build --release
copy D:\Projects\Git\obj2psx\target\release\deps\obj2psx.exe D:\Projects\Git\ShooterPSX\tools\obj2psx.exe /Y
cd D:/Projects/Git/ShooterPSX

cd D:/Projects/Git/psx_vislist_generator
cargo build --release
copy D:\Projects\Git\psx_vislist_generator\target\release\deps\psx_vislist_generator.exe D:\Projects\Git\ShooterPSX\tools\psx_vislist_generator.exe /Y