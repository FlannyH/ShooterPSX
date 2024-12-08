PROJECT_NAME = SubNivis
ISO_XML = iso.xml
PROJECT_SOURCE_DIR = .

# Get executable extension
EXE_EXT = 
ifeq ($(OS),Windows_NT)
	EXE_EXT += .exe
endif

# Find NDS compiler
ifeq ($(GCC_ARM_NONE_EABI_PATH),)
	ifneq ($(wildcard /opt/wonderful/toolchain/gcc-arm-none-eabi),)
		GCC_ARM_NONE_EABI_PATH = /opt/wonderful/toolchain/gcc-arm-none-eabi
	endif
	ifneq ($(wildcard C:/msys64/opt/wonderful/toolchain/gcc-arm-none-eabi),)
		GCC_ARM_NONE_EABI_PATH = C:/msys64/opt/wonderful/toolchain/gcc-arm-none-eabi
	endif
endif

# Find BlocksDS for NDS target
ifeq ($(BLOCKSDS),)
	ifneq ($(wildcard C:/msys64/opt/wonderful/thirdparty/blocksds/core)),)
		BLOCKSDS := C:/msys64/opt/wonderful/thirdparty/blocksds/core
	endif
	ifneq ($(wildcard /opt/wonderful/thirdparty/blocksds/core)),)
		BLOCKSDS := /opt/wonderful/thirdparty/blocksds/core
	endif
	ifneq ($(wildcard /opt/blocksds/core)),)
		BLOCKSDS := /opt/blocksds/core
	endif
endif

# Input folders
PATH_SOURCE = source
PATH_ASSETS_TO_BUILD = assets_to_build

# Output folders
PATH_TEMP = temp
PATH_BUILD = build
PATH_ASSETS = assets
PATH_TOOLS_BIN = tools/bin/

PATH_TEMP_PSX = 		  $(PATH_TEMP)/psx
PATH_TEMP_WIN = 		  $(PATH_TEMP)/windows
PATH_TEMP_NDS = 		  $(PATH_TEMP)/nds
PATH_TEMP_LEVEL_EDITOR =  $(PATH_TEMP)/level_editor
PATH_BUILD_PSX = 		  $(PATH_BUILD)/psx
PATH_BUILD_WIN = 		  $(PATH_BUILD)/windows
PATH_BUILD_NDS = 		  $(PATH_BUILD)/nds
PATH_BUILD_LEVEL_EDITOR = $(PATH_BUILD)/level_editor
PATH_LIB_WIN = $(PATH_TEMP_WIN)/lib
PATH_LIB_PSX = $(PSN00BSDK_LIBS)/release
PATH_LIB_NDS = $(BLOCKSDS)/libs/libnds/lib

# Source files shared by all targets
CODE_ENGINE_SHARED_C = collision.c \
					   credits.c \
					   debug_menu_main.c \
					   debug_menu_music.c \
					   debug_menu_level.c \
			  	  	   entity.c \
					   in_game.c \
			  	  	   level.c \
			  	  	   memory.c \
			  	  	   mesh.c \
			  	  	   player.c \
			  	  	   renderer_shared.c \
					   title_screen.c \
					   settings.c \
					   pause_menu.c \
					   texture.c \
					   text.c \
			  	  	   ui.c \
			  	  	   vislist.c \
				  	   entities/chaser.c \
				  	   entities/crate.c \
				  	   entities/door.c \
				  	   entities/pickup.c \
				  	   entities/platform.c \
				  	   entities/trigger.c 

# Source files specific to PSX
CODE_ENGINE_PSX_C = psx/file.c \
				    psx/input.c \
				    psx/mesh.c \
				    psx/music.c \
				    psx/renderer.c \
				    psx/timer.c 

# Source files specific to Windows
CODE_ENGINE_WIN_C = windows/file.c \
				    windows/input.c \
				    windows/mesh.c \
				    windows/music.c \
				    windows/psx.c \
				    windows/renderer.c 
CODE_ENGINE_WIN_CPP = windows/debug_layer.cpp

