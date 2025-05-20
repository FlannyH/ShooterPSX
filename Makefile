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
PATH_TEMP_PC = 		      $(PATH_TEMP)/pc
PATH_TEMP_NDS = 		  $(PATH_TEMP)/nds
PATH_TEMP_LEVEL_EDITOR =  $(PATH_TEMP)/level_editor
PATH_TEMP_LIGHT_BAKE   =  $(PATH_TEMP)/light_bake
PATH_BUILD_PSX = 		  $(PATH_BUILD)/psx
PATH_BUILD_PC = 		  $(PATH_BUILD)/pc
PATH_BUILD_NDS = 		  $(PATH_BUILD)/nds
PATH_BUILD_LEVEL_EDITOR = $(PATH_BUILD)/level_editor
PATH_BUILD_LIGHT_BAKE   = $(PATH_BUILD)/light_bake
PATH_LIB_PC  = $(PATH_TEMP_PC)/lib
PATH_LIB_PSX = $(PSN00BSDK_LIBS)/release
PATH_LIB_NDS = $(BLOCKSDS)/libs/libnds/lib

# Source files shared by all targets
CODE_ENGINE_SHARED_C = collision.c \
			  	  	   level.c \
			  	  	   memory.c \
			  	  	   mesh.c \
			  	  	   music.c \
			  	  	   renderer_shared.c \
					   texture.c \
					   text.c \
			  	  	   ui.c \
			  	  	   entity.c \
			  	  	   vislist.c

CODE_GAME_C = 		   debug_menu_main.c \
					   debug_menu_music.c \
					   debug_menu_level.c \
					   title_screen.c \
					   pause_menu.c \
					   settings.c \
					   credits.c \
					   in_game.c \
			  	  	   player.c \
				  	   entities/platform.c \
				  	   entities/trigger.c \
				  	   entities/pickup.c \
				  	   entities/chaser.c \
				  	   entities/crate.c \
				  	   entities/door.c

# Source files specific to PSX
CODE_ENGINE_PSX_C = psx/file.c \
				    psx/input.c \
				    psx/mesh.c \
				    psx/mixer.c \
				    psx/renderer.c 

# Source files specific to Windows
CODE_ENGINE_PC_C =   pc/file.c \
				     pc/input.c \
				     pc/mesh.c \
				     pc/mixer.c \
				     pc/psx.c \
				     pc/renderer.c 
CODE_ENGINE_PC_CPP = pc/debug_layer.cpp

# Source files specific to NDS
CODE_ENGINE_NDS_C = nds/psx.c \
				    nds/file.c \
				    nds/input.c \
				    nds/mesh.c \
				    nds/mixer.c \
				    nds/renderer.c 
					
# Source files used the light baker
CODE_ENGINE_LIGHT_BAKE_C =   editor/light_bake.c \
				      		 nds/mesh.c \
				      		 pc/mixer.c \
				      		 pc/psx.c \
				      		 pc/renderer.c \
				      		 pc/file.c 
CODE_ENGINE_LIGHT_BAKE_CPP = pc/debug_layer.cpp

# Where the object files go
PATH_OBJ_PSX = $(PATH_TEMP_PSX)/obj
PATH_OBJ_PC  = $(PATH_TEMP_PC)/obj
PATH_OBJ_NDS = $(PATH_TEMP_NDS)/obj
PATH_OBJ_LEVEL_EDITOR = $(PATH_TEMP_LEVEL_EDITOR)/obj
PATH_OBJ_LIGHT_BAKE = $(PATH_TEMP_LIGHT_BAKE)/obj

# Misc source file definitions
CODE_GAME_MAIN = main.c
CODE_LEVEL_EDITOR = editor/main.c editor/camera.c

