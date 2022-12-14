#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include "constants.h"


int fd, newfd, errno, errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;

char *word_file;
int n_lines;
char buffer[MAX_READ_SIZE + 1];
char *port = "58082";
int verbose = 0;

void random_word(){ // --------- afinal é sequencial
    int random_line = rand() % n_lines;
    FILE *fp = fopen(word_file, "r");
    for(int i=0; i<random_line; i++)
        fgets(buffer, MAX_WORD_LENGTH + 1, fp);
    fclose(fp);
}


void start(){
    char *ptr = buffer + 4;
    char word[MAX_WORD_LENGTH + 1];
    char PLID[MAX_PLID_SIZE + 1];
    sscanf(ptr, "%s", PLID);
    FILE *fp;
    char filename[MAX_PLAYER_FILENAME_SIZE];
    sprintf(filename, "GAME_%s.txt", PLID);
    if(!access(filename, F_OK)){ // File exists
        fp = fopen(filename, "r");
        fgets(buffer, MAX_READ_SIZE, fp);
        sscanf(buffer, "%s", word);
        if(fgets(buffer, MAX_READ_SIZE, fp)!=NULL){ // Check if no play was yet received
            fclose(fp);
            sprintf(buffer, "RSG NOK\n");
            n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addrlen);
            if(n == -1){
                fprintf(stderr, "error: %s\n", strerror(errno));
                exit(1);
            }
            return;
        }
        fclose(fp);
    } else{
        random_word();
        sscanf(buffer, "%s", word);
        fp = fopen(filename, "w");
        fprintf(fp, "%s", buffer);
        fclose(fp);
    }
    int max_errors;
    int len = strlen(word);
    if(len < 7)
        max_errors = 7;
    else if(len < 11)
        max_errors = 8;
    else
        max_errors = 9;

    sprintf(buffer, "RSG OK %d %d\n", len, max_errors);
    
    n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
}

void play(){
    char *ptr = buffer + 4;
    char letter;
    int trial_number;
    char word[MAX_WORD_LENGTH + 1];
    char PLID[MAX_PLID_SIZE + 1];
    n = sscanf(ptr, "%s %c %d\n", PLID, &letter, &trial_number);
    if(n==3){ // Correct format
        FILE *fp;
        char filename[MAX_PLAYER_FILENAME_SIZE];
        sprintf(filename, "GAME_%s.txt", PLID);
        if((fp = fopen(filename, "r+")) != NULL){
            fgets(buffer, MAX_READ_SIZE, fp);
            // TODO
        } else{
            sprintf(buffer, "RLG ERR\n");
        }
    } else{
        sprintf(buffer, "RLG ERR\n");
    }
    n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
}


