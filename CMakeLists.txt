# PSn00bSDK example CMake script
# (C) 2021 spicyjpeg - MPL licensed

cmake_minimum_required(VERSION 3.21)

if (DEFINED PSN00BSDK_TARGET) # PS1 (game)
    project(
        PSn00bSDK-template
        LANGUAGES    C CXX ASM
        VERSION      1.0.0
        DESCRIPTION  "PSn00bSDK template"
        HOMEPAGE_URL "http://lameguy64.net/?page=psn00bsdk"
    )
    include_directories(source/)
    add_compile_definitions(_PSX)
    
    psn00bsdk_add_executable(main GPREL 
        source/psx/file.c 
        source/psx/input.c 
        source/psx/renderer.c 
        source/psx/music.c
        source/psx/texture.c
        source/psx/timer.c
        source/main.c 
        source/mesh.c 
        source/camera.c
        source/collision.c
        source/player.c
        source/memory.c
        source/vislist.c
        source/entity.c
        source/level.c
        source/entities/door.c
        source/entities/pickup.c
        source/entities/crate.c
        source/entities/chaser.c
    )

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -Wall -Wno-unused-function -fanalyzer")

    psn00bsdk_add_cd_image(
        iso      # Target name
        ShooterPSX # Output file name (= template.bin + template.cue)
        iso.xml  # Path to config file
        DEPENDS main
    )
elseif (DEFINED NDS_TARGET) # Nintendo DS (game)
    set(CMAKE_CXX_STANDARD 20)

    project ("ShooterNDS")
    add_compile_definitions(_NDS)
    include_directories(source/)
    add_executable (ShooterNDS
        # source/nds/file.c 
        # source/nds/input.c 
        # source/nds/renderer.c 
        # source/nds/music.c
        # source/nds/texture.c
        source/nds/psx.c
        source/main.c 
        source/mesh.c 
        source/camera.c
        source/collision.c
        source/player.c
        source/memory.c
        source/vislist.c
        source/entity.c
        source/level.c
        source/entities/door.c
        source/entities/pickup.c
        source/entities/crate.c
        source/entities/chaser.c
    )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-function -fanalyzer")

elseif (DEFINED LEVEL_EDITOR_TARGET) # Windows (level editor)
    project ("ShooterPSX")
    set(CMAKE_CXX_STANDARD 20)
    add_executable (ShooterPSX
        # New code for the editor
        source/editor/main.c

        # Reused code from the game for rendering
        source/windows/file.c 
        source/windows/input.c 
        source/windows/renderer.c 
        source/windows/music.c
        source/windows/texture.c
        source/mesh.c 
        source/camera.c
        source/collision.c
        source/player.c
        source/memory.c
        source/vislist.c
        source/level.c
        source/entity.c
        source/entities/door.c
        source/entities/pickup.c
        source/entities/crate.c
        source/entities/chaser.c
        source/windows/psx.c
        source/windows/debug_layer.cpp
        external/source/GL/gl3w.c
        external/include/imgui/imgui.cpp
        external/include/imgui/imgui_widgets.cpp
        external/include/imgui/imgui_tables.cpp
        external/include/imgui/imgui_draw.cpp
        external/include/imgui/backends/imgui_impl_glfw.cpp
        external/include/imgui/backends/imgui_impl_opengl3.cpp
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
    include_directories(source/)
    include_directories(external/include/)
    include_directories(external/include/imgui)
    include_directories(external/include/imguizmo)
    include_directories(external/include/imfilebrowser)
    
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
else() # Windows (game)
    project ("ShooterPSX")
    set(CMAKE_CXX_STANDARD 20)
    add_executable (ShooterPSX
        source/windows/file.c 
        source/windows/input.c 
        source/windows/renderer.c 
        source/windows/music.c
        source/windows/texture.c
        source/main.c 
        source/mesh.c 
        source/camera.c
        source/collision.c
        source/player.c
        source/memory.c
        source/vislist.c
        source/level.c
        source/entity.c
        source/entities/door.c
        source/entities/pickup.c
        source/entities/crate.c
        source/entities/chaser.c
        source/windows/psx.c
        source/windows/debug_layer.cpp
        external/source/GL/gl3w.c
        external/include/imgui/imgui.cpp
        external/include/imgui/imgui_widgets.cpp
        external/include/imgui/imgui_tables.cpp
        external/include/imgui/imgui_draw.cpp
        external/include/imgui/backends/imgui_impl_glfw.cpp
        external/include/imgui/backends/imgui_impl_opengl3.cpp
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
    include_directories(source/)
    include_directories(external/include/)
    include_directories(external/include/imgui)
    include_directories(external/include/imguizmo)
    include_directories(external/include/imfilebrowser)
    
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
