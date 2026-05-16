# Off-device host build: compiles the renderer and the test modes against the
# mock libnx layer in this directory and produces the scene generator.
#
#   make            build ./mockgen
#   make run        build, then render every scene into out/*.ppm
#   make assets     run, then convert out/*.ppm to PNG/GIF (needs ffmpeg)
#   make clean      remove the binary and out/

CXX      ?= g++
SRCDIR   := ../../source
CXXFLAGS := -std=gnu++17 -O2 -Wall -Wextra -I. -I$(SRCDIR)

MODES := gfx font menu_mode display_mode touch_mode \
         gesture_mode controls_mode hwinfo_mode
SRC   := $(addprefix $(SRCDIR)/,$(addsuffix .cpp,$(MODES))) mock.cpp mockgen.cpp

mockgen: $(SRC) switch.h mockctl.h
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

run: mockgen
	./mockgen

assets: run
	./gen-assets.sh

clean:
	rm -f mockgen
	rm -rf out

.PHONY: run assets clean
