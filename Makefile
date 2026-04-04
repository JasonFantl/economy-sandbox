CC     = gcc
CFLAGS = -Wall -Wextra -O2 -I.
LIBS   = $(shell pkg-config --libs raylib) -lm

ECON_SRC     = econ/agent.c econ/econ.c econ/market.c econ/nav.c
RENDER_SRC   = render/render.c render/panels.c render/controls.c render/inspector.c render/camera.c render/input.c
WTHROUGH_SRC = walkthrough/walkthrough.c walkthrough/scenes.c
WORLD_SRC    = world/world.c world/tileset.c

GAME_SRC     = main.c sim.c $(ECON_SRC) $(RENDER_SRC) $(WTHROUGH_SRC) $(WORLD_SRC)
BUILDER_SRC  = mapbuilder/mapbuilder.c world/world.c world/tileset.c
BUILDER_BIN  = mapbuilder/mapbuilder

all: game $(BUILDER_BIN)

game: $(GAME_SRC)
	$(CC) $(CFLAGS) $(GAME_SRC) -o game $(LIBS)

$(BUILDER_BIN): $(BUILDER_SRC)
	$(CC) $(CFLAGS) $(BUILDER_SRC) -o $(BUILDER_BIN) $(LIBS)

clean:
	rm -f game $(BUILDER_BIN)

.PHONY: all clean