# Create code sets and object sets
CODE_PSX_C				= $(CODE_ENGINE_SHARED_C)  		$(CODE_ENGINE_PSX_C) 	$(CODE_GAME_MAIN) 		$(CODE_GAME_C)
CODE_PSX_CPP			= $(CODE_ENGINE_SHARED_CPP) 	
CODE_PC_C				= $(CODE_ENGINE_SHARED_C)  		$(CODE_ENGINE_PC_C) 	$(CODE_GAME_MAIN) 		$(CODE_GAME_C)
CODE_PC_CPP			    = $(CODE_ENGINE_SHARED_CPP) 	$(CODE_ENGINE_PC_CPP)
CODE_NDS_C				= $(CODE_ENGINE_SHARED_C)  		$(CODE_ENGINE_NDS_C) 	$(CODE_GAME_MAIN) 		$(CODE_GAME_C)
CODE_NDS_CPP			= $(CODE_ENGINE_SHARED_CPP) 	$(CODE_ENGINE_NDS_CPP)
CODE_LEVEL_EDITOR_C		= $(CODE_ENGINE_SHARED_C)  		$(CODE_ENGINE_PC_C) 	$(CODE_LEVEL_EDITOR) 	$(CODE_GAME_C)
CODE_LEVEL_EDITOR_CPP	= $(CODE_ENGINE_SHARED_CPP) 	$(CODE_ENGINE_PC_CPP)
CODE_LIGHT_BAKE_C		= $(CODE_ENGINE_LIGHT_BAKE_C)	$(CODE_ENGINE_SHARED_C)
CODE_LIGHT_BAKE_CPP	    = $(CODE_ENGINE_LIGHT_BAKE_CPP)

OBJ_PSX					= 	$(patsubst %.c, 	$(PATH_OBJ_PSX)/%.o,	        $(CODE_PSX_C))				\
							$(patsubst %.cpp, 	$(PATH_OBJ_PSX)/%.o,	        $(CODE_PSX_CPP))				
OBJ_PC					= 	$(patsubst %.c, 	$(PATH_OBJ_PC)/%.o,	            $(CODE_PC_C))				\
							$(patsubst %.cpp, 	$(PATH_OBJ_PC)/%.o,	            $(CODE_PC_CPP))				
OBJ_NDS					= 	$(patsubst %.c, 	$(PATH_OBJ_NDS)/%.o,	        $(CODE_NDS_C))				\
							$(patsubst %.cpp, 	$(PATH_OBJ_NDS)/%.o,	        $(CODE_NDS_CPP))				
OBJ_LEVEL_EDITOR		= 	$(patsubst %.c, 	$(PATH_OBJ_LEVEL_EDITOR)/%.o, 	$(CODE_LEVEL_EDITOR_C))		\
							$(patsubst %.cpp, 	$(PATH_OBJ_LEVEL_EDITOR)/%.o, 	$(CODE_LEVEL_EDITOR_CPP))		
OBJ_LIGHT_BAKE		    = 	$(patsubst %.c, 	$(PATH_OBJ_LIGHT_BAKE)/%.o, 	$(CODE_LIGHT_BAKE_C))		\
							$(patsubst %.cpp, 	$(PATH_OBJ_LIGHT_BAKE)/%.o, 	$(CODE_LIGHT_BAKE_CPP))		

CFLAGS = -Wall -Wextra -std=c11 -Wno-old-style-declaration -Wno-format 
CXXFLAGS = -Wall -Wextra -std=c++20 -Wno-format
LINKER_FLAGS = 

.PHONY: all submodules tools assets pc level_editor psx nds clean mkdir_output_pc pc_dependencies glfw gl3w imgui imguizmo light_bake
all: submodules tools assets pc level_editor psx nds light_bake

# Windows target
pc: DEFINES = _PC
pc: LIBRARIES = glfw3 portaudio stdc++
ifeq ($(OS),Windows_NT)
pc: LIBRARIES += gdi32 opengl32 winmm ole32 SetupAPI 
else
pc: LIBRARIES += asound
endif
pc: CC = gcc
pc: CXX = g++
pc: CFLAGS += $(patsubst %, -D%, $(DEFINES)) -g
pc: CXXFLAGS += $(patsubst %, -D%, $(DEFINES)) -std=c++20 -g
pc: LINKER_FLAGS += $(patsubst %, -l%, $(LIBRARIES)) $(patsubst %, -L%, $(PATH_LIB_PC)) -std=c++20
pc: INCLUDE_DIRS = source \
			   external/portaudio/include \
			   external/cglm/include \
			   external/gl3w/include \
			   external/glfw/include \
			   external/imgui \
			   $(PATH_LIB_PC)/gl3w/include
