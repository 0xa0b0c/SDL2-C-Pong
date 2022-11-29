SHELL = /bin/sh
CC = cc
WARN = -Wall -Wextra -pedantic -Werror
DEBUG = -ggdb -DDEBUG
RELEASE = -O3 -DNDEBUG
CFLAGS = -std=c99 $(DEBUG) $(WARN)
LDFLAGS = -lSDL2 -lSDL2_image -lSDL2_ttf

all: kesman
	mkdir -p out
	$(CC) -$(CFLAGS) -o out/pong src/pong.c src/subsystem.c src/timer.c $(LDFLAGS)

kesman:
	cppcheck --enable=all . --language=c --suppress=missingIncludeSystem

run:
	./out/pong

dbg:
	gdb -q ./out/pong