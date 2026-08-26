# Build math-game against either backend:
#   make PLATFORM=fbdev   (Raspberry Pi: raw /dev/fb0 + evdev)
#   make PLATFORM=sdl     (macOS dev loop, via nix-shell)
#
# Shared game logic (game.c, viewport.c, ttf_font.c) is identical either
# way; only the platform_*.c file and its flags differ. Text is rendered
# via FreeType against the bundled ../assets/DejaVuSans-Bold.ttf.

PLATFORM ?= fbdev
CC ?= cc
CFLAGS ?= -Wall -Wextra -std=c99 -O2
FONT_PATH ?= ../assets/DejaVuSans-Bold.ttf

COMMON_SRCS = game.c viewport.c ttf_font.c
TARGET = math_game
FT_CFLAGS = $(shell pkg-config --cflags freetype2)
FT_LIBS = $(shell pkg-config --libs freetype2)
DEFS = -DFONT_PATH=\"$(FONT_PATH)\"

ifeq ($(PLATFORM),fbdev)
	SRCS = $(COMMON_SRCS) platform_fbdev.c
	EXTRA_CFLAGS = $(FT_CFLAGS)
	EXTRA_LIBS = $(FT_LIBS) -lm
else ifeq ($(PLATFORM),sdl)
	SRCS = $(COMMON_SRCS) platform_sdl.c
	EXTRA_CFLAGS = $(shell pkg-config --cflags sdl2) $(FT_CFLAGS)
	EXTRA_LIBS = $(shell pkg-config --libs sdl2) $(FT_LIBS) -lm
else
	$(error Unknown PLATFORM '$(PLATFORM)', expected 'fbdev' or 'sdl')
endif

.PHONY: all clean run
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(DEFS) $(EXTRA_CFLAGS) $(SRCS) -o $(TARGET) $(EXTRA_LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
