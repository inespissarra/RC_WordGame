#include  <signal.h>
#include "constants.h"
#include "auxiliar_player.h"

int errno;
char hostname[MAX_HOSTNAME_SIZE + 1] = "\0";
char* port = "58082";

char buffer[MAX_READ_SIZE + 1], PLID[MAX_PLID_SIZE + 1], game[MAX_WORD_LENGTH + 1];
int trial_number = 0;
int erros_restantes = 0;

void  INThandler(int sig);

int main(int argc, char** argv){

    // Read hostname and port
    for(int i=1; i < argc; i+=2){
        if(!strcmp(argv[i], "-n"))
            strcpy(hostname, argv[i+1]);
        else if(!strcmp(argv[i], "-p")){
            port = argv[i+1];
        }
    }
    if(strlen(hostname)==0){
        if(gethostname(buffer, MAX_READ_SIZE)==-1)
            fprintf(stderr, "error: %s\n", strerror(errno));
        else
            strcpy(hostname, buffer);
    }

    signal(SIGINT, INThandler);

    printf("Welcome to Word Game :D\n");

    char command[10];
    while(1){
        // Read command
        scanf("%s", command);
        if(!strcmp(command, "start") || !strcmp(command, "sg"))
            start(hostname, port, buffer, PLID, game, &trial_number, &erros_restantes);
        else if(!strcmp(command, "play") || !strcmp(command, "pl")){
            play(hostname, port, buffer, PLID, game, &trial_number, &erros_restantes);
        }
        else if(!strcmp(command, "guess") || !strcmp(command, "gw"))
            guess(hostname, port, buffer, PLID, &trial_number, &erros_restantes);
        else if(!strcmp(command, "scoreboard") || !strcmp(command, "sb"))
            scoreboard(hostname, port, buffer, PLID);
        else if(!strcmp(command, "hint") || !strcmp(command, "h"))
            hint(hostname, port, buffer, PLID);
        else if(!strcmp(command, "state") || !strcmp(command, "st"))
            state(hostname, port, buffer, PLID);
        else if(!strcmp(command, "quit") || !strcmp(command, "q")){
            if(trial_number>0)
                quit(hostname, port, buffer, PLID, &trial_number);
            else{
                printf(NO_GAME);
            }
        }
        else if(!strcmp(command, "exit")){
            if(trial_number>0)
                quit(hostname, port, buffer, PLID, &trial_number);
            else{
                printf(NO_GAME);
            }
            printf(EXIT);
            break; 
        }
        else
            printf(INVALID_COMMAND);
    }
}

void  INThandler(int sig){
    signal(sig, SIG_IGN);
    printf("\n");
    if(trial_number>0)
        quit(hostname, port, buffer, PLID, &trial_number);
    else{
        printf(NO_GAME);
    }
    printf(EXIT);
    exit(0);
}