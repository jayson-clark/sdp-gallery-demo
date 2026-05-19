# Web Makefile for the FEH Simulator.
# Invoked indirectly by build.sh — do not call directly unless you've set up
# emscripten yourself (emcc must be on PATH).
#
# Required variables:
#   SRC_DIR  — absolute path to the student project (contains main.cpp + simulator_libraries/)
#   OUT_DIR  — where to drop index.html / game.js / game.wasm
#   GAME     — output basename (default "game")
#
# Optional:
#   ASSET_DIR — directory inside SRC_DIR to bundle as MEMFS (default "assets" if exists)

GAME    ?= game
WEB_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
LIB_DIR := $(SRC_DIR)/simulator_libraries
OUT_DIR ?= $(WEB_DIR)/dist/$(GAME)

# --- Source discovery -------------------------------------------------------
# All student .cpp files anywhere under SRC_DIR, EXCEPT the bundled libraries.
recursiveWildcard = $(foreach d,$(wildcard $(1:=/*)),$(call recursiveWildcard,$d,$2) $(filter $(subst *,%,$2),$d))
STUDENT_SRCS := $(filter-out $(LIB_DIR)/%, $(call recursiveWildcard,$(SRC_DIR),*.cpp))

# FEH library sources, EXCLUDING the ones we replace for web.
LIB_SRCS := \
    $(LIB_DIR)/FEHLCD.cpp \
    $(LIB_DIR)/FEHUtility.cpp \
    $(LIB_DIR)/FEHRandom.cpp \
    $(LIB_DIR)/FEHSD.cpp \
    $(LIB_DIR)/FEHImages.cpp \
    $(LIB_DIR)/FEHKeyboard.cpp

# Web-only replacements for tigr.cpp and FEHSound.cpp.
WEB_SRCS := \
    $(WEB_DIR)/tigr_web.cpp \
    $(WEB_DIR)/overrides/FEHSound_web.cpp

ALL_SRCS := $(STUDENT_SRCS) $(LIB_SRCS) $(WEB_SRCS)

# --- Compiler flags ---------------------------------------------------------
CXX      := em++
CXXFLAGS := -std=c++14 -O2 -w \
            -I$(LIB_DIR) -I$(WEB_DIR) \
            -DEMSCRIPTEN_WEB_BUILD

# Asyncify is what lets `while(1) LCD.Update();` work in a browser without
# editing the student's code: tigrUpdate() yields the Wasm stack to the JS
# event loop on every call, then resumes.
LDFLAGS := \
    -sASYNCIFY=1 \
    -sASYNCIFY_STACK_SIZE=65536 \
    -sALLOW_MEMORY_GROWTH=1 \
    -sINITIAL_MEMORY=33554432 \
    -sEXPORTED_FUNCTIONS=['_main','_tigr_web_key_event','_tigr_web_char_event','_tigr_web_mouse_event','_tigr_web_close_event'] \
    -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS','UTF8ToString','HEAPU8'] \
    -sENVIRONMENT=web \
    -sMODULARIZE=1 \
    -sEXPORT_NAME=createFEHGame \
    -sFORCE_FILESYSTEM=1

# Preload an asset directory if one exists.
ifeq ($(strip $(ASSET_DIR)),)
ASSET_DIR := $(SRC_DIR)/assets
endif
ifneq ($(wildcard $(ASSET_DIR)),)
LDFLAGS += --preload-file $(ASSET_DIR)@/
endif

# --- Targets ----------------------------------------------------------------
.PHONY: all clean check_stb

all: check_stb $(OUT_DIR)/index.html

check_stb:
	@test -f $(WEB_DIR)/stb_image.h || { \
	  echo "[web] Downloading stb_image.h..."; \
	  curl -sSLf -o $(WEB_DIR)/stb_image.h \
	    https://raw.githubusercontent.com/nothings/stb/master/stb_image.h ; }

$(OUT_DIR)/index.html: $(ALL_SRCS) $(WEB_DIR)/shell/index.html $(WEB_DIR)/shell/embed.html $(WEB_DIR)/shell/style.css
	@mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) $(ALL_SRCS) $(LDFLAGS) -o $(OUT_DIR)/game.js
	@cp $(WEB_DIR)/shell/index.html $(OUT_DIR)/index.html
	@cp $(WEB_DIR)/shell/embed.html $(OUT_DIR)/embed.html
	@cp $(WEB_DIR)/shell/style.css  $(OUT_DIR)/style.css
	@echo
	@echo "[web] Built: $(OUT_DIR)/"
	@ls -lh $(OUT_DIR)

clean:
	@rm -rf $(OUT_DIR)
