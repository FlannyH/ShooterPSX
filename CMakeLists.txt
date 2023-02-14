# PSn00bSDK example CMake script
# (C) 2021 spicyjpeg - MPL licensed

cmake_minimum_required(VERSION 3.21)

if (DEFINED PSN00BSDK_TARGET)
    project(
	    PSn00bSDK-template
	    LANGUAGES    C CXX ASM
	    VERSION      1.0.0
	    DESCRIPTION  "PSn00bSDK template"
	    HOMEPAGE_URL "http://lameguy64.net/?page=psn00bsdk"
    )
    add_compile_definitions(_PSX)
    
    psn00bsdk_add_executable(main GPREL 
        source/file_psx.c 
        source/input_psx.c 
        source/renderer_psx.c 
        source/music_psx.c
        source/texture_psx.c
        source/main.c 
        source/mesh.c 
        source/camera.c)

    psn00bsdk_add_cd_image(
	    iso      # Target name
	    ShooterPSX # Output file name (= template.bin + template.cue)
	    iso.xml  # Path to config file
	    DEPENDS main
    )
else()
    project ("ShooterPSX")
    add_executable (ShooterPSX
        source/file_windows.c 
        source/input_windows.c 
        source/renderer_windows.c 
        source/music_windows.c
        source/texture_windows.c
        source/main.c 
        source/mesh.c 
        source/camera.c
        source/win/psx.c
        external/source/GL/gl3w.c
    )
    target_link_libraries(ShooterPSX ${CMAKE_CURRENT_SOURCE_DIR}/external/libraries/glfw3.lib)
    include_directories(external/include/)

    add_custom_command(
        TARGET ShooterPSX POST_BUILD
        COMMAND robocopy /MIR
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/
                ${CMAKE_CURRENT_BINARY_DIR}/assets/
    )
    add_compile_definitions(_WINDOWS)
endif()
