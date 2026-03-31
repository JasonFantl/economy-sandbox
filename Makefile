CC     = gcc
CFLAGS = -Wall -Wextra -O2 -I.
LIBS   = $(shell pkg-config --libs raylib) -lm

# Shared source files used by both executables
SHARED_SRC = src/world.c src/tileset.c

# Game simulation
GAME_SRC   = main.c src/agent.c src/econ.c src/market.c src/render.c \
             src/inspector.c src/controls.c src/assets.c $(SHARED_SRC)

# Map builder tool (lives in its own folder; binary at mapbuilder/mapbuilder)
BUILDER_SRC = mapbuilder/mapbuilder.c $(SHARED_SRC)
BUILDER_BIN = mapbuilder/mapbuilder

all: game $(BUILDER_BIN)

game: $(GAME_SRC)
	$(CC) $(CFLAGS) $(GAME_SRC) -o game $(LIBS)

$(BUILDER_BIN): $(BUILDER_SRC)
	$(CC) $(CFLAGS) $(BUILDER_SRC) -o $(BUILDER_BIN) $(LIBS)

clean:
	rm -f game $(BUILDER_BIN)

.PHONY: all clean