pc: INCLUDE_FLAGS = $(patsubst %, -I%, $(INCLUDE_DIRS))
level_editor: DEFINES = _PC _LEVEL_EDITOR _DEBUG_CAMERA _DEBUG
level_editor: LIBRARIES = glfw3 portaudio stdc++ 
ifeq ($(OS),Windows_NT)
level_editor: LIBRARIES += gdi32 opengl32 winmm ole32 SetupAPI 
endif
level_editor: CC = gcc
level_editor: CXX = g++
level_editor: CFLAGS += $(patsubst %, -D%, $(DEFINES)) -g
level_editor: CXXFLAGS += $(patsubst %, -D%, $(DEFINES)) -std=c++20 -g
level_editor: LINKER_FLAGS += $(patsubst %, -l%, $(LIBRARIES)) $(patsubst %, -L%, $(PATH_LIB_PC)) -std=c++20
level_editor: INCLUDE_DIRS = source \
			   external/portaudio/include \
			   external/cglm/include \
			   external/gl3w/include \
			   external/glfw/include \
			   external/imgui \
			   external/imguizmo \
			   external/imgui-filebrowser \
			   $(PATH_LIB_PC)/gl3w/include
level_editor: INCLUDE_FLAGS = $(patsubst %, -I%, $(INCLUDE_DIRS))

light_bake: DEFINES = _PC _LIGHT_BAKE _DEBUG
light_bake: LIBRARIES = glfw3 portaudio stdc++ 
ifeq ($(OS),Windows_NT)
light_bake: LIBRARIES += gdi32 opengl32 winmm ole32 SetupAPI 
endif
light_bake: CC = gcc
light_bake: CXX = g++
light_bake: CFLAGS += $(patsubst %, -D%, $(DEFINES)) -g
light_bake: CXXFLAGS += $(patsubst %, -D%, $(DEFINES)) -std=c++20 -g
light_bake: LINKER_FLAGS += $(patsubst %, -l%, $(LIBRARIES)) $(patsubst %, -L%, $(PATH_LIB_PC)) -std=c++20
light_bake: INCLUDE_DIRS = source \
			   external/portaudio/include \
			   external/cglm/include \
			   external/gl3w/include \
			   external/glfw/include \
			   external/imgui \
			   external/imguizmo \
			   external/imgui-filebrowser \
			   $(PATH_LIB_PC)/gl3w/include
light_bake: INCLUDE_FLAGS = $(patsubst %, -I%, $(INCLUDE_DIRS))

mkdir_output_pc:
	mkdir -p $(PATH_TEMP_PC)
	mkdir -p $(PATH_OBJ_PC)
	mkdir -p $(PATH_OBJ_PC)/entities
	mkdir -p $(PATH_OBJ_PC)/pc

submodules:
	# git submodule update --init --recursive
	
pc_dependencies: submodules glfw gl3w imgui imguizmo portaudio

GLFW_LIB_PATHS = $(PATH_LIB_PC)/glfw/src/libglfw3.a \
				 $(PATH_LIB_PC)/glfw/src/glfw3.lib \
				 $(PATH_LIB_PC)/glfw/src/Debug/glfw3.lib \
				 $(PATH_LIB_PC)/glfw/src/Release/glfw3.lib \

# Build glfw - using Unix Makefiles generator because we're already using Make anyway
glfw:
	mkdir -p $(PATH_LIB_PC)/glfw
	@cmake -S external/glfw -B $(PATH_LIB_PC)/glfw -G "Unix Makefiles"
	@cmake --build $(PATH_LIB_PC)/glfw --target glfw
	cp $(PATH_LIB_PC)/glfw/src/libglfw3.a $(PATH_LIB_PC)

