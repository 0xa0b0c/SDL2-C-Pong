SHELL = /bin/sh
CC = cc
WARN = -Wall -Wextra -pedantic -Werror
DEBUG = -ggdb -DDEBUG
RELEASE = -O3
CFLAGS = -std=c99 $(DEBUG) $(WARN)
LDFLAGS = -lSDL2 -lSDL2_image

all: kesman
	mkdir -p out
	$(CC) -$(CFLAGS) -o out/pong pong.c subsystem.c $(LDFLAGS)

kesman:
	cppcheck --enable=all . --language=c --suppress=missingIncludeSystem

run:
	./out/pong

dbg:
	gdb -q ./out/pong