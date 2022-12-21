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
#include <ctype.h>

void createUDPsocket(char* hostname, char* port);
void createTCPsocket(char* hostname, char* port);
void closeSocket();
void sendAndReadUDP(char *buffer, char *hostname, char *port);
void sendAndReadTCP(char *buffer);
ssize_t readFromTCPsocket(char *buffer, ssize_t nleft);
void readFile(int command, char *buffer);
void readUntilSpace(char *ptr);
int isNumeric(char *str);
void start(char* hostname, char* port, char *buffer, char *PLID, char *game, int *trial_number, int *erros_restantes);
void play(char* hostname, char* port, char *buffer, char *PLID, char *game, int *trial_number, int *erros_restantes);
void guess(char* hostname, char* port, char *buffer, char *PLID, int *trial_number, int *erros_restantes);
void scoreboard(char* hostname, char* port, char *buffer, char *PLID);
void hint(char* hostname, char* port, char *buffer, char *PLID);
void state(char* hostname, char* port, char *buffer, char *PLID);
void quit(char* hostname, char* port, char *buffer, char *PLID, int *trial_number);


#endif