# Build portaudio
portaudio:
	mkdir -p $(PATH_LIB_PC)/portaudio
	@cmake -S external/portaudio -B $(PATH_LIB_PC)/portaudio -G "Unix Makefiles"
	@cmake --build $(PATH_LIB_PC)/portaudio --target portaudio
	cp $(PATH_LIB_PC)/portaudio/libportaudio.a $(PATH_LIB_PC)

OBJ_PC += $(PATH_LIB_PC)/gl3w.o
OBJ_LEVEL_EDITOR += $(PATH_LIB_PC)/gl3w.o
OBJ_LIGHT_BAKE += $(PATH_LIB_PC)/gl3w.o
gl3w:
	mkdir -p $(PATH_LIB_PC)/gl3w
	@cmake -S external/gl3w -B $(PATH_LIB_PC)/gl3w -G "Unix Makefiles"
	@cmake --build $(PATH_LIB_PC)/gl3w
	$(CC) -std=c11 -I$(PATH_LIB_PC)/gl3w/include -c $(PATH_LIB_PC)/gl3w/src/gl3w.c -o $(PATH_LIB_PC)/gl3w.o

# Define the base directories
IMGUI_SRC_DIR = external/imgui
IMGUI_OBJ_DIR = $(PATH_LIB_PC)/imgui
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
IMGUIZMO_OBJ_DIR = $(PATH_LIB_PC)/imguizmo
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

$(PATH_BUILD_PC)/$(PROJECT_NAME): mkdir_output_pc pc_dependencies $(OBJ_PC)
	@mkdir -p $(dir $@)
	@echo Linking $@
	@$(CXX) -o $@ $(OBJ_PC) $(OBJ_IMGUI) $(LINKER_FLAGS)
	@echo Copying assets
	@cp $(PATH_TEMP)/pc/assets.sfa $(dir $@)

$(PATH_BUILD_LEVEL_EDITOR)/assets/imgui.ini:
	@mkdir -p $(dir $@)
	@echo Copying default imgui.ini
	@cp $(PATH_ASSETS_TO_BUILD)/level_editor/imgui.ini $@

