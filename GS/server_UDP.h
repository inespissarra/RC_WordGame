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
#include <dirent.h>

#include "constants.h"

#define N_TRIALS 0
#define CORRECTS 1
#define ERRORS 2
#define DUP 3

void UDP_command(char *word_file, char *port, int verbose);
void UDP_connect(char *port);
void get_new_word(char *word_file);
void start(char *word_file, int verbose);
int count_unique_char(char word[MAX_WORD_LENGTH]);
int get_max_errors(int length);
void get_command(char code, char *command);
void get_state(FILE *fp, char *word, char *move, int state[4], char current_code);
void move(char *filename, char *move, char code, char *PLID, int trial_number);
void create_score_file(char* PLID, char *word, int corrects, int trials);
void valid_move(char *word, char *move, char code, int state[4], char *PLID, char *filename);
void play(int verbose);
void guess(int verbose);
void quit(int verbose);
void finish_game(char *PLID, char *filename, char state);


#endif