# Source files specific to NDS
CODE_ENGINE_NDS_C = nds/psx.c \
				    nds/file.c \
				    nds/input.c \
				    nds/mesh.c \
				    nds/music.c \
				    nds/renderer.c \
				    nds/timer.c 

# Where the object files go
PATH_OBJ_PSX = $(PATH_TEMP_PSX)/obj
PATH_OBJ_WIN = $(PATH_TEMP_WIN)/obj
PATH_OBJ_NDS = $(PATH_TEMP_NDS)/obj
PATH_OBJ_LEVEL_EDITOR = $(PATH_TEMP_LEVEL_EDITOR)/obj

# Misc source file definitions
CODE_GAME_MAIN = main.c
CODE_LEVEL_EDITOR = editor/main.c

# Create code sets and object sets
CODE_PSX_C				= $(CODE_ENGINE_SHARED_C)  		$(CODE_ENGINE_PSX_C) 	$(CODE_GAME_MAIN)
CODE_PSX_CPP			= $(CODE_ENGINE_SHARED_CPP) 	
CODE_WIN_C				= $(CODE_ENGINE_SHARED_C)  		$(CODE_ENGINE_WIN_C) 	$(CODE_GAME_MAIN)
CODE_WIN_CPP			= $(CODE_ENGINE_SHARED_CPP) 	$(CODE_ENGINE_WIN_CPP)
CODE_NDS_C				= $(CODE_ENGINE_SHARED_C)  		$(CODE_ENGINE_NDS_C) 	$(CODE_GAME_MAIN)
CODE_NDS_CPP			= $(CODE_ENGINE_SHARED_CPP) 	$(CODE_ENGINE_NDS_CPP)
CODE_LEVEL_EDITOR_C		= $(CODE_ENGINE_SHARED_C)  		$(CODE_ENGINE_WIN_C) 	$(CODE_LEVEL_EDITOR) 
CODE_LEVEL_EDITOR_CPP	= $(CODE_ENGINE_SHARED_CPP) 	$(CODE_ENGINE_WIN_CPP)

OBJ_PSX					= 	$(patsubst %.c, 	$(PATH_OBJ_PSX)/%.o,	        $(CODE_PSX_C))				\
							$(patsubst %.cpp, 	$(PATH_OBJ_PSX)/%.o,	        $(CODE_PSX_CPP))				
OBJ_WIN					= 	$(patsubst %.c, 	$(PATH_OBJ_WIN)/%.o,	        $(CODE_WIN_C))				\
							$(patsubst %.cpp, 	$(PATH_OBJ_WIN)/%.o,	        $(CODE_WIN_CPP))				
OBJ_NDS					= 	$(patsubst %.c, 	$(PATH_OBJ_NDS)/%.o,	        $(CODE_NDS_C))				\
							$(patsubst %.cpp, 	$(PATH_OBJ_NDS)/%.o,	        $(CODE_NDS_CPP))				
OBJ_LEVEL_EDITOR		= 	$(patsubst %.c, 	$(PATH_OBJ_LEVEL_EDITOR)/%.o, 	$(CODE_LEVEL_EDITOR_C))		\
							$(patsubst %.cpp, 	$(PATH_OBJ_LEVEL_EDITOR)/%.o, 	$(CODE_LEVEL_EDITOR_CPP))		

CFLAGS = -Wall -Wextra -std=c11 -Wno-old-style-declaration -Wno-format
CXXFLAGS = -Wall -Wextra -std=c++20 -Wno-format

.PHONY: all submodules tools assets windows level_editor psx nds clean mkdir_output_win windows_dependencies glfw gl3w imgui imguizmo
all: submodules tools assets windows level_editor psx nds 

