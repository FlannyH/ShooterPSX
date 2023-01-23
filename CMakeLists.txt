# PSn00bSDK example CMake script
# (C) 2021 spicyjpeg - MPL licensed

cmake_minimum_required(VERSION 3.21)

project(
	PSn00bSDK-template
	LANGUAGES    C CXX ASM
	VERSION      1.0.0
	DESCRIPTION  "PSn00bSDK template"
	HOMEPAGE_URL "http://lameguy64.net/?page=psn00bsdk"
)

file(GLOB _sources *.c *.s)
psn00bsdk_add_executable(main GPREL ${_sources})

psn00bsdk_add_cd_image(
	iso      # Target name
	ShooterPSX # Output file name (= template.bin + template.cue)
	iso.xml  # Path to config file
	DEPENDS main
)
