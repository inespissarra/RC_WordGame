CC=gcc
CFLAGS=-I. # -I. includes header files in the current directory
SOURCES  := $(wildcard */*.c)
HEADERS  := $(wildcard */*.h)
OBJECTS  := $(SOURCES:.c=.o)
TARGET_EXECS := player


all: $(TARGET_EXECS)

player: player.c auxiliar_player.c

clean: # make clean
	rm -f $(OBJECTS) $(TARGET_EXECS)