# Windows target
windows: DEFINES = _WINDOWS
windows: LIBRARIES = glfw3 stdc++ 
ifeq ($(OS),Windows_NT)
windows: LIBRARIES += gdi32 opengl32
endif
# ifeq ($(OS),Windows_NT)
# 	windows: LIBRARIES += gdi32 opengl32
# endif
windows: CC = gcc
windows: CXX = g++
windows: CFLAGS += $(patsubst %, -D%, $(DEFINES)) -g
windows: CXXFLAGS += $(patsubst %, -D%, $(DEFINES)) -std=c++20 -g
windows: LINKER_FLAGS = $(patsubst %, -l%, $(LIBRARIES)) $(patsubst %, -L%, $(PATH_LIB_WIN)) -std=c++20
windows: INCLUDE_DIRS = source \
			   external/cglm/include \
			   external/gl3w/include \
			   external/glfw/include \
			   external/imgui \
			   $(PATH_LIB_WIN)/gl3w/include
windows: INCLUDE_FLAGS = $(patsubst %, -I%, $(INCLUDE_DIRS))
level_editor: DEFINES = _WINDOWS _WIN32 _LEVEL_EDITOR _DEBUG_CAMERA _DEBUG
level_editor: LIBRARIES = glfw3 stdc++ gdi32 opengl32
level_editor: CC = gcc
level_editor: CXX = g++
level_editor: CFLAGS += $(patsubst %, -D%, $(DEFINES)) -g
level_editor: CXXFLAGS += $(patsubst %, -D%, $(DEFINES)) -std=c++20 -g
level_editor: LINKER_FLAGS = $(patsubst %, -l%, $(LIBRARIES)) $(patsubst %, -L%, $(PATH_LIB_WIN)) -std=c++20
level_editor: INCLUDE_DIRS = source \
			   external/cglm/include \
			   external/gl3w/include \
			   external/glfw/include \
			   external/imgui \
			   external/imguizmo \
			   external/imgui-filebrowser \
			   $(PATH_LIB_WIN)/gl3w/include
level_editor: INCLUDE_FLAGS = $(patsubst %, -I%, $(INCLUDE_DIRS))

mkdir_output_win:
	mkdir -p $(PATH_TEMP_WIN)
	mkdir -p $(PATH_OBJ_WIN)
	mkdir -p $(PATH_OBJ_WIN)/entities
	mkdir -p $(PATH_OBJ_WIN)/windows

submodules:
	git submodule update --init --recursive
	
windows_dependencies: submodules glfw gl3w imgui imguizmo

GLFW_LIB_PATHS = $(PATH_LIB_WIN)/glfw/src/libglfw3.a \
				 $(PATH_LIB_WIN)/glfw/src/glfw3.lib \
				 $(PATH_LIB_WIN)/glfw/src/Debug/glfw3.lib \
				 $(PATH_LIB_WIN)/glfw/src/Release/glfw3.lib \

# Build glfw - using Unix Makefiles generator because we're already using Make anyway
glfw:
	mkdir -p $(PATH_LIB_WIN)/glfw
	@cmake -S external/glfw -B $(PATH_LIB_WIN)/glfw -G "Unix Makefiles"
	@cmake --build $(PATH_LIB_WIN)/glfw --target glfw
	cp $(PATH_LIB_WIN)/glfw/src/libglfw3.a $(PATH_LIB_WIN)
	@echo Could not find compiled glfw library!


OBJ_WIN += $(PATH_LIB_WIN)/gl3w.o
OBJ_LEVEL_EDITOR += $(PATH_LIB_WIN)/gl3w.o
gl3w:
	mkdir -p $(PATH_LIB_WIN)/gl3w
	@cmake -S external/gl3w -B $(PATH_LIB_WIN)/gl3w -G "Unix Makefiles"
	@cmake --build $(PATH_LIB_WIN)/gl3w
	$(CC) -std=c11 -I$(PATH_LIB_WIN)/gl3w/include -c $(PATH_LIB_WIN)/gl3w/src/gl3w.c -o $(PATH_LIB_WIN)/gl3w.o

# Define the base directories
IMGUI_SRC_DIR = external/imgui
IMGUI_OBJ_DIR = $(PATH_LIB_WIN)/imgui
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
IMGUIZMO_OBJ_DIR = $(PATH_LIB_WIN)/imguizmo
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