void write_file(char *filename, char *buffer){
    char filename2[MAX_FILENAME_SIZE + 1];
    sprintf(filename2, "Dados_GS/%s", filename);
    FILE *fp = fopen(filename2, "r");
    if(fp == NULL){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fclose(fp);
    printf("tamanho do ficheiro: %d\n", size);

    sprintf(buffer, "%s %d ", filename, size);
    char *ptr = buffer;
    int n_left = strlen(buffer);
    while((n = write(newfd, ptr, n_left))!=0){
        if(n == -1){
            printf("ERROR\n");
            exit(1);
        }
        ptr += n;
        n_left -= n;
    }

    fp = fopen(filename2, "r");
    if(fp == NULL){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    while (size > 0){
        n_left = fread(buffer, 1, MAX_READ_SIZE, fp);
        ptr = buffer;
        while((n = write(newfd, ptr, n_left))!=0){
            if(n == -1){
                printf("ERROR\n");
                exit(1);
            }
            ptr += n;
            n_left -= n;
            size -=n;
        }
    }
    
    buffer[0] = '\n';
    write(newfd, buffer, 1);
    fclose(fp);
}

void hint(){
    char *ptr = buffer + 4;
    char PLID[MAX_PLID_SIZE + 1];
    int n = sscanf(ptr, "%s\n", PLID);
    FILE *fp;
    char filename[MAX_PLAYER_FILENAME_SIZE + 1];
    sprintf(filename, "GAME_%s.txt", PLID);
    
    if((fp = fopen(filename, "r")) != NULL){
        fgets(buffer, MAX_READ_SIZE, fp);
        fclose(fp);
        sscanf(buffer, "%*s %s", filename);
        sprintf(buffer, "RHL OK ");
        char *ptr = buffer;
        int n_left = strlen(buffer);
        while((n = write(newfd, ptr, n_left))!=0){
            if(n == -1){
                printf("ERROR1\n");
                exit(1);
            } 
            ptr += n;
            n_left -= n;
        }
        write_file(filename, buffer);
    } else {
        sprintf(buffer, "RHL NOK\n");
        while((n = write(newfd, buffer, strlen(buffer)))!=0)
        if(n == -1){
            printf("ERROR2\n");
            exit(1);
        }
    }
}


void UDP_connect(){

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd == -1){
        printf("ERROR\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, port, &hints, &res);
    if(errcode!=0){ 
        printf("ERROR\n");
        exit(1);
    }
    
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        printf("ERROR\n");
        exit(1);
    }
}

void TCP_open_socket(){
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1){
        printf("ERROR\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, port, &hints, &res);
    if(errcode!=0){ 
        printf("ERROR\n");
        exit(1);
    }
    
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        printf("ERROR10\n");
        exit(1);
    }

    if(listen(fd, 5) == -1){ // 5 is the maximum number of pending connections, can be changed (?)
        printf("ERROR\n");
        exit(1);
    }
}

void close_connection(){
    freeaddrinfo(res);
    close(fd);
}

void UDP_command(){
    char command[MAX_COMMAND_SIZE + 1];

    while(1){
        UDP_connect();
        addrlen = sizeof(addr);
        n = recvfrom(fd, buffer, MAX_READ_SIZE, 0, (struct sockaddr*)&addr, &addrlen);
        if(n == -1){
            printf("ERROR\n");
            exit(1);
        }
        buffer[n] = '\0';

        printf("%s\n", buffer);
        
        if(verbose)
            printf("Received: %s\n", buffer);

        sscanf(buffer, "%s", command);
        if(!strcmp(command, "SNG")){
            start();
        } else if(!strcmp(command, "PLG")){
            play();
        } else if(!strcmp(command, "PWG")){
            //guess();
        } else if(!strcmp(command, "QUT")){
            //quit();
        } else{
            // Invalid command
            printf("ERROR\n");
            exit(1);
        }
        close_connection();
    }
}
    
void TCP_command(){
    char command[MAX_COMMAND_SIZE + 1];

    TCP_open_socket();
    while(1){
        addrlen = sizeof(addr);
        if((newfd = accept(fd, (struct sockaddr*)&addr, &addrlen)) == -1){
            printf("ERROR\n");
            exit(1);
        }
        
        printf("TCP read starts\n");
        char *ptr = buffer;
        
        while((n=read(newfd, ptr, MAX_READ_SIZE))!=0){
            if(n == -1){
                printf("ERROR\n");
                exit(1);
            }
            ptr += n;
            break; //............................................................... reparar isto
        }
        
        *ptr = '\0';
        if(verbose)
            printf("Received: %s\n", buffer);

        sscanf(buffer, "%s ", command);
        if(!strcmp(command, "GSB")){
            //scoreboard();
        } else if(!strcmp(command, "GHL")){
            hint();
        } else if(!strcmp(command, "STA")){
            //state();
        } else{
            // Invalid command
            printf("ERROR\n");
            exit(1);
        }
        close(newfd);
    }
    close_connection();
}



int main(int argc, char** argv){

    word_file = argv[1];

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

    FILE *fp = fopen(word_file, "r");
    if(fp==NULL){
        printf("ERROR\n");
        exit(1);
    }
    n_lines = 0;
    while(fgets(buffer, MAX_READ_SIZE, fp) != NULL){
        n_lines++;
    }
    fclose(fp);
    
    pid_t c1_pid, c2_pid, wpid;
    int status;

    (c1_pid = fork()) && (c2_pid = fork());
    if(c1_pid == 0){
        UDP_command();
        exit(0);
    } else if(c2_pid == 0){
        TCP_command();
        exit(0);
    } else{
        /* Parent code goes here */
        while ((wpid = wait(&status)) > 0);
        printf("else\n");
    }
}