CC=gcc
CFLAGS=-I. # -I. includes header files in the current directory
DEPS = constants.h # header files
OBJ = player.o # c files

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

player: $(OBJ) # executable file with name "player"
	$(CC) -o $@ $^ $(CFLAGS)

clean: # make clean
	rm -f $(OBJ)