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
void getNewWord(char *word_file);
void start(char *word_file, int verbose);
int countDiffChar(char word[MAX_WORD_LENGTH + 1]);
int getMaxErrors(int length);
void getCommand(char code, char *command);
void getState(FILE *fp, char *word, char *move, int state[4], char current_code);
void move(char *filename, char *move, char code, char *PLID, int trial_number);
void validMove(char *word, char *move, char code, int state[4], char *PLID, char *filename);
void play(int verbose);
void guess(int verbose);
void quit(int verbose);
void finishGame(char *PLID, char *filename, char state);
void createScoreFile(char* PLID, char *word, int corrects, int trials);
void printVerbose(char *command, char *PLID);


#endif
