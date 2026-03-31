CC     = gcc
CFLAGS = -Wall -Wextra -O2 -I.
LIBS   = $(shell pkg-config --libs raylib) -lm

# Shared source files used by both executables
SHARED_SRC = src/world.c src/tileset.c

# Game simulation
GAME_SRC   = main.c src/agent.c src/econ.c src/market.c src/render.c \
             src/inspector.c src/controls.c src/assets.c $(SHARED_SRC)

# Map builder tool
BUILDER_SRC = mapbuilder.c $(SHARED_SRC)

all: game mapbuilder

game: $(GAME_SRC)
	$(CC) $(CFLAGS) $(GAME_SRC) -o game $(LIBS)

mapbuilder: $(BUILDER_SRC)
	$(CC) $(CFLAGS) $(BUILDER_SRC) -o mapbuilder $(LIBS)

clean:
	rm -f game mapbuilder

.PHONY: all clean
