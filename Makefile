SHELL = /bin/sh
CC = cc
WARN = -Wall -Wextra -pedantic -Werror -march=native
DEBUG = -ggdb -DDEBUG
RELEASE = -O3 -DNDEBUG
CFLAGS = -std=c99 $(RELEASE) $(WARN)
LDFLAGS = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

all:
	mkdir -p out
	$(CC) -$(CFLAGS) -o out/pong src/pong.c src/subsystem.c src/timer.c $(LDFLAGS)

kesman:
	cppcheck --enable=all . --language=c --suppress=missingIncludeSystem

run:
	./out/pong

dbg:
	gdb -q ./out/pong