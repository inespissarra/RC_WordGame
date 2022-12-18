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

#include "constants.h"

void TCP_command(char *port, int verbose);
void TCP_open_socket(char *port);
void write_file(char *filename, char *buffer, int verbose);
void read_PLID(char *PLID);
void hint(int verbose);
int find_top_scores(char sb_file[MAX_FILE_SIZE]);
void scoreboard(int verbose);
int find_last_game(char *PLID, char *filename);
void create_state_file(char *PLID, char *filename, char file[MAX_FILE_SIZE], int status);
void state(int verbose);
#endif