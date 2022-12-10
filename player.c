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

#define MAX_WORD_LENGTH 31
#define MAX_FILENAME_SIZE 24
#define MAX_FSIZE_SIZE 10

int fd, errno, errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;

char buffer[129], PLID[7], game[MAX_WORD_LENGTH];
int trial_number;

char* hostname = NULL;
char* port = "58082";

void create_UDP_socket(){
    
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    errcode = getaddrinfo(hostname, port, &hints, &res);
    if(errcode != 0){
        fprintf(stderr, "error: %s\n", gai_strerror(errcode));
        exit(1);
    }
}

void create_TCP_socket(){
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    errcode = getaddrinfo(hostname, port, &hints, &res);
    if(errcode != 0){
        fprintf(stderr, "error: %s\n", gai_strerror(errcode));
        exit(1);
    }
}

void close_socket(){
    freeaddrinfo(res);
    close(fd);
}

ssize_t read_from_TCP_socket(ssize_t nleft){
    char *ptr = buffer;
    ssize_t total_read = 0;
    while(nleft > 0){
        n = read(fd, ptr, nleft);
        if(n == -1){
            fprintf(stderr, "error: %s\n", strerror(errno));
            exit(1);
        } else if (n == 0){
            break;
        }
        nleft -= n;
        ptr += n;
        total_read += n;
    }
    buffer[total_read] = '\0';
    return total_read;
}


void start(){
    scanf("%s", PLID);
    sprintf(buffer, "SNG %s\n", PLID);

    create_UDP_socket();

    n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    buffer[n]='\0';

    close_socket();

    char buf1[4], buf2[4];
    int n_let, errors;
    sscanf(buffer, "%s %s %d %d\n", buf1, buf2, &n_let, &errors);
    if(strcmp(buf1, "RSG")==0){
        if(strcmp(buf2, "OK") == 0){
            memset(game, '_', n_let);
            game[n_let] = '\0';
            printf("New game started (max %d errors): %s\n", errors, game);
            trial_number = 1;
        }
        else if(strcmp(buf2, "NOK") == 0)
            printf("Error: game already started\n");
        else
            printf("ERROR\n");
    }
    else{
        printf("ERROR\n");
    }
}


void play(){
    char letter;
    scanf(" %c", &letter);
    sprintf(buffer, "PLG %s %c %d\n", PLID, letter, trial_number);

    create_UDP_socket();

    n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    buffer[n]='\0';
    
    close_socket();

    char buf[4], *ptr = buffer;
    int trial;
    sscanf(buffer, "%s ", buf);
    if(strcmp(buf, "RLG")==0){
        ptr += 4;
        sscanf(ptr, "%s ", buf);
        if(strcmp(buf, "OK") == 0){
            int n_let, pos;
            ptr += 3;
            sscanf(ptr, "%d %d", &trial, &n_let);
            if (trial != trial_number)
                printf("ERROR\n");
            else {
                trial_number++;
                ptr += 4;
                for(int i = 0; i<n_let; i++){
                    sscanf(ptr, "%d", &pos);
                    game[pos-1] = letter;
                    ptr += 2;
                }
                printf("Yes, \"%c\" is part of the word: %s\n", letter, game);
            }

        } else if(strcmp(buf, "NOK") == 0){
            ptr += 4;
            sscanf(ptr, "%d", &trial);
            if (trial != trial_number)
                printf("ERROR\n");
            else{
                printf("No, \"%c\" is not part of the word: %s\n", letter, game);
                trial_number++;
            }

        } else if(!strcmp(buf, "WIN")){
            ptr += 4;
            sscanf(ptr, "%d", &trial);
            if(trial != trial_number)
                printf("ERROR\n");
            else{
                for(int i=0; i < sizeof(game); i++){
                    if(game[i] == '_')
                        game[i] = letter;
                }
                printf("WELL DONE! You guessed: %s\n", game);
                trial_number = 0;
            }

        } else if(!strcmp(buf, "DUP")){
            ptr += 4;
            sscanf(ptr, "%d", &trial);
            if (trial != trial_number)
                printf("ERROR\n");
            else
                printf("The letter has been inserted before\n");
        } else if(!strcmp(buf, "OVR")){
            ptr += 4;
            sscanf(ptr, "%d", &trial);
            if (trial != trial_number)
                printf("TRIAL NUMBER ERROR\n");
            else{
                printf("No, \"%c\" is not part of the word: %s\nYou have reached the maximum error limit GG\n", letter, game);
                trial_number = 0;
            }
        } else if(!strcmp(buf, "INV")){
            printf("Trial number is not valid\n");
        }
        
        else
            printf("ERROR\n");
    } else 
        printf("ERRORc\n");
}


