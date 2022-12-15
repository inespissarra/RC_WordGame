#include "constants.h"
#include "auxiliar_player.h"

int fd, errno, errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;


void create_UDP_socket(char* hostname, char* port){
    int timeout = TIMEOUT;
    
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

    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char *) &timeout, sizeof(timeout));
}

void create_TCP_socket(char* hostname, char* port){
    int timeout = TIMEOUT;
    
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
    
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char *) &timeout, sizeof(timeout));
}

void close_socket(){
    freeaddrinfo(res);
    close(fd);
}

ssize_t read_from_TCP_socket(char *buffer, ssize_t nleft){
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


void start(char* hostname, char* port, char *buffer, char *PLID, char *game, int *trial_number){
    scanf("%s", PLID);
    sprintf(buffer, "SNG %s\n", PLID);

    create_UDP_socket(hostname, port);

    n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, MAX_READ_SIZE, 0, (struct sockaddr*)&addr, &addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    buffer[n]='\0';
    close_socket();

    char buf1[4], buf2[4];
    int n_let, errors;
    sscanf(buffer, "%s %s %d %d\n", buf1, buf2, &n_let, &errors);
    if(!strcmp(buf1, "RSG")){
        if(!strcmp(buf2, "OK")){
            memset(game, '_', n_let);
            game[n_let] = '\0';
            printf("New game started (max %d errors): %s\n", errors, game);
            *trial_number = 1;
        }
        else if(!strcmp(buf2, "NOK"))
            printf("Error: game already started\n");
        else
            printf("ERROR\n");
    }
    else{
        printf("ERROR\n");
    }
}


void play(char* hostname, char* port, char *buffer, char *PLID, char *game, int *trial_number){
    char letter;
    scanf(" %c", &letter);
    sprintf(buffer, "PLG %s %c %d\n", PLID, letter, *trial_number);

    create_UDP_socket(hostname, port);

    n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, MAX_READ_SIZE, 0, (struct sockaddr*)&addr, &addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    buffer[n]='\0';
    
    close_socket();

    char buf[4], *ptr = buffer;
    int trial;
    sscanf(buffer, "%s ", buf);
    if(!strcmp(buf, "RLG")){
        ptr += 4;
        sscanf(ptr, "%s ", buf);
        if(!strcmp(buf, "OK")){
            int n_let;
            char pos[3];
            ptr += 3;
            sscanf(ptr, "%d %d", &trial, &n_let);
            if (trial != *trial_number)
                printf("ERROR\n");
            else {
                *trial_number = *trial_number + 1;
                ptr += 4;
                for(int i = 0; i<n_let; i++){
                    sscanf(ptr, "%s ", pos);
                    game[atoi(pos) - 1] = letter;
                    ptr += strlen(pos) + 1;
                }
                printf("Yes, \"%c\" is part of the word: %s\n", letter, game);
            }

        } else if(!strcmp(buf, "NOK")){
            ptr += 4;
            sscanf(ptr, "%d", &trial);
            if (trial != *trial_number)
                printf("ERROR\n");
            else{
                printf("No, \"%c\" is not part of the word: %s\n", letter, game);
                *trial_number = *trial_number + 1;
            }

        } else if(!strcmp(buf, "WIN")){
            ptr += 4;
            sscanf(ptr, "%d", &trial);
            if(trial != *trial_number)
                printf("ERROR\n");
            else{
                for(int i=0; i < strlen(game); i++){
                    if(game[i] == '_')
                        game[i] = letter;
                }
                printf("WELL DONE! You guessed: %s\n", game);
                *trial_number = 0;
            }

        } else if(!strcmp(buf, "DUP")){
            ptr += 4;
            sscanf(ptr, "%d", &trial);
            if (trial != ((*trial_number))) // - 1?
                printf("ERROR\n");
            else
                printf("The letter has been inserted before\n");
        } else if(!strcmp(buf, "OVR")){
            ptr += 4;
            sscanf(ptr, "%d", &trial);
            if (trial != *trial_number)
                printf("TRIAL NUMBER ERROR\n");
            else{
                printf("No, \"%c\" is not part of the word: %s\nYou have reached the maximum error limit GG\n", letter, game);
                *trial_number = 0;
            }
        } else if(!strcmp(buf, "INV")){
            printf("Trial number is not valid\n");
        }
        
        else
            printf("ERROR\n");
    } else 
        printf("ERROR\n");
}


void guess(char* hostname, char* port, char *buffer, char *PLID, int *trial_number){
    char word[MAX_WORD_LENGTH +1];
    scanf(" %s", word);
    sprintf(buffer, "PWG %s %s %d\n", PLID, word, *trial_number);

    create_UDP_socket(hostname, port);

    n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, MAX_READ_SIZE, 0, (struct sockaddr*)&addr, &addrlen);
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
            if(trial != *trial_number)
                printf("ERROR\n");
            else{
                printf("WELL DONE! You guessed: %s\n", word);
                *trial_number = 0;
            }
        } else if(!strcmp(buf, "NOK")){
            ptr += 4;
            sscanf(ptr, "%d", &trial);
            if (trial != *trial_number)
                printf("ERROR\n");
            else{
                printf("No, this word is not the correct answer :/\n");
                *trial_number = *trial_number + 1;
            }

        } else if(!strcmp(buf, "OVR")){
            ptr += 4;
            sscanf(ptr, "%d", &trial);
            if (trial != *trial_number)
                printf("ERROR\n");
            else{
                printf("No, %s is not the correct answer :/\nYou have reached the maximum error limit GG\n", word);
                *trial_number = 0;
            }
        } else
            printf("ERROR\n");
    } else 
        printf("ERROR\n");

}