$(PATH_BUILD_WIN)/$(PROJECT_NAME): mkdir_output_win windows_dependencies $(OBJ_WIN)
	@mkdir -p $(dir $@)
	@echo Linking $@
	@$(CXX) -o $@ $(OBJ_WIN) $(OBJ_IMGUI) $(LINKER_FLAGS)
	@echo Copying assets
	@cp -r $(PATH_ASSETS) $(dir $@)

$(PATH_BUILD_LEVEL_EDITOR)/imgui.ini:
	@mkdir -p $(dir $@)
	@echo Copying default imgui.ini
	@cp $(PATH_ASSETS_TO_BUILD)/level_editor/imgui.ini $@

$(PATH_BUILD_LEVEL_EDITOR)/LevelEditor: mkdir_output_win windows_dependencies $(OBJ_LEVEL_EDITOR) $(PATH_BUILD_LEVEL_EDITOR)/imgui.ini
	@mkdir -p $(dir $@)
	@echo Linking $@
	@$(CXX) -o $@ $(OBJ_LEVEL_EDITOR) $(OBJ_IMGUI) $(OBJ_IMGUIZMO) $(LINKER_FLAGS)
	@echo Copying assets
	@cp -r $(PATH_ASSETS) $(dir $@)

$(PATH_OBJ_WIN)/%.o: $(PATH_SOURCE)/%.c
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

$(PATH_OBJ_WIN)/%.o: $(PATH_SOURCE)/%.cpp
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

$(PATH_OBJ_LEVEL_EDITOR)/%.o: $(PATH_SOURCE)/%.c
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

$(PATH_OBJ_LEVEL_EDITOR)/%.o: $(PATH_SOURCE)/%.cpp
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

windows: tools assets $(PATH_BUILD_WIN)/$(PROJECT_NAME)
level_editor: tools assets $(PATH_BUILD_LEVEL_EDITOR)/LevelEditor

# PSX target
psx: PSN00BSDK_PATH = $(PSN00BSDK_LIBS)/../..
psx: DEFINES = _PSX PSN00BSDK=1 NDEBUG=1
psx: LIBRARIES = gcc \
				 psxgpu_exe_gprel \
				 psxgte_exe_gprel \
				 psxspu_exe_gprel \
				 psxcd_exe_gprel \
				 psxpress_exe_gprel \
				 psxsio_exe_gprel \
				 psxetc_exe_gprel \
				 psxapi_exe_gprel \
				 smd_exe_gprel \
				 lzp_exe_gprel \
				 c_exe_gprel \
				 gcc 
psx: CC = $(PSN00BSDK_PATH)/bin/mipsel-none-elf-gcc$(EXE_EXT)
psx: CXX = $(PSN00BSDK_PATH)/bin/mipsel-none-elf-g++$(EXE_EXT)
psx: CFLAGS += $(patsubst %, -D%, $(DEFINES)) -Wno-unused-function -fanalyzer -O3 -g -Wa,--strip-local-absolute -ffreestanding -fno-builtin -nostdlib -fdata-sections -ffunction-sections -fsigned-char -fno-strict-overflow -fdiagnostics-color=always -msoft-float -march=r3000 -mtune=r3000 -mabi=32 -mno-mt -mno-llsc -G8 -fno-pic -mno-abicalls -mgpopt -mno-extern-sdata -MMD -MP
psx: CXXFLAGS += $(patsubst %, -D%, $(DEFINES)) -std=c++20
psx: LINKER_FLAGS = $(patsubst %, -l%, $(LIBRARIES)) $(patsubst %, -L%, $(PATH_LIB_PSX)) -nostdlib -Wl,-gc-sections -G8 -static -T$(PSN00BSDK_LIBS)/ldscripts/exe.ld
psx: INCLUDE_DIRS = source \
			   $(PATH_LIB_WIN)/gl3w/include \
			   $(PSN00BSDK_PATH)/include/libpsn00b 
psx: INCLUDE_FLAGS = $(patsubst %, -I%, $(INCLUDE_DIRS))

$(PATH_TEMP_PSX)/$(PROJECT_NAME).elf: $(OBJ_PSX)
	@mkdir -p $(dir $@)
	@echo Linking $@
	@$(CC) -o $@ $(OBJ_PSX) $(LINKER_FLAGS)

