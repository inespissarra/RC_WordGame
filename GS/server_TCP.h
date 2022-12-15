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

#include "constants.h"

void TCP_command(char *port, int verbose);
void TCP_open_socket(char *port);
void write_file(char *filename, char *buffer, int verbose);
void read_PLID(char *PLID);
void hint(int verbose);

#endif