void read_file(int print, char *buffer){
    char filename[MAX_FILENAME_SIZE], filesize_str[MAX_FSIZE_SIZE];

    read_until_space(filename);
    read_until_space(filesize_str);

    ssize_t filesize = atoi(filesize_str);

    FILE *fp = fopen(filename, "w");
    if(fp == NULL){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    ssize_t nleft = filesize;

    while (nleft > 0){
        if(nleft > MAX_READ_SIZE)
            n = read_from_TCP_socket(buffer, MAX_READ_SIZE);
        else
            n = read_from_TCP_socket(buffer, nleft);
        nleft -= n;
        fwrite(buffer, 1, n, fp);
        if (print)
            printf("%s", buffer);
    }
    fclose(fp);
    if(!print)
        printf("Hint file received: %s %zu\n", filename, filesize);
}

void read_until_space(char *ptr){
    do {
        n = read(fd, ptr, 1);
        if(n == -1){
            printf("ERROR\n");
            exit(1);
        }
        ptr+=n;
    } while(*(ptr-1)!=' ' && *(ptr-1)!='\n');
    *(ptr-1) = '\0';
}


void scoreboard(char* hostname, char* port, char *buffer, char *PLID){
    sprintf(buffer, "GSB\n");

    create_TCP_socket(hostname, port);
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
        nleft-=n; ptr+=n;
    }

    read_until_space(buffer);

    if(!strcmp(buffer, "RSB")){
        read_until_space(buffer);
        if(!strcmp(buffer, "OK")){
            read_file(1, buffer);
        } else if(!strcmp(buffer, "EMPTY")){
            printf("No game was yet won by any player\n");
        }
        else {
            printf("ERROR\n");
        }
    } else {
        printf("ERROR\n");
    }
    close_socket();
}


void hint(char* hostname, char* port, char *buffer, char *PLID){
    sprintf(buffer, "GHL %s\n", PLID);

    create_TCP_socket(hostname, port);
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
        nleft-=n; ptr+=n;
    }

    read_until_space(buffer);
    if(!strcmp(buffer, "RHL")){
        read_until_space(buffer);
        if(!strcmp(buffer, "OK")){
            read_file(0, buffer);
        }
        else if(!strcmp(buffer, "NOK")){
            printf("No hint file available\n");
        }
    } else {
        printf("ERROR\n");
    }
    close_socket();
}


void state(char* hostname, char* port, char *buffer, char *PLID){
    sprintf(buffer, "STA %s\n", PLID);

    create_TCP_socket(hostname, port);
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
        nleft-=n; ptr+=n;
    }

    read_until_space(buffer);

    if(!strcmp(buffer, "RST")){
        read_until_space(buffer);
        if(!strcmp(buffer, "ACT") || !strcmp(buffer, "FIN")){
            read_file(1, buffer);
        }
        else if(!strcmp(buffer, "NOK")){
            printf("No games active or finished for you :/\n");
        }
    } else {
        printf("ERROR\n");
    }
    close_socket();

}


void quit(char* hostname, char* port, char *buffer, char *PLID, int *trial_number){
    sprintf(buffer, "QUT %s\n", PLID);

    create_UDP_socket(hostname, port);
    n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    *trial_number=0;

    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, MAX_READ_SIZE, 0, (struct sockaddr*)&addr, &addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    buffer[n]='\0';

    close_socket();

    char buf[4];
    sscanf(buffer, "%s ", buf);
    if(!strcmp(buf, "RQT")){
        sscanf(buffer, "%*s %s ", buf);
        if(strcmp(buf, "OK") != 0)
            printf("ERROR\n");
    } else
        printf("ERROR\n");
}