$(PATH_OBJ_PSX)/%.o: $(PATH_SOURCE)/%.c
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

$(PATH_OBJ_PSX)/%.o: $(PATH_SOURCE)/%.cpp
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

$(PATH_TEMP_PSX)/$(PROJECT_NAME).exe: $(PATH_TEMP_PSX)/$(PROJECT_NAME).elf
	@mkdir -p $(dir $@)
	@echo Creating $@
	@$(PSN00BSDK_PATH)/bin/elf2x -q $(PATH_TEMP_PSX)/$(PROJECT_NAME).elf $(PATH_TEMP_PSX)/$(PROJECT_NAME).exe

psx: tools assets $(PATH_TEMP_PSX)/$(PROJECT_NAME).exe
	@mkdir -p $(PATH_BUILD_PSX)
	@echo Building CD image
	$(PSN00BSDK_PATH)/bin/mkpsxiso -y -o $(PATH_BUILD_PSX)/$(PROJECT_NAME).bin -c $(PATH_BUILD_PSX)/$(PROJECT_NAME).cue $(ISO_XML)

# NDS target
# Set GCC_ARM_NONE_EABI_PATH to the folder containing the GCC compiler
# todo: maybe try to find it using a set of defaults first before resorting to this one
nds: DEFINES = _NDS
nds: LIBRARIES = nds9 c
nds: CC = $(GCC_ARM_NONE_EABI_PATH)/bin/arm-none-eabi-gcc$(EXE_EXT)
nds: CXX = $(GCC_ARM_NONE_EABI_PATH)/bin/arm-none-eabi-g++$(EXE_EXT)
nds: CFLAGS += $(patsubst %, -D%, $(DEFINES)) -mthumb -mcpu=arm946e-s+nofp -std=gnu17 -O2 -ffunction-sections -fdata-sections -specs=$(BLOCKSDS)/sys/crts/ds_arm9.specs
nds: CXXFLAGS += $(patsubst %, -D%, $(DEFINES)) -mthumb -mcpu=arm946e-s+nofp -std=gnu++17 -O2 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -specs=$(BLOCKSDS)/sys/crts/ds_arm9.specs
nds: LINKER_FLAGS = $(patsubst %, -L%, $(PATH_LIB_NDS)) -mthumb -mcpu=arm946e-s+nofp -Wl,--start-group $(patsubst %, -l%, $(LIBRARIES)) -Wl,--end-group -specs=$(BLOCKSDS)/sys/crts/ds_arm9.specs
nds: INCLUDE_DIRS = source \
					$(BLOCKSDS)/libs/libnds/include
nds: INCLUDE_FLAGS = $(patsubst %, -I%, $(INCLUDE_DIRS))

$(PATH_OBJ_NDS)/%.o: $(PATH_SOURCE)/%.c
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

$(PATH_OBJ_NDS)/%.o: $(PATH_SOURCE)/%.cpp
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

$(PATH_TEMP_NDS)/$(PROJECT_NAME).elf: $(OBJ_NDS)
	@mkdir -p $(dir $@)
	@echo Linking $@
	@$(CC) -o $@ $(OBJ_NDS) $(LINKER_FLAGS)

$(PATH_BUILD_NDS)/$(PROJECT_NAME).nds: $(PATH_TEMP_NDS)/$(PROJECT_NAME).elf
	@mkdir -p $(dir $@)
	@echo Building $@
	@mkdir -p $(PATH_TEMP_NDS)/nitrofs
	@cp -r $(PATH_ASSETS) $(PATH_TEMP_NDS)/nitrofs
	$(BLOCKSDS)/tools/ndstool/ndstool$(EXE_EXT) -c $@ -7 $(BLOCKSDS)/sys/default_arm7/arm7.elf -9 $< $(NDSTOOL_ARGS) -d $(PATH_TEMP_NDS)/nitrofs -b $(PATH_ASSETS_TO_BUILD)/ds_icon.png "$(PROJECT_NAME)"

nds: tools assets $(PATH_BUILD_NDS)/$(PROJECT_NAME).nds

