# PSn00bSDK example CMake script
# (C) 2021 spicyjpeg - MPL licensed

cmake_minimum_required(VERSION 3.21)

if (DEFINED WINDOWS_TARGET)
    option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
    option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
    option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
    add_subdirectory(external/glfw)
    add_subdirectory(external/gl3w)
    add_subdirectory(external/cglm)
endif()

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
        source/nds/file.c 
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
    message("${CMAKE_CURRENT_BINARY_DIR}/external/include")
    project (ShooterPSX)
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

        # External libraries
        external/gl3w/src/gl3w.c
        external/imgui/imgui.cpp
        external/imgui/imgui_widgets.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/backends/imgui_impl_glfw.cpp
        external/imgui/backends/imgui_impl_opengl3.cpp
        external/imguizmo/ImGuizmo.cpp
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} fsanitize=address")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} fsanitize=address")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
    include_directories(source/)
    include_directories(external/glfw/include)
    include_directories(external/gl3w/include)
    include_directories(external/cglm/include)
    include_directories(external/imgui)
    target_link_libraries(ShooterPSX glfw)
    add_custom_target(game_copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different 
        ${CMAKE_CURRENT_SOURCE_DIR}/ASSETS
        ${CMAKE_CURRENT_BINARY_DIR}/ASSETS/
        COMMENT "Copying asset folder to output"
    )
    add_dependencies(ShooterPSX game_copy_assets)
    add_compile_definitions(_WINDOWS)
    add_compile_definitions(_LEVEL_EDITOR)
    add_compile_definitions(DEBUG_CAMERA)
else() # Windows (game)
    message("${CMAKE_CURRENT_BINARY_DIR}/external/include")
    project (ShooterPSX)
    set(CMAKE_CXX_STANDARD 20)
    add_executable (ShooterPSX
        # Sub Nivis
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

        # External libraries
        external/gl3w/src/gl3w.c
        external/imgui/imgui.cpp
        external/imgui/imgui_widgets.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/backends/imgui_impl_glfw.cpp
        external/imgui/backends/imgui_impl_opengl3.cpp
        external/imguizmo/ImGuizmo.cpp
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpedantic")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
    include_directories(source/)
    include_directories(external/glfw/include)
    include_directories(external/gl3w/include)
    include_directories(external/cglm/include)
    include_directories(external/imgui)
    target_link_libraries(ShooterPSX glfw)
    add_custom_target(game_copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different 
        ${CMAKE_CURRENT_SOURCE_DIR}/ASSETS
        ${CMAKE_CURRENT_BINARY_DIR}/ASSETS/
        COMMENT "Copying asset folder to output"
    )
    add_dependencies(ShooterPSX game_copy_assets)
    add_compile_definitions(_WINDOWS)
endif()
