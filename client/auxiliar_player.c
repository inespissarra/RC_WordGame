#include "constants.h"
#include "auxiliar_player.h"

int fd, errno, errcode, afd = 0;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;


void createUDPsocket(char* hostname, char* port){
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    
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

    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

void createTCPsocket(char* hostname, char* port){
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    
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
    
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

void closeSocket(){
    freeaddrinfo(res);
    close(fd);
}

int sendAndReadUDP(char *buffer, char *hostname, char *port){
    createUDPsocket(hostname, port);

    n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        printf(SEND_FAILED);
        return 0;
    }

    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, MAX_READ_SIZE, 0, (struct sockaddr*)&addr, &addrlen);
    if(n == -1){
        printf(RECEIVE_FAILED);
        return 0;
    }
    buffer[n]='\0';
    
    closeSocket();
    return 1;
}

int sendAndReadTCP(char *buffer){
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
            printf(SEND_FAILED);
            return 0;
        }
        nleft-=n; ptr+=n;
    }

    return readUntilSpace(buffer);
}

ssize_t readFromTCPsocket(char *buffer, ssize_t nleft){
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

void readFile(int command, char *buffer){
    char filename[MAX_FILENAME_SIZE + 1], filesize_str[MAX_FSIZE_SIZE + 1];

    if(!readUntilSpace(filename) || !readUntilSpace(filesize_str))
        return;

    ssize_t filesize = atoi(filesize_str);

    FILE *fp = fopen(filename, "w");
    if(fp == NULL){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    ssize_t nleft = filesize;

    while (nleft > 0){
        if(nleft > MAX_READ_SIZE)
            n = readFromTCPsocket(buffer, MAX_READ_SIZE);
        else
            n = readFromTCPsocket(buffer, nleft);
        nleft -= n;
        fwrite(buffer, 1, n, fp);
        if (command == STATE || command == SCOREBOARD)
            printf("%s", buffer);
    }
    fclose(fp);
    while((n = read(fd, buffer, 1))==0){
        if(n == -1){
            printf(RECEIVE_FAILED);
            exit(1);
        }
    }

    if(command == HINT)
        printf(RECEIVED_HINT, filename, filesize);
    else if(command == STATE)
        printf(RECEIVED_STATE, filename, filesize);
    else if(command == SCOREBOARD)
        printf(RECEIVED_SCOREBOARD, filename, filesize);
}

int readUntilSpace(char *ptr){
    do {
        n = read(fd, ptr, 1);
        if(n == -1){
            printf(RECEIVE_FAILED);
            return 0;
        }
        ptr+=n;
    } while(*(ptr-1)!=' ' && *(ptr-1)!='\n');
    *(ptr-1) = '\0';
    return 1;
}

int isNumeric(char *str){
    while(*str){
        if(!isdigit(*str))
            return 0;
        str++;
    }
    return 1;
}


void start(char* hostname, char* port, char *buffer, char *PLID, char *game, int *trial_number, int *errors){
    if(!scanf(" %s", PLID) || strlen(PLID) != MAX_PLID_SIZE || !isNumeric(PLID)){
        printf(INVALID_PLID);
        return;
    }

    sprintf(buffer, "SNG %s\n", PLID);

    if(!sendAndReadUDP(buffer, hostname, port))
        return;

    char buf1[4], buf2[4], n;
    int n_let, max_errors, i;
    i = sscanf(buffer, "%s %s ", buf1, buf2);
    if(i==2 && !strcmp(buf1, "RSG")){
        if(!strcmp(buf2, "OK")){
            i = sscanf(buffer, "%*s %*s %d %d%c",&n_let, &max_errors, &n);
            if(i==3 && n=='\n'){
                *errors = max_errors;
                memset(game, '_', n_let);
                game[n_let] = '\0';
                printf(NEW_GAME, *errors, game);
                *trial_number = 1;
                return;
            }
        }
        else if(!strcmp(buf2, "NOK")){
            i = sscanf(buffer, "%*s %*s%c", &n);
            if(n=='\n'){
                printf(GAME_ALREADY_STARTED);
                return;
            }
        } else if(!strcmp(buf2, "ERR"))
            i = sscanf(buffer, "%*s %*s%c", &n);
            if(n=='\n'){
                printf(ERROR);
                return;
            }
    }
    else if(!strcmp(buf1, "ERR")){
        i = sscanf(buffer, "%*s%c", &n);
        if(n=='\n'){
            printf(ERROR);
            return;
        }
    }
    printf(FORMAT_ERROR);
}


void play(char* hostname, char* port, char *buffer, char *PLID, char *game, int *trial_number, int *errors){
    char letter;
    if(!scanf(" %c", &letter) || !isalpha(letter)){
        printf(INVALID_LETTER);
        return;
    }
    letter = tolower(letter);
    sprintf(buffer, "PLG %s %c %d\n", PLID, letter, *trial_number);
    
    if(!sendAndReadUDP(buffer, hostname, port))
        return;

    char buf[4], *ptr = buffer, n;
    int trial, i;
    i = sscanf(buffer, "%s ", buf);
    if(i==1 && !strcmp(buf, "RLG")){
        ptr += 4;
        sscanf(ptr, "%s ", buf);
        if(!strcmp(buf, "OK")){
            int n_let;
            char pos[3];
            ptr += 3;
            i = sscanf(ptr, "%d %d ", &trial, &n_let);
            if(i!=2)
                printf(FORMAT_ERROR);
            else if (trial != *trial_number)
                printf(INVALID_TRIAL);
            else {
                *trial_number = *trial_number + 1;
                ptr += 4;
                for(int j = 0; j<n_let; j++){
                    sscanf(ptr, "%s ", pos);
                    game[atoi(pos) - 1] = letter;
                    ptr += strlen(pos) + 1;
                }
                printf(GOOD_PLAY, letter, *errors, game);
            }
        } else if(!strcmp(buf, "NOK")){
            ptr += 4;
            i = sscanf(ptr, "%d%c", &trial, &n);
            if(i!=2 || n!='\n')
                printf(FORMAT_ERROR);
            else if (trial != *trial_number)
                printf(INVALID_TRIAL);
            else{
                *errors = *errors - 1;
                printf(BAD_PLAY, letter, *errors, game);
                *trial_number = *trial_number + 1;
            }

        } else if(!strcmp(buf, "WIN")){
            ptr += 4;
            i = sscanf(ptr, "%d%c", &trial, &n);
            if(i!=2 || n!='\n')
                printf(FORMAT_ERROR);
            else if (trial != *trial_number)
                printf(INVALID_TRIAL);
            else{
                for(int i=0; i < strlen(game); i++){
                    if(game[i] == '_')
                        game[i] = letter;
                }
                printf(GOOD_GUESS, game);
                *trial_number = 0;
            }

        } else if(!strcmp(buf, "DUP")){
            ptr += 4;
            i = sscanf(ptr, "%d%c", &trial, &n);
            if(i!=2 || n!='\n')
                printf(FORMAT_ERROR);
            else if (trial != ((*trial_number)))
                printf(INVALID_TRIAL);
            else
                printf(DUP_PLAY);
        } else if(!strcmp(buf, "OVR")){
            ptr += 4;
            i = sscanf(ptr, "%d%c", &trial, &n);
            if(i!=2 || n!='\n')
                printf(FORMAT_ERROR);
            else if (trial != *trial_number)
                printf(INVALID_TRIAL);
            else{
                *errors = *errors - 1;
                printf(BAD_PLAY, letter, *errors, game);
                printf(GAME_OVER);
                *trial_number = 0;
            }
        } else if(!strcmp(buf, "INV"))
            printf(INVALID_TRIAL);
        else if(!strcmp(buf, "ERR"))
            printf(ERROR);
        else
            printf(FORMAT_ERROR);
    } else if(!strcmp(buf, "ERR"))
            printf(UNEXPECTED_COMMAND);
    else 
        printf(FORMAT_ERROR);
}


int validGuess(char* word){
    for(int i = 0; i < strlen(word); i++){
        if(!isalpha(word[i])){
            return 0;
        }
    }
    return 1;
}

void toLowerCase(char str[]){
    for(int i=0; i<strlen(str); i++){
        str[i] = tolower(str[i]);
    }
}


void guess(char* hostname, char* port, char *buffer, char *PLID, int *trial_number, int *errors){
    char word[MAX_WORD_LENGTH + 1];
    if(!scanf(" %s", word) || !validGuess(word) || strlen(word) < MIN_WORD_LENGTH || strlen(word) > MAX_WORD_LENGTH){
        printf(INVALID_WORD);
        return;
    }
    toLowerCase(word);
    sprintf(buffer, "PWG %s %s %d\n", PLID, word, *trial_number);

    if(!sendAndReadUDP(buffer, hostname, port))
        return;

    char buf[4], *ptr = buffer, n;
    int trial, i;
    i = sscanf(buffer, "%s ", buf);

    if(i==1 && !strcmp(buf, "RWG")){
        ptr += 4;
        i = sscanf(ptr, "%s ", buf);
        if(i==1 && !strcmp(buf, "WIN")){
            ptr += 4;
            i = sscanf(ptr, "%d%c", &trial, &n);
            if(i!=2 || n!='\n')
                printf(FORMAT_ERROR);
            else if(trial != *trial_number)
                printf(INVALID_TRIAL);
            else{
                printf(GOOD_GUESS, word);
                *trial_number = 0;
            }
        } else if(!strcmp(buf, "NOK")){
            ptr += 4;
            i = sscanf(ptr, "%d%c", &trial, &n);
            if(i!=2 || n!='\n')
                printf(FORMAT_ERROR);
            else if (trial != *trial_number)
                printf(INVALID_TRIAL);
            else{
                *errors = *errors - 1;
                printf(BAD_GUESS, word, *errors);
                *trial_number = *trial_number + 1;
            }

        } else if(!strcmp(buf, "OVR")){
            ptr += 4;
            i = sscanf(ptr, "%d%c", &trial, &n);
            if(i!=2 || n!='\n')
                printf(FORMAT_ERROR);
            else if (trial != *trial_number)
                printf(INVALID_TRIAL);
            else{
                *errors = *errors - 1;
                printf(BAD_GUESS, word, *errors);
                printf(GAME_OVER);
                *trial_number = 0;
            }
        } else if(!strcmp(buf, "ERR"))
            printf(ERROR); 
        else
            printf(FORMAT_ERROR);
    } else if(!strcmp(buf, "ERR"))
        printf(UNEXPECTED_COMMAND); 
    else 
        printf(FORMAT_ERROR);

}


void scoreboard(char* hostname, char* port, char *buffer, char *PLID){
    sprintf(buffer, "GSB\n");

    createTCPsocket(hostname, port);
    if(!sendAndReadTCP(buffer))
        return;

    if(!strcmp(buffer, "RSB")){
        if(!readUntilSpace(buffer))
            return;
        if(!strcmp(buffer, "OK")){
            readFile(1, buffer);
        } else if(!strcmp(buffer, "EMPTY")){
            printf(NO_SCORES);
        } else if(!strcmp(buffer, "ERR"))
            printf(ERROR);
        else {
            printf(FORMAT_ERROR);
        }
    } else if(!strcmp(buffer, "ERR"))
        printf(ERROR);
    else
        printf(FORMAT_ERROR);

    closeSocket();
}


void hint(char* hostname, char* port, char *buffer, char *PLID){
    sprintf(buffer, "GHL %s\n", PLID);

    createTCPsocket(hostname, port);
    if(!sendAndReadTCP(buffer))
        return;

    if(!strcmp(buffer, "RHL")){
        if(!readUntilSpace(buffer))
            return;
        if(!strcmp(buffer, "OK"))
            readFile(0, buffer);
        else if(!strcmp(buffer, "NOK"))
            printf(NO_HINT);
        else if(!strcmp(buffer, "ERR"))
            printf(ERROR);
        else
            printf(FORMAT_ERROR);
    } else if(!strcmp(buffer, "ERR"))
        printf(ERROR);
    else
        printf(FORMAT_ERROR);
    closeSocket();
}


void state(char* hostname, char* port, char *buffer, char *PLID, int *trial_number){
    sprintf(buffer, "STA %s\n", PLID);

    createTCPsocket(hostname, port);
    if(!sendAndReadTCP(buffer))
        return;

    if(!strcmp(buffer, "RST")){
        if(!readUntilSpace(buffer))
            return;
        if(!strcmp(buffer, "ACT")){
            readFile(1, buffer);
        } else if(!strcmp(buffer, "FIN")){
            readFile(1, buffer);
            *trial_number = 0;
        } else if(!strcmp(buffer, "NOK")){
            printf(NO_STATE);
        } else if(!strcmp(buffer, "ERR"))
            printf(ERROR);
        else
            printf(FORMAT_ERROR);
    } else if(!strcmp(buffer, "ERR"))
        printf(ERROR);
    else {
        printf(FORMAT_ERROR);
    }
    closeSocket();

}


void quit(char* hostname, char* port, char *buffer, char *PLID, int *trial_number){
    sprintf(buffer, "QUT %s\n", PLID);

    if(!sendAndReadUDP(buffer, hostname, port))
        return;

    char buf[4], n;
    sscanf(buffer, "%s ", buf);
    if(!strcmp(buf, "RQT")){
        sscanf(buffer, "%*s %s%c", buf, &n);
        if(n=='\n' && !strcmp(buf, "OK")){
            *trial_number=0;
            printf(QUIT);
        }
        else if(n=='\n' && !strcmp(buf, "NOK"))
            printf(NO_GAME);
        else if(!strcmp(buf, "ERR"))
            printf(ERROR);
        else
            printf(FORMAT_ERROR);
        
    } else if(!strcmp(buf, "ERR"))
        printf(ERROR); 
    else
        printf(FORMAT_ERROR);
}
