# FujiNet News for the Bally Astrocade. Standalone, outside mekkogx, in the
# netcat/battleship mold: the build is build.sh (zmac + the layout checks),
# this file is the convenience wrapper.

SRCS := news.asm HVGLIB.H fujinet.inc fujilib.inc $(wildcard *.inc)

all: build/news.bin

build/news.bin: $(SRCS) build.sh tools/checkrom.py tools/checksize.py
	./build.sh

run: build/news.bin
	./run.sh

MAME_DIR ?= $(HOME)/Workspace/mame

# Headless end-to-end test: launch, pick Top Stories, open the first
# article, snapshotting the list and the reader. Needs a running
# fujinet-pc (BoIP on 127.0.0.1:9995) and a reachable news server.
smoke: build/news.bin
	rm -rf build/astrocde
	FUJINET_DEBUG=$${FUJINET_DEBUG:-1} $(MAME_DIR)/mame astrocde \
	    -rompath $(MAME_DIR)/roms \
	    -cartslot fujinet -cart $(CURDIR)/build/news.bin \
	    -autoboot_script emu/smoke.lua -video none -sound none \
	    -seconds_to_run 40 -snapshot_directory $(CURDIR)/build
	@ls build/astrocde/

clean:
	rm -rf build

.PHONY: all run smoke clean
