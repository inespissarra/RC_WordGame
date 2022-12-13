#ifndef AUXILIAR_PLAYER_H
#define AUXILIAR_PLAYER_H

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

void create_UDP_socket(char* hostname, char* port);
void create_TCP_socket(char* hostname, char* port);
void close_socket();
ssize_t read_from_TCP_socket(char *buffer, ssize_t nleft);
void start(char* hostname, char* port, char *buffer, char *PLID, char *game, int *trial_number);
void play(char* hostname, char* port, char *buffer, char *PLID, char *game, int *trial_number);
void guess(char* hostname, char* port, char *buffer, char *PLID, int *trial_number);
void read_file(int print, char *ptr, char *buffer);
void scoreboard(char* hostname, char* port, char *buffer, char *PLID);
void hint(char* hostname, char* port, char *buffer, char *PLID);
void state(char* hostname, char* port, char *buffer, char *PLID);
void quit(char* hostname, char* port, char *buffer, char *PLID, int *trial_number);


#endif