void guess(){
    char word[MAX_WORD_LENGTH];
    scanf(" %s", word);
    printf("word: %s\n", word);
    sprintf(buffer, "PWG %s %s %d\n", PLID, word, trial_number);

    create_UDP_socket();

    n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    buffer[n]='\0';
    
    close_socket();

    char buf[4], *ptr = buffer;
    int trial;
    sscanf(buffer, "%s ", buf);

    if(!strcmp(buf, "RWG")){
        ptr += 4;
        sscanf(ptr, "%s ", buf);
        if(!strcmp(buf, "WIN")){
            ptr += 4;
            sscanf(ptr, "%d", &trial);
            if(trial != trial_number)
                printf("ERROR\n");
            else{
                printf("WELL DONE! You guessed: %s\n", word);
                trial_number = 0;
            }
        } else if(!strcmp(buf, "NOK")){
            ptr += 4;
            sscanf(ptr, "%d", &trial);
            if (trial != trial_number)
                printf("ERROR\n");
            else{
                printf("No, this word is not the correct answer :/\n");
                trial_number++;
            }

        } else if(!strcmp(buf, "OVR")){
            ptr += 4;
            sscanf(ptr, "%d", &trial);
            if (trial != trial_number)
                printf("ERROR\n");
            else{
                printf("No, %s is not the correct answer :/\nYou have reached the maximum error limit GG\n", word);
                trial_number = 0;
            }
        } else
            printf("ERROR\n");
    } else 
        printf("ERRORc\n");

}


void scoreboard(){
    sprintf(buffer, "GSB\n");

    create_TCP_socket();
    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    ssize_t nleft = strlen(buffer);
    char *ptr = buffer;
    while(nleft>0){
        n = write(fd, ptr, strlen(buffer));
        if(n == -1){
            fprintf(stderr, "error: %s\n", strerror(errno));
            exit(1);
        }
        nleft-=n;
        ptr+=n;
    }

    n = read_from_TCP_socket(128);
    char buf[4];
    ptr = buffer;
    sscanf(buffer, "%s ", buf);

    if(!strcmp(buf, "RSB")){
        ptr += 4;
        sscanf(ptr, "%s ", buf);
        if(!strcmp(buf, "OK")){
            char filename[22], filesize_str[8];
            ptr += 3;
            sscanf(ptr, "%s %s", filename, filesize_str);
            ssize_t filesize = atoi(filesize_str);
            printf("file name: %s, file size: %zu\n", filename, filesize);
            FILE *fp = fopen(filename, "w");
            if(fp == NULL){
                fprintf(stderr, "error: %s\n", strerror(errno));
                exit(1);
            }
            ptr += strlen(filename) + strlen(filesize_str) + 2;
            printf("%s", ptr);
            ssize_t nread = n - strlen(filename) - strlen(filesize_str) - 9;
            
            fwrite(ptr, 1, nread, fp);
            nleft = filesize - nread;
            while (nleft > 0){
                n = read_from_TCP_socket(128);
                
                nleft -= n;
                nread += n;
                fwrite(buffer, 1, n, fp);
                printf("%s", buffer);
            }
            fclose(fp);
            close_socket();
            
        } else if(!strcmp(buf, "EMPTY")){
            printf("No game was yet won by any player\n");
        }
        else {
            printf("ERROR\n");
        }
    } else {
        printf("ERROR\n");
    }
}


void hint(){
    sprintf(buffer, "GHL %s\n", PLID);

    create_TCP_socket();
    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    ssize_t nleft = strlen(buffer);
    char *ptr = buffer;
    while(nleft>0){
        n = write(fd, ptr, strlen(buffer));
        if(n == -1){
            fprintf(stderr, "error: %s\n", strerror(errno));
            exit(1);
        }
        nleft-=n;
        ptr+=n;
    }

    n = read_from_TCP_socket(128);
    char buf[4];
    ptr = buffer;
    sscanf(buffer, "%s ", buf);

    if(!strcmp(buf, "RHL")){
        ptr += 4;
        sscanf(ptr, "%s ", buf);
        if(!strcmp(buf, "OK")){
            char filename[22], filesize_str[8];

            ptr += 3;
            sscanf(ptr, "%s %s", filename, filesize_str);
            ssize_t filesize = atoi(filesize_str);

            FILE *fp = fopen(filename, "w");
            if(fp == NULL){
                fprintf(stderr, "error: %s\n", strerror(errno));
                exit(1);
            }

            ptr += strlen(filename) + strlen(filesize_str) + 2;
            ssize_t nread = n - strlen(filename) - strlen(filesize_str) - 9; // bytes already read
            fwrite(ptr, 1, nread, fp);
            nleft = filesize - nread;
            while (nleft > 0){
                n = read_from_TCP_socket(128);
                
                nleft -= n;
                fwrite(buffer, 1, n, fp);
            }
            fclose(fp);
        }
        else if(!strcmp(buf, "NOK")){
            printf("No hint file available\n");
        }
    }
    close_socket();
}


