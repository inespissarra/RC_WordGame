#ifndef SERVER_TCP_H
#define SERVER_TCP_H

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
#include <sys/dir.h>
#include <dirent.h>
#include <ctype.h>

#include "constants.h"

void TCP_command(char *port, int verbose);
void TCP_OpenSocket(char *port);
int writeFile(char *filename, char *buffer, int verbose);
int writeToTCP(char *ptr, int to_write, int verbose);
int isNumericTCP(char *str);
int readPLID(char *PLID, int verbose);
void hint(int verbose);
int findTopScores(char sb_file[MAX_FILE_SIZE + 1]);
void scoreboard(int verbose);
int findLastGame(char *PLID, char *filename);
void createStateFile(char *PLID, char *filename, char *file, int status);
void state(int verbose);
#endif