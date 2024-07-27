# Flags
VERBOSE = 0

# Input folders
SOURCE = source
ASSETS = assets

# Output folders
PATH_TEMP = temp
PATH_BUILD = build

PATH_TEMP_PSX = $(PATH_TEMP)/psx
PATH_TEMP_WIN = $(PATH_TEMP)/windows
PATH_BUILD_PSX = $(PATH_BUILD)/psx
PATH_BUILD_WIN = $(PATH_BUILD)/windows
COMPILED_LIB_OUTPUT_WIN = $(PATH_TEMP_WIN)/lib

# Include directories
INCLUDE_DIRS = source \
			   external/cglm/include \
			   external/gl3w/include \
			   external/glfw/include \
			   external/imgui \
			   external/imguizmo \
			   external/imgui-filebrowser \
			   $(COMPILED_LIB_OUTPUT_WIN)/gl3w/include
INCLUDE_FLAGS = $(patsubst %, -I%, $(INCLUDE_DIRS))

# Source files shared by all targets
CODE_ENGINE_SHARED_C = camera.c \
			  	  	   collision.c \
			  	  	   entity.c \
			  	  	   level.c \
			  	  	   memory.c \
			  	  	   mesh.c \
			  	  	   player.c \
			  	  	   vislist.c \
				  	   entities/chaser.c \
				  	   entities/crate.c \
				  	   entities/door.c \
				  	   entities/pickup.c 

# Source files specific to PSX
CODE_ENGINE_PSX_C = psx/file.c \
				    psx/input.c \
				    psx/music.c \
				    psx/renderer.c \
				    psx/texture.c \
				    psx/timer.c 

# Source files specific to Windows
CODE_ENGINE_WIN_C = windows/file.c \
				    windows/input.c \
				    windows/music.c \
				    windows/psx.c \
				    windows/renderer.c \
				    windows/texture.c 

CODE_ENGINE_WIN_CPP = windows/debug_layer.cpp

# Where the object files go
PATH_OBJ_PSX = $(PATH_TEMP_PSX)/obj
PATH_OBJ_WIN = $(PATH_TEMP_WIN)/obj

# Misc source file definitions
CODE_GAME_MAIN = main.c
CODE_LEVEL_EDITOR = editor/main.c

# Create code sets and object sets
CODE_PSX_C				= $(CODE_ENGINE_SHARED_C)  		$(CODE_ENGINE_PSX_C) 	$(CODE_GAME_MAIN)
CODE_PSX_CPP			= $(CODE_ENGINE_SHARED_CPP) 	
CODE_WIN_C				= $(CODE_ENGINE_SHARED_C)  		$(CODE_ENGINE_WIN_C) 	$(CODE_GAME_MAIN)
CODE_WIN_CPP			= $(CODE_ENGINE_SHARED_CPP) 	$(CODE_ENGINE_WIN_CPP)
CODE_LEVEL_EDITOR_C		= $(CODE_ENGINE_SHARED_C)  		$(CODE_ENGINE_WIN_C) 	$(CODE_LEVEL_EDITOR) 
CODE_LEVEL_EDITOR_CPP	= $(CODE_ENGINE_SHARED_CPP) 	$(CODE_ENGINE_WIN_CPP)

OBJ_PSX					= 	$(patsubst %.c, 	$(PATH_OBJ_PSX)/%.o,	$(CODE_PSX_C))				\
							$(patsubst %.cpp, 	$(PATH_OBJ_PSX)/%.o,	$(CODE_PSX_CPP))				
OBJ_WIN					= 	$(patsubst %.c, 	$(PATH_OBJ_WIN)/%.o,	$(CODE_WIN_C))				\
							$(patsubst %.cpp, 	$(PATH_OBJ_WIN)/%.o,	$(CODE_WIN_CPP))				
OBJ_LEVEL_EDITOR		= 	$(patsubst %.c, 	$(PATH_OBJ_WIN)/%.o, 	$(CODE_LEVEL_EDITOR_C))		\
							$(patsubst %.cpp, 	$(PATH_OBJ_WIN)/%.o, 	$(CODE_LEVEL_EDITOR_CPP))		

# Compiler flags
CFLAGS = -Wall -Wextra -std=c11
CXXFLAGS = -Wall -Wextra -std=c++20

# Windows target
windows: DEFINES = _WINDOWS _WIN32
windows: LIBRARIES = glfw3 stdc++ gdi32 opengl32
windows: CC = gcc
windows: CXX = g++
windows: CFLAGS = $(patsubst %, -D%, $(DEFINES))
windows: CXXFLAGS = $(patsubst %, -D%, $(DEFINES)) -std=c++20
windows: LINKER_FLAGS = $(patsubst %, -l%, $(LIBRARIES)) $(patsubst %, -L%, $(COMPILED_LIB_OUTPUT_WIN)) -std=c++20
level_editor: DEFINES = _WINDOWS _WIN32 _LEVEL_EDITOR
level_editor: LIBRARIES = glfw3 stdc++ gdi32 opengl32
level_editor: CC = gcc
level_editor: CXX = g++
level_editor: CFLAGS = $(patsubst %, -D%, $(DEFINES)) 
level_editor: CXXFLAGS = $(patsubst %, -D%, $(DEFINES)) -std=c++20
level_editor: LINKER_FLAGS = $(patsubst %, -l%, $(LIBRARIES)) $(patsubst %, -L%, $(COMPILED_LIB_OUTPUT_WIN)) -std=c++20

mkdir_output_win:
	mkdir -p $(PATH_TEMP_WIN)
	mkdir -p $(PATH_OBJ_WIN)
	mkdir -p $(PATH_OBJ_WIN)/entities
	mkdir -p $(PATH_OBJ_WIN)/windows

