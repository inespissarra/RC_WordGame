#ifndef SERVER_UDP_H
#define SERVER_UDP_H

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#include "constants.h"

void UDP_command(char *word_file, char *port, int verbose);
void UDP_connect(char *port);
void get_new_word(char *word_file);
void finish_game(char *PLID, char *filename, char state);
void start(char *word_file, int verbose);
int count_unique_char(char word[MAX_WORD_LENGTH]);
void play(int verbose);
void guess(int verbose);
void quit(int verbose);


#endif