void state(){
    sprintf(buffer, "STA %s\n", PLID);

    create_TCP_socket();
    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    ssize_t nleft = strlen(buffer);
    char *ptr = buffer;
    while(nleft>0){
        n = write(fd, ptr, strlen(buffer));
        if(n == -1){
            fprintf(stderr, "error: %s\n", strerror(errno));
            exit(1);
        }
        nleft-=n;
        ptr+=n;
    }

    n = read_from_TCP_socket(128);
    char buf[4];
    ptr = buffer;
    sscanf(buffer, "%s ", buf);

    if(!strcmp(buf, "RST")){
        ptr += 4;
        sscanf(ptr, "%s ", buf);
        if(!strcmp(buf, "ACT") || !strcmp(buf, "FIN")){
            char filename[MAX_FILENAME_SIZE], filesize_str[MAX_FSIZE_SIZE];

            ptr += 4;
            sscanf(ptr, "%s %s", filename, filesize_str);
            ssize_t filesize = atoi(filesize_str);

            FILE *fp = fopen(filename, "w");
            if(fp == NULL){
                fprintf(stderr, "error: %s\n", strerror(errno));
                exit(1);
            }

            ptr += strlen(filename) + strlen(filesize_str) + 2;
            printf("%s", ptr);
            ssize_t nread = n - strlen(filename) - strlen(filesize_str) - 9; // bytes already read
            fwrite(ptr, 1, nread, fp);
            nleft = filesize - nread;
            while (nleft > 0){
                n = read_from_TCP_socket(128);
                
                nleft -= n;
                fwrite(buffer, 1, n, fp);
                printf("%s", buffer);
            }
            fclose(fp);
        }
        else if(!strcmp(buf, "NOK")){
            printf("No games active or finished for you :/\n");
        }
    }
    close_socket();

}


void quit(){
    sprintf(buffer, "QUT %s\n", PLID);

    create_UDP_socket();
    n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    trial_number=0;

    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    buffer[n]='\0';

    close_socket();

    char buf[4];
    sscanf(buffer, "%s ", buf);
    if(strcmp(buf, "RQT")==0){
        sscanf(buffer, "%*s %s ", buf);
        if(strcmp(buf, "OK") != 0)
            printf("ERROR\n");
    } else
        printf("ERROR\n");
}


int main(int argc, char** argv){
    // Read hostname and port
    for(int i=1; i < argc; i+=2){
        if(strcmp(argv[i], "-n")==0)
            hostname = argv[i+1];
        else if(strcmp(argv[i], "-p")==0){
            port = argv[i+1];
        }
    }
    if(hostname == NULL){
        char buf[128];
        if(gethostname(buf, 128)==-1)
            fprintf(stderr, "error: %s\n", strerror(errno));
        else
            hostname = buf;
    }

    trial_number = 0;

    char command[10];
    while(1){
        // Read command
        scanf("%s", command);
        if(strcmp(command, "start")==0 || strcmp(command, "sg")==0){
            start();
        }
        else if(strcmp(command, "play")==0 || strcmp(command, "pl")==0){
            play();
        }
        else if(!strcmp(command, "guess") || !strcmp(command, "gw")){
            guess();
        }
        else if(!strcmp(command, "scoreboard") || !strcmp(command, "sb")){
            scoreboard();
        }
        else if(!strcmp(command, "hint") || !strcmp(command, "h")){
            hint();
        }
        else if(!strcmp(command, "state") || !strcmp(command, "st")){
            state();
        }
        else if(strcmp(command, "quit")==0 || strcmp(command, "q")==0){
            if(trial_number>0)
                quit();
        }
        else if(strcmp(command, "exit")==0){
            if(trial_number>0)
                quit();
            break;
                
        }
        else{
            printf("ERROR\n");
        }
    }
}

// No final pesquisar por 'ERROR' e ver se é para aterar algo; exit(1)
