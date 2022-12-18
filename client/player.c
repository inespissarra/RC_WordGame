#include "constants.h"
#include "auxiliar_player.h"

int errno;

int main(int argc, char** argv){
    char hostname[MAX_HOSTNAME_SIZE + 1] = "\0";
    char* port = "58082";

    char buffer[MAX_READ_SIZE + 1], PLID[MAX_PLID_SIZE + 1], game[MAX_WORD_LENGTH + 1];
    int trial_number = 0;

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

    char command[10];
    while(1){
        // Read command
        scanf("%s", command);
        if(!strcmp(command, "start") || !strcmp(command, "sg"))
            start(hostname, port, buffer, PLID, game, &trial_number);
        else if(!strcmp(command, "play") || !strcmp(command, "pl"))
            play(hostname, port, buffer, PLID, game, &trial_number);
        else if(!strcmp(command, "guess") || !strcmp(command, "gw"))
            guess(hostname, port, buffer, PLID, &trial_number);
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
                printf("There is no game in progress\n");
            }
        }
        else if(!strcmp(command, "exit")){
            if(trial_number>0)
                quit(hostname, port, buffer, PLID, &trial_number);
            else{
                printf("There is no game in progress\n");
            }
            printf("Bye!\n");
            break; 
        }
        else
            printf("ERROR\n");
    }
}