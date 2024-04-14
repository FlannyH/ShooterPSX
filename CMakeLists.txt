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
        source/camera.c
        source/collision.c
        source/fixed_point.c
        source/player.c
        source/memory.c
        source/vislist.c
        source/entity.c
        source/timer.c
        source/entities/door.c
        source/entities/pickup.c
        source/entities/crate.c
    )

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -Wall -Wno-unused-function")

    psn00bsdk_add_cd_image(
        iso      # Target name
        ShooterPSX # Output file name (= template.bin + template.cue)
        iso.xml  # Path to config file
        DEPENDS main
    )
elseif (DEFINED LEVEL_EDITOR_TARGET)
    project ("ShooterPSX")
    set(CMAKE_CXX_STANDARD 20)
    add_executable (ShooterPSX
        # New code for the editor
        source/editor/main.c

        # Reused code from the game for rendering
        source/file_windows.c 
        source/input_windows.c 
        source/renderer_windows.c 
        source/music_windows.c
        source/texture_windows.c
        source/mesh.c 
        source/camera.c
        source/collision.c
        source/fixed_point.c
        source/player.c
        source/memory.c
        source/vislist.c
        source/entity.c
        source/entities/door.c
        source/entities/pickup.c
        source/entities/crate.c
        source/win/psx.c
        source/win/debug_layer.cpp
        external/source/GL/gl3w.c
        external/include/imgui/imgui.cpp
        external/include/imgui/imgui_widgets.cpp
        external/include/imgui/imgui_tables.cpp
        external/include/imgui/imgui_draw.cpp
        external/include/imgui/imgui_impl_glfw.cpp
        external/include/imgui/imgui_impl_opengl3.cpp
        external/include/imguizmo/ImGuizmo.cpp
    )
    if(WIN32)
        target_link_libraries(
            ShooterPSX 
            ${CMAKE_CURRENT_SOURCE_DIR}/external/libraries/glfw3.lib
            ${CMAKE_CURRENT_SOURCE_DIR}/external/libraries/cimgui.lib
        )
        add_custom_command(
            TARGET ShooterPSX POST_BUILD
            COMMAND robocopy /e /xx
                    ${CMAKE_CURRENT_SOURCE_DIR}/assets/
                    ${CMAKE_CURRENT_BINARY_DIR}/assets/
                    || (exit 0)
            COMMAND robocopy /e /xx
                    ${CMAKE_CURRENT_SOURCE_DIR}/external/dll/
                    ${CMAKE_CURRENT_BINARY_DIR}/
                    || (exit 0)
        )
    else()
        target_link_libraries(
            ShooterPSX 
            ${CMAKE_CURRENT_SOURCE_DIR}/external/libraries/glfw3.a
        )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lm -lstdc++ -fsanitize=undefined -fsanitize=address ")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -fsanitize=undefined -fsanitize=address ")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined -fsanitize=address ")
    endif()
    include_directories(external/include/)
    include_directories(external/include/imgui)
    include_directories(external/include/imguizmo)
    
    add_custom_target(game_copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/ASSETS
        ${CMAKE_CURRENT_BINARY_DIR}/ASSETS/
        COMMENT "Copying asset folder to output"
    )
    add_custom_target(game_copy_dll ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/external/dll/
        ${CMAKE_CURRENT_BINARY_DIR}/
        COMMENT "Copying dll folder to output"
    )

    add_compile_definitions(_WINDOWS)
    add_compile_definitions(_LEVEL_EDITOR)
    add_compile_definitions(DEBUG_CAMERA)
else()
    project ("ShooterPSX")
    set(CMAKE_CXX_STANDARD 17)
    add_executable (ShooterPSX
        source/file_windows.c 
        source/input_windows.c 
        source/renderer_windows.c 
        source/music_windows.c
        source/texture_windows.c
        source/main.c 
        source/mesh.c 
        source/camera.c
        source/collision.c
        source/fixed_point.c
        source/player.c
        source/memory.c
        source/vislist.c
        source/entity.c
        source/entities/door.c
        source/entities/pickup.c
        source/entities/crate.c
        source/win/psx.c
        source/win/debug_layer.cpp
        external/source/GL/gl3w.c
        external/include/imgui/imgui.cpp
        external/include/imgui/imgui_widgets.cpp
        external/include/imgui/imgui_tables.cpp
        external/include/imgui/imgui_draw.cpp
        external/include/imgui/imgui_impl_glfw.cpp
        external/include/imgui/imgui_impl_opengl3.cpp
        external/include/imguizmo/ImGuizmo.cpp
    )
    if(WIN32)
        target_link_libraries(
            ShooterPSX 
            ${CMAKE_CURRENT_SOURCE_DIR}/external/libraries/glfw3.lib
            ${CMAKE_CURRENT_SOURCE_DIR}/external/libraries/cimgui.lib
        )
        add_custom_command(
            TARGET ShooterPSX POST_BUILD
            COMMAND robocopy /e /xx
                    ${CMAKE_CURRENT_SOURCE_DIR}/assets/
                    ${CMAKE_CURRENT_BINARY_DIR}/assets/
                    || (exit 0)
            COMMAND robocopy /e /xx
                    ${CMAKE_CURRENT_SOURCE_DIR}/external/dll/
                    ${CMAKE_CURRENT_BINARY_DIR}/
                    || (exit 0)
        )
    else()
        target_link_libraries(
            ShooterPSX 
            ${CMAKE_CURRENT_SOURCE_DIR}/external/libraries/glfw3.a
        )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lm -lstdc++ -fsanitize=undefined -fsanitize=address ")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -fsanitize=undefined -fsanitize=address ")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined -fsanitize=address ")
    endif()
    include_directories(external/include/)
    include_directories(external/include/imgui)
    include_directories(external/include/imguizmo)
    
    add_custom_target(game_copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/ASSETS
        ${CMAKE_CURRENT_BINARY_DIR}/ASSETS/
        COMMENT "Copying asset folder to output"
    )
    add_custom_target(game_copy_dll ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/external/dll/
        ${CMAKE_CURRENT_BINARY_DIR}/
        COMMENT "Copying dll folder to output"
    )

    add_compile_definitions(_WINDOWS)
endif()
