CC=gcc
CFLAGS=-I. # -I. includes header files in the current directory
SOURCES  := $(wildcard */*.c)
HEADERS  := $(wildcard */*.h)
OBJECTS  := $(SOURCES:.c=.o)
TARGET_EXECS := client/player GS/server


all: $(TARGET_EXECS)

client/player: client/player.c client/auxiliar_player.c

GS/server: GS/server.c

clean: # make clean
	rm -f $(OBJECTS) $(TARGET_EXECS)