windows_dependencies: glfw gl3w imgui imguizmo

glfw:
	mkdir -p $(COMPILED_LIB_OUTPUT_WIN)/glfw
	@cmake -S external/glfw -B $(COMPILED_LIB_OUTPUT_WIN)/glfw -G "MSYS Makefiles"
	make -C $(COMPILED_LIB_OUTPUT_WIN)/glfw glfw
	cp $(COMPILED_LIB_OUTPUT_WIN)/glfw/src/libglfw3.a $(COMPILED_LIB_OUTPUT_WIN)

OBJ_WIN += $(COMPILED_LIB_OUTPUT_WIN)/gl3w.o
gl3w:
	mkdir -p $(COMPILED_LIB_OUTPUT_WIN)/gl3w
	@cmake -S external/gl3w -B $(COMPILED_LIB_OUTPUT_WIN)/gl3w -G "MSYS Makefiles"
	make -C $(COMPILED_LIB_OUTPUT_WIN)/gl3w 
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -c $(COMPILED_LIB_OUTPUT_WIN)/gl3w/src/gl3w.c -o $(COMPILED_LIB_OUTPUT_WIN)/gl3w.o

# Define the base directories
IMGUI_SRC_DIR = external/imgui
IMGUI_OBJ_DIR = $(COMPILED_LIB_OUTPUT_WIN)/imgui
IMGUI_BACKENDS_OBJ_DIR = $(IMGUI_OBJ_DIR)/backends

# List of source files
IMGUI_SRC = \
    $(IMGUI_SRC_DIR)/imgui.cpp \
    $(IMGUI_SRC_DIR)/imgui_widgets.cpp \
    $(IMGUI_SRC_DIR)/imgui_tables.cpp \
    $(IMGUI_SRC_DIR)/imgui_draw.cpp \
    $(IMGUI_SRC_DIR)/backends/imgui_impl_glfw.cpp \
    $(IMGUI_SRC_DIR)/backends/imgui_impl_opengl3.cpp

# Corresponding object files
OBJ_IMGUI = $(IMGUI_SRC:$(IMGUI_SRC_DIR)/%.cpp=$(IMGUI_OBJ_DIR)/%.o)

# Pattern rule for building object files
$(IMGUI_OBJ_DIR)/%.o: $(IMGUI_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(CXX) $(INCLUDE_FLAGS) -c $< -o $@

# Target rule for building imgui
imgui: $(OBJ_IMGUI)

# Define the base directories
IMGUIZMO_SRC_DIR = external/imguizmo
IMGUIZMO_OBJ_DIR = $(COMPILED_LIB_OUTPUT_WIN)/imguizmo
IMGUIZMO_BACKENDS_OBJ_DIR = $(IMGUIZMO_OBJ_DIR)/backends

# List of source files
IMGUIZMO_SRC = \
    $(IMGUIZMO_SRC_DIR)/GraphEditor.cpp \
    $(IMGUIZMO_SRC_DIR)/ImCurveEdit.cpp \
    $(IMGUIZMO_SRC_DIR)/ImGradient.cpp \
    $(IMGUIZMO_SRC_DIR)/ImGuizmo.cpp \
    $(IMGUIZMO_SRC_DIR)/ImSequencer.cpp 

# Corresponding object files
OBJ_IMGUIZMO = $(IMGUIZMO_SRC:$(IMGUIZMO_SRC_DIR)/%.cpp=$(IMGUIZMO_OBJ_DIR)/%.o)

# Pattern rule for building object files
$(IMGUIZMO_OBJ_DIR)/%.o: $(IMGUIZMO_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(CXX) $(INCLUDE_FLAGS) -c $< -o $@

# Target rule for building imguizmo
imguizmo: $(OBJ_IMGUIZMO)

$(PATH_BUILD_WIN)/SubNivis: mkdir_output_win windows_dependencies $(OBJ_WIN)
	@mkdir -p $(PATH_BUILD_WIN)
	@echo Linking $@
	@$(CXX) -o $@ $(OBJ_WIN) $(OBJ_IMGUI) $(LINKER_FLAGS)
	@echo Copying assets
	@cp -r $(ASSETS) $(PATH_BUILD_WIN)
	
$(PATH_BUILD_WIN)/LevelEditor: mkdir_output_win windows_dependencies $(OBJ_LEVEL_EDITOR)
	@mkdir -p $(PATH_BUILD_WIN)
	@echo Linking $@
	$(CXX) -o $@ $(OBJ_LEVEL_EDITOR) $(OBJ_IMGUI) $(OBJ_IMGUIZMO) $(LINKER_FLAGS)
	@cp -r $(ASSETS) $(PATH_BUILD_WIN)

$(PATH_OBJ_WIN)/%.o: $(SOURCE)/%.c
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

$(PATH_OBJ_WIN)/%.o: $(SOURCE)/%.cpp
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

windows: $(PATH_BUILD_WIN)/SubNivis
level_editor: $(PATH_BUILD_WIN)/LevelEditor

# Define compiler for PSX
$(TARGET_PSX): CC = D:/Tools/psn00bsdk/bin/mipsel-none-elf-gcc.exe
$(TARGET_PSX): CXX = D:/Tools/psn00bsdk/bin/mipsel-none-elf-g++.exe
$(TARGET_PSX): CFLAGS = -D_PSX
$(TARGET_PSX): CXXFLAGS = -D_PSX

clean:
	rm -rf $(PATH_TEMP)
	rm -rf $(PATH_BUILD)