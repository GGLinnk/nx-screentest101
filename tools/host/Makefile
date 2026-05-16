# Off-device host build: compiles the renderer and the test modes against the
# mock libnx layer in this directory and produces the scene generator.
#
#   make            build build/mockgen
#   make run        render every scene into build/*.ppm
#   make video      render the feature walk-through to dist/showcase.mp4 (needs ffmpeg)
#   make assets     run, then convert build/*.ppm to dist/ PNG/GIF (needs ffmpeg or ImageMagick)
#   make clean      remove build/ and dist/
#
# build/  intermediate artifacts (the binary and raw PPM frames)
# dist/   finished assets (PNG stills, GIFs, MP4)

CXX      ?= g++
SRCDIR   := ../../source
CXXFLAGS := -std=gnu++17 -O2 -Wall -Wextra -I. -I$(SRCDIR)

MODES := gfx font menu_mode display_mode touch_mode \
         gesture_mode controls_mode hwinfo_mode
SRC   := $(addprefix $(SRCDIR)/,$(addsuffix .cpp,$(MODES))) mock.cpp mockgen.cpp
BIN   := build/mockgen

$(BIN): $(SRC) switch.h mockctl.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

run: $(BIN)
	./$(BIN)

video: $(BIN)
	./$(BIN) --video

assets: run
	./gen-assets.sh

clean:
	rm -rf build dist

.PHONY: run video assets clean