$(PATH_BUILD_LEVEL_EDITOR)/LevelEditor: mkdir_output_pc pc_dependencies $(OBJ_LEVEL_EDITOR) $(PATH_BUILD_LEVEL_EDITOR)/assets/imgui.ini
	@mkdir -p $(dir $@)
	@mkdir -p $(PATH_ASSETS)/shared
	@mkdir -p $(PATH_ASSETS)/pc
	@mkdir -p $(PATH_ASSETS)/level_editor
	@echo Linking $@
	@$(CXX) -o $@ $(OBJ_LEVEL_EDITOR) $(OBJ_IMGUI) $(OBJ_IMGUIZMO) $(LINKER_FLAGS)
	@echo Copying assets
	@cp -r $(PATH_ASSETS)/shared/* $(dir $@)/assets
	@cp -r $(PATH_ASSETS)/pc/* $(dir $@)/assets
	@cp -r $(PATH_ASSETS)/level_editor/* $(dir $@)/assets

$(PATH_BUILD_LIGHT_BAKE)/LightBake: mkdir_output_pc pc_dependencies $(OBJ_LIGHT_BAKE)
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $@)/assets
	@mkdir -p $(PATH_ASSETS)/shared
	@mkdir -p $(PATH_ASSETS)/pc
	@mkdir -p $(PATH_ASSETS)/light_bake
	@echo Linking $@
	@$(CXX) -o $@ $(OBJ_LIGHT_BAKE) $(OBJ_IMGUI) $(OBJ_IMGUIZMO) $(LINKER_FLAGS)
	@echo Copying assets
	cp -r $(PATH_ASSETS)/shared/* $(dir $@)/assets
	cp -r $(PATH_ASSETS)/pc/* $(dir $@)/assets
	cp -r $(PATH_ASSETS)/level_editor/* $(dir $@)/assets
	@echo Copying assets
	@cp $(PATH_TEMP)/pc/assets.sfa $(dir $@)

$(PATH_OBJ_PC)/%.o: $(PATH_SOURCE)/%.c
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

$(PATH_OBJ_PC)/%.o: $(PATH_SOURCE)/%.cpp
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

$(PATH_OBJ_LIGHT_BAKE)/%.o: $(PATH_SOURCE)/%.c
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

$(PATH_OBJ_LIGHT_BAKE)/%.o: $(PATH_SOURCE)/%.cpp
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

pc: tools assets $(PATH_BUILD_PC)/$(PROJECT_NAME)
level_editor: tools assets $(PATH_BUILD_LEVEL_EDITOR)/LevelEditor
light_bake: tools assets $(PATH_BUILD_LIGHT_BAKE)/LightBake

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
psx: CFLAGS += $(patsubst %, -D%, $(DEFINES)) -Wno-unused-function -fanalyzer -O3 -g -Wa,--strip-local-absolute -ffreestanding -fno-builtin -nostdlib -fdata-sections -ffunction-sections -fsigned-char -fno-strict-overflow -fdiagnostics-color=always -msoft-float -march=r3000 -mtune=r3000 -mabi=32 -mno-mt -mno-llsc -G8 -fno-pic -mno-abicalls -mgpopt -mno-extern-sdata -MMD -MP -flto=auto -fuse-linker-plugin
psx: CXXFLAGS += $(patsubst %, -D%, $(DEFINES)) -std=c++20 -flto=auto -fuse-linker-plugin
psx: LINKER_FLAGS += $(patsubst %, -l%, $(LIBRARIES)) $(patsubst %, -L%, $(PATH_LIB_PSX)) -nostdlib -Wl,-gc-sections -G8 -static -T$(PSN00BSDK_LIBS)/ldscripts/exe.ld -flto=auto -save-temps
psx: INCLUDE_DIRS = source \
			   $(PATH_LIB_PC)/gl3w/include \
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
nds: LINKER_FLAGS += $(patsubst %, -L%, $(PATH_LIB_NDS)) -mthumb -mcpu=arm946e-s+nofp -Wl,--start-group $(patsubst %, -l%, $(LIBRARIES)) -Wl,--end-group -specs=$(BLOCKSDS)/sys/crts/ds_arm9.specs
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
	@mkdir -p $(PATH_ASSETS)/shared
	@mkdir -p $(PATH_ASSETS)/nds
	@echo Building $@
	@mkdir -p $(PATH_TEMP_NDS)/nitrofs
	@cp -r $(PATH_ASSETS)/shared/* $(PATH_TEMP_NDS)/nitrofs
	@cp -r $(PATH_ASSETS)/nds/* $(PATH_TEMP_NDS)/nitrofs
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
COMPILED_ASSET_LIST = $(PATH_ASSETS)/pc/GOURAUD.FSH \
					  $(PATH_ASSETS)/pc/GOURAUD.VSH \
					  $(PATH_ASSETS)/pc/BLIT.FSH \
					  $(PATH_ASSETS)/pc/BLIT.VSH \
					  $(PATH_ASSETS)/shared/levels/test.lvl \
					  $(PATH_ASSETS)/shared/levels/test2.lvl \
					  $(PATH_ASSETS)/shared/levels/test3.lvl \
					  $(PATH_ASSETS)/shared/levels/level1.lvl \
					  $(PATH_ASSETS)/shared/levels/level2.lvl \
					  $(PATH_ASSETS)/shared/models/entity.msh \
					  $(PATH_ASSETS)/shared/models/entity.txc \
					  $(PATH_ASSETS)/shared/models/level.col \
					  $(PATH_ASSETS)/shared/models/level.vis \
					  $(PATH_ASSETS)/shared/models/level.msh \
					  $(PATH_ASSETS)/shared/models/level.txc \
					  $(PATH_ASSETS)/shared/models/level2.col \
					  $(PATH_ASSETS)/shared/models/level2.vis \
					  $(PATH_ASSETS)/shared/models/level2.msh \
					  $(PATH_ASSETS)/shared/models/level2.txc \
					  $(PATH_ASSETS)/shared/models/test.col \
					  $(PATH_ASSETS)/shared/models/test.vis \
					  $(PATH_ASSETS)/shared/models/test.msh \
					  $(PATH_ASSETS)/shared/models/test.txc \
					  $(PATH_ASSETS)/shared/models/test2.col \
					  $(PATH_ASSETS)/shared/models/test2.vis \
					  $(PATH_ASSETS)/shared/models/test2.msh \
					  $(PATH_ASSETS)/shared/models/test2.txc \
					  $(PATH_ASSETS)/shared/models/test3.col \
					  $(PATH_ASSETS)/shared/models/test3.vis \
					  $(PATH_ASSETS)/shared/models/test3.msh \
					  $(PATH_ASSETS)/shared/models/test3.txc \
					  $(PATH_ASSETS)/shared/models/weapons.msh \
					  $(PATH_ASSETS)/shared/models/weapons.txc \
					  $(PATH_ASSETS)/shared/models/ui_tex/menu1.txc \
					  $(PATH_ASSETS)/shared/models/ui_tex/menu2.txc \
					  $(PATH_ASSETS)/nds/models/ui_tex/menu_ds.txc \
					  $(PATH_ASSETS)/shared/models/ui_tex/ui.txc \
					  $(PATH_ASSETS)/pc/audio/instr.sbk \
					  $(PATH_ASSETS)/pc/audio/sfx.sbk \
					  $(PATH_ASSETS)/psx/audio/instr.sbk \
					  $(PATH_ASSETS)/psx/audio/sfx.sbk \
					  $(PATH_ASSETS)/shared/audio/music/black.dss \
					  $(PATH_ASSETS)/shared/audio/music/combust.dss \
					  $(PATH_ASSETS)/shared/audio/music/e1m1.dss \
					  $(PATH_ASSETS)/shared/audio/music/e3m3.dss \
					  $(PATH_ASSETS)/shared/audio/music/energia.dss \
					  $(PATH_ASSETS)/shared/audio/music/justice.dss \
					  $(PATH_ASSETS)/shared/audio/music/level1.dss \
					  $(PATH_ASSETS)/shared/audio/music/level2.dss \
					  $(PATH_ASSETS)/shared/audio/music/level3.dss \
					  $(PATH_ASSETS)/shared/audio/music/pitchtst.dss \
					  $(PATH_ASSETS)/shared/audio/music/subnivis.dss \
					  $(PATH_ASSETS)/level_editor/editor/gizmos.msh \
					  $(PATH_ASSETS)/level_editor/editor/gizmos.txc

# Shaders for Windows and Level Editor build
$(PATH_ASSETS)/pc/%.FSH: $(PATH_ASSETS_TO_BUILD)/%.FSH
	@mkdir -p $(dir $@)
	@echo Copying $@
	@cp $< $@

$(PATH_ASSETS)/pc/%.VSH: $(PATH_ASSETS_TO_BUILD)/%.VSH
	@mkdir -p $(dir $@)
	@echo Copying $@
	@cp $< $@

# Level files - just copy as is
$(PATH_ASSETS)/shared/levels/%.lvl: $(PATH_ASSETS_TO_BUILD)/levels/%.lvl
	@mkdir -p $(dir $@)
	@echo Copying $@
	@cp $< $@

# If we encounter a vislist, we need to compile the model slightly differently. So do that before creating the vislist
$(PATH_ASSETS)/shared/models/%.vis: $(PATH_ASSETS_TO_BUILD)/models/%.obj
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(PATH_TOOLS_BIN)/obj2psx$(EXE_EXT) --input $< --output $(basename $@) --split
	@echo Compiling vislist $<
	@$(PATH_TOOLS_BIN)/psx_vislist_generator$(EXE_EXT) $(basename $@).msh $(basename $@).col $@

# Collision model
$(PATH_ASSETS)/shared/models/%.col: $(PATH_ASSETS_TO_BUILD)/models/%_col.obj
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(PATH_TOOLS_BIN)/obj2psx$(EXE_EXT) --input $< --output $(basename $@) --collision

# Any other model, like weapon models or entity models
$(PATH_ASSETS)/shared/models/%.msh: $(PATH_ASSETS_TO_BUILD)/models/%.obj
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(PATH_TOOLS_BIN)/obj2psx$(EXE_EXT) --input $< --output $(basename $@)
$(PATH_ASSETS)/shared/models/%.txc: $(PATH_ASSETS)/models/%.msh

# UI textures
$(PATH_ASSETS)/shared/models/ui_tex/%.txc: $(PATH_ASSETS_TO_BUILD)/models/ui_tex/%.png
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(PATH_TOOLS_BIN)/obj2psx$(EXE_EXT) --input $< --output $@
$(PATH_ASSETS)/nds/models/ui_tex/%.txc: $(PATH_ASSETS_TO_BUILD)/models/ui_tex/%.png
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(PATH_TOOLS_BIN)/obj2psx$(EXE_EXT) --input $< --output $@

# Soundbank
$(PATH_ASSETS)/pc/audio/%.sbk: $(PATH_ASSETS_TO_BUILD)/audio/%.csv
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(PATH_TOOLS_BIN)/psx_soundfont_generator$(EXE_EXT) $< $@ pcm16

$(PATH_ASSETS)/psx/audio/%.sbk: $(PATH_ASSETS_TO_BUILD)/audio/%.csv
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(PATH_TOOLS_BIN)/psx_soundfont_generator$(EXE_EXT) $< $@ psx

# Music sequences
$(PATH_ASSETS)/shared/audio/music/%.dss: $(PATH_ASSETS_TO_BUILD)/audio/music/%.mid
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(PATH_TOOLS_BIN)/midi2psx$(EXE_EXT) $< $@

# Level editor assets
$(PATH_ASSETS)/level_editor/%.msh: $(PATH_ASSETS_TO_BUILD)/level_editor/%.obj
	@mkdir -p $(dir $@)
	@echo Compiling $<
	@$(PATH_TOOLS_BIN)/obj2psx$(EXE_EXT) --input $< --output $(basename $@)

$(PATH_TEMP)/pc/assets.sfa: $(COMPILED_ASSET_LIST)
	@mkdir -p $(dir $@)
	@mkdir -p $(PATH_ASSETS)/shared
	@mkdir -p $(PATH_ASSETS)/pc
	@mkdir -p $(PATH_TEMP)/pc/assets/
	@echo Compiling $<
	@cp -r $(PATH_ASSETS)/shared/* $(PATH_TEMP)/pc/assets/
	@cp -r $(PATH_ASSETS)/pc/* $(PATH_TEMP)/pc/assets/
	@$(PATH_TOOLS_BIN)/fsfa_builder$(EXE_EXT) $(PATH_TEMP)/pc/assets/ $@ --align 2048

$(PATH_TEMP)/psx/assets.sfa: $(COMPILED_ASSET_LIST)
	@mkdir -p $(dir $@)
	@mkdir -p $(PATH_ASSETS)/shared
	@mkdir -p $(PATH_ASSETS)/psx
	@mkdir -p $(PATH_TEMP)/psx/assets/
	@echo Compiling $<
	@cp -r $(PATH_ASSETS)/shared/* $(PATH_TEMP)/psx/assets/
	@cp -r $(PATH_ASSETS)/psx/* $(PATH_TEMP)/psx/assets/
	@$(PATH_TOOLS_BIN)/fsfa_builder$(EXE_EXT) $(PATH_TEMP)/psx/assets/ $@ --align 2048

assets: tools $(PATH_TEMP)/pc/assets.sfa $(PATH_TEMP)/psx/assets.sfa

clean_assets: 
	rm -rf $(PATH_ASSETS)

rebuild_assets: clean_assets assets

clean:
	rm -rf $(PATH_TEMP)
	rm -rf $(PATH_BUILD)
	rm -rf $(PATH_ASSETS)
	rm -rf $(PATH_TOOLS_BIN)
	cargo clean --manifest-path=tools/obj2psx/Cargo.toml
	cargo clean --manifest-path=tools/midi2psx/Cargo.toml
	cargo clean --manifest-path=tools/psx_vislist_generator/Cargo.toml