obj2psx:
	@mkdir -p $(PATH_TOOLS_BIN)
	@echo Building $@
	@cargo build --release --manifest-path=tools/obj2psx/Cargo.toml
	@cp tools/obj2psx/target/release/obj2psx$(EXE_EXT) $(PATH_TOOLS_BIN)

midi2psx:
	@mkdir -p $(PATH_TOOLS_BIN)
	@echo Building $@
	@cargo build --release --manifest-path=tools/midi2psx/Cargo.toml
	@cp tools/midi2psx/target/release/midi2psx$(EXE_EXT) $(PATH_TOOLS_BIN)

psx_vislist_generator:
	@mkdir -p $(PATH_TOOLS_BIN)
	@echo Building $@
	@cargo build --release --manifest-path=tools/psx_vislist_generator/Cargo.toml
	@cp tools/psx_vislist_generator/target/release/psx_vislist_generator$(EXE_EXT) $(PATH_TOOLS_BIN)

psx_soundfont_generator:
	@mkdir -p $(PATH_TOOLS_BIN)
	@echo Building $@
	@make -C tools/psx_soundfont_generator TARGET_DIR=../../$(PATH_TOOLS_BIN)

fsfa_builder:
	@mkdir -p $(PATH_TOOLS_BIN)
	@echo Building $@
	@make -C tools/fsfa-builder TARGET_DIR=../../$(PATH_TOOLS_BIN)

tools: obj2psx midi2psx psx_vislist_generator psx_soundfont_generator fsfa_builder

# For levels, make the first 2 art .col, .vis, and then the rest. this way everything can be built in the right order
COMPILED_ASSET_LIST = $(PATH_ASSETS)/GOURAUD.FSH \
					  $(PATH_ASSETS)/GOURAUD.VSH \
					  $(PATH_ASSETS)/levels/test.lvl \
					  $(PATH_ASSETS)/levels/test2.lvl \
					  $(PATH_ASSETS)/levels/level1.lvl \
					  $(PATH_ASSETS)/levels/level2.lvl \
					  $(PATH_ASSETS)/models/entity.msh \
					  $(PATH_ASSETS)/models/entity.txc \
					  $(PATH_ASSETS)/models/level.col \
					  $(PATH_ASSETS)/models/level.vis \
					  $(PATH_ASSETS)/models/level.msh \
					  $(PATH_ASSETS)/models/level.txc \
					  $(PATH_ASSETS)/models/level2.col \
					  $(PATH_ASSETS)/models/level2.vis \
					  $(PATH_ASSETS)/models/level2.msh \
					  $(PATH_ASSETS)/models/level2.txc \
					  $(PATH_ASSETS)/models/test.col \
					  $(PATH_ASSETS)/models/test.vis \
					  $(PATH_ASSETS)/models/test.msh \
					  $(PATH_ASSETS)/models/test.txc \
					  $(PATH_ASSETS)/models/test2.col \
					  $(PATH_ASSETS)/models/test2.vis \
					  $(PATH_ASSETS)/models/test2.msh \
					  $(PATH_ASSETS)/models/test2.txc \
					  $(PATH_ASSETS)/models/weapons.msh \
					  $(PATH_ASSETS)/models/weapons.txc \
					  $(PATH_ASSETS)/models/ui_tex/menu1.txc \
					  $(PATH_ASSETS)/models/ui_tex/menu2.txc \
					  $(PATH_ASSETS)/models/ui_tex/menu_ds.txc \
					  $(PATH_ASSETS)/models/ui_tex/ui.txc \
					  $(PATH_ASSETS)/audio/instr.sbk \
					  $(PATH_ASSETS)/audio/sfx.sbk \
					  $(PATH_ASSETS)/audio/music/black.dss \
					  $(PATH_ASSETS)/audio/music/combust.dss \
					  $(PATH_ASSETS)/audio/music/e1m1.dss \
					  $(PATH_ASSETS)/audio/music/e3m3.dss \
					  $(PATH_ASSETS)/audio/music/energia.dss \
					  $(PATH_ASSETS)/audio/music/justice.dss \
					  $(PATH_ASSETS)/audio/music/level1.dss \
					  $(PATH_ASSETS)/audio/music/level2.dss \
					  $(PATH_ASSETS)/audio/music/level3.dss \
					  $(PATH_ASSETS)/audio/music/pitchtst.dss \
					  $(PATH_ASSETS)/audio/music/subnivis.dss

