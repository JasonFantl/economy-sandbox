CC     = gcc
CFLAGS = -Wall -Wextra -O2 -I.
LIBS   = $(shell pkg-config --libs raylib) -lm

TARGET = game
SRC    = main.c src/agent.c src/market.c src/render.c src/inspector.c src/controls.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: clean
