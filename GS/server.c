#include  <signal.h>
#include "server_UDP.h"
#include "server_TCP.h"

void INThandler(int sig);

int main(int argc, char** argv){

    char word_file[strlen(FOLDER_DATA) + MAX_FILENAME_SIZE + 1];
    strcpy(word_file, FOLDER_DATA);
    strcat(word_file, argv[1]);
    char *port = "58082";
    int verbose = 0;

    // Read port
    for(int i = 2; i < argc;){
        if(!strcmp(argv[i], "-p")){
            port = argv[i+1];
            i+=2;
        } else if(!strcmp(argv[i], "-v")){
            verbose = 1;
            i+=1;
        }
    }

    signal(SIGINT, INThandler);

    if(atoi(port) < 1024 || atoi(port) > 65535){
        fprintf(stderr, "error: invalid port number\n");
        exit(1);
    }

    struct sigaction act;
    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    if(sigaction(SIGCHLD, &act, NULL)==-1){
        perror("sigaction");
        exit(1);
    }
    
    pid_t c1_pid, c2_pid, wpid;
    int status;

    (c1_pid = fork()) && (c2_pid = fork());
    if(c1_pid == 0){
        UDP_command(word_file, port, verbose);
        exit(0);
    } else if(c2_pid == 0){
        TCP_command(port, verbose);
        exit(0);
    } else if(c1_pid > 0 && c2_pid > 0){
        /* Parent code goes here */
        while ((wpid = wait(&status)) > 0);
    } else{
        /* Error */
        perror("fork");
        exit(1);
    }
}

void INThandler(int sig){
    signal(sig, SIG_IGN);
    while(kill(0, SIGKILL)!=0);
    kill(0, SIGKILL);
    printf(BYE_SERVER);
    exit(0);
}