# Shaders for Windows and Level Editor build
$(PATH_ASSETS)/%.FSH: $(PATH_ASSETS_TO_BUILD)/%.FSH
	@mkdir -p $(dir $@)
	@echo Copying $@
	@cp $< $@

$(PATH_ASSETS)/%.VSH: $(PATH_ASSETS_TO_BUILD)/%.VSH
	@mkdir -p $(dir $@)
	@echo Copying $@
	@cp $< $@

# Level files - just copy as is
$(PATH_ASSETS)/levels/%.lvl: $(PATH_ASSETS_TO_BUILD)/levels/%.lvl
	@mkdir -p $(dir $@)
	@echo Copying $@
	@cp $< $@

# If we encounter a vislist, we need to compile the model slightly differently. So do that before creating the vislist
$(PATH_ASSETS)/models/%.vis: $(PATH_ASSETS_TO_BUILD)/models/%.obj
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(PATH_TOOLS_BIN)/obj2psx$(EXE_EXT) --input $< --output $(basename $@) --split
	@echo Compiling vislist $<
	@$(PATH_TOOLS_BIN)/psx_vislist_generator$(EXE_EXT) $(basename $@).msh $(basename $@).col $@

# Collision model
$(PATH_ASSETS)/models/%.col: $(PATH_ASSETS_TO_BUILD)/models/%_col.obj
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(PATH_TOOLS_BIN)/obj2psx$(EXE_EXT) --input $< --output $(basename $@) --collision

# Any other model, like weapon models or entity models
$(PATH_ASSETS)/models/%.msh: $(PATH_ASSETS_TO_BUILD)/models/%.obj
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(PATH_TOOLS_BIN)/obj2psx$(EXE_EXT) --input $< --output $(basename $@)
$(PATH_ASSETS)/models/%.txc: $(PATH_ASSETS)/models/%.msh

# UI textures
$(PATH_ASSETS)/models/ui_tex/%.txc: $(PATH_ASSETS_TO_BUILD)/models/ui_tex/%.png
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(PATH_TOOLS_BIN)/obj2psx$(EXE_EXT) --input $< --output $@

# Soundbank
$(PATH_ASSETS)/audio/%.sbk: $(PATH_ASSETS_TO_BUILD)/audio/%.csv
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(PATH_TOOLS_BIN)/psx_soundfont_generator$(EXE_EXT) $< $@

# Music sequences
$(PATH_ASSETS)/audio/music/%.dss: $(PATH_ASSETS_TO_BUILD)/audio/music/%.mid
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(PATH_TOOLS_BIN)/midi2psx$(EXE_EXT) $< $@

$(PATH_TEMP)/assets.sfa: $(COMPILED_ASSET_LIST)
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(PATH_TOOLS_BIN)/fsfa_builder$(EXE_EXT) $(PATH_ASSETS) $@

assets: $(PATH_TEMP)/assets.sfa

clean_assets: 
	rm -rf $(PATH_ASSETS)

rebuild_assets: clean_assets assets

clean:
	rm -rf $(PATH_TEMP)
	rm -rf $(PATH_BUILD)
	rm -rf $(PATH_ASSETS)
	rm -f $(PATH_TOOLS_BIN)/midi2psx$(EXE_EXT)
	rm -f $(PATH_TOOLS_BIN)/obj2psx$(EXE_EXT)
	rm -f $(PATH_TOOLS_BIN)/psx_soundfont_generator$(EXE_EXT)
	rm -f $(PATH_TOOLS_BIN)/psx_vislist_generator$(EXE_EXT)
	cargo clean --manifest-path=tools/obj2psx/Cargo.toml
	cargo clean --manifest-path=tools/midi2psx/Cargo.toml
	cargo clean --manifest-path=tools/psx_vislist_generator/Cargo.toml