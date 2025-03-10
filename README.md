# Sub Nivis PS1
This is a work in progress PlayStation 1 port of Sub Nivis, a game I worked on in college.

https://github.com/FlannyH/ShooterPSX/assets/60531875/5cb85d24-7a6c-44a8-9a6b-af4fc6346165

## Building the project
### Windows & Linux
1. Install Rust compiler, a C/C++ compiler like `gcc`, and install `make`
2. Open command line and type `make pc`
4. Navigate to folder `build/pc/` and open SubNivis executable

### PlayStation 1
1. Install Rust compiler and install `make`
2. [Install PSn00bSDK](https://github.com/Lameguy64/PSn00bSDK/blob/master/doc/installation.md) and make sure you do set `PSN00BSDK_LIBS` environment variable to the `psn00bsdk/lib/libpsn00b/` folder, as described in the PSn00bSDK docs.
3. Rip the license data file from any PS1 game you own, and save it to the repository's root folder as `license_data.dat`
4. Open command line and type `make psx`
5. Navigate to folder `build/psx/` and either burn the SubNivis.cue file to a CD, or open it in an emulator

### Nintendo DS (experimental)
1. Install Rust compiler and `make`
2. [Install BlocksDS](https://blocksds.github.io/docs/setup/options/windows/). Either keep the default paths, or manually set an environment variable `GCC_ARM_NONE_EABI_PATH` to the `gcc-arm-none-eabi/` folder and a `BLOCKSDS` environment variable to `blocksds/core/`
3. Open command line and type `make nds`
4. Navigate to folder `build/nds/` and either copy the SubNivis.nds file to a flash cart or run it in an emulator
