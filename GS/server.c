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
int offset = 0;
char buffer[MAX_READ_SIZE + 1];
char *port = "58082";
int verbose = 0;

void get_new_word(){
    FILE *fp = fopen(word_file, "r");
    if(fp==NULL){
        printf("ERROR\n");
        exit(1);
    }
    fseek(fp, offset, SEEK_SET);
    if(fgets(buffer, MAX_READ_SIZE, fp)==NULL){
        rewind(fp);
        fgets(buffer, MAX_READ_SIZE, fp);
    }
    offset += strlen(buffer);
    fclose(fp);
}


void start(){
    char *ptr = buffer + 4;
    char word[MAX_WORD_LENGTH + 1];
    char PLID[MAX_PLID_SIZE + 1];
    sscanf(ptr, "%s", PLID);
    FILE *fp;
    char filename[MAX_PLAYER_FILENAME_SIZE];
    int max_errors;
    sprintf(filename, "GAME_%s.txt", PLID);
    if(!access(filename, F_OK)){ // File exists
        fp = fopen(filename, "r");
        fgets(buffer, MAX_READ_SIZE, fp);
        sscanf(buffer, "%s", word);
        fgets(buffer, MAX_READ_SIZE, fp);
        int trial_number;
        sscanf(buffer, "%d %d", &trial_number, &max_errors);
        if(trial_number!=1){ // Check if no play was yet received
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
        get_new_word();
        sscanf(buffer, "%s", word);
        int len = strlen(word);
        if(len < 7)
            max_errors = 7;
        else if(len < 11)
            max_errors = 8;
        else
            max_errors = 9;
        fp = fopen(filename, "w");
        fprintf(fp, "%s", buffer);
        sprintf(buffer, "1 %d ", max_errors); // Trial number, attempts left  // dois espaços kkkkk como melhorar?
        memset(buffer + 4, '_', strlen(word)); // state of the game
        buffer[4 + strlen(word)] = '\0';
        fprintf(fp, "%s\n", buffer);
        fclose(fp);
    }

    int len = strlen(word);

    sprintf(buffer, "RSG OK %d %d\n", len, max_errors);
    
    n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
}

int correct_trial_not_duplicate(char *filename, char letter, int trial_number){
    FILE *fp = fopen(filename, "r");
    if(fp == NULL){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    fgets(buffer, MAX_READ_SIZE, fp);
    char word[MAX_WORD_LENGTH + 1];
    sscanf(buffer, "%s", word);
    fgets(buffer, MAX_READ_SIZE, fp);
    int trial_number2;
    sscanf(buffer, "%d", &trial_number2);
    printf("%d %d\n", trial_number, trial_number2);
    if(trial_number2==trial_number){ // ver se existem duplicados
        for(int i = 0; i < trial_number; i++){
            fgets(buffer, MAX_READ_SIZE, fp);
            char code, letter2;
            sscanf(buffer, "%c %c", &code, &letter2);
            if(code=='T' && letter2 == letter){
                sprintf(buffer, "RLG DUP %d\n", trial_number2);
                fclose(fp);
                return 0;
            }
        }
        fclose(fp);
        return 1; // nova jogada
    } else if(trial_number==trial_number2-1){ //mandou a jogada anterior de novo?
        for(int i = 0; i < trial_number; i++){
            fgets(buffer, MAX_READ_SIZE, fp);
        }
        char code, letter2;
        sscanf(buffer, "%c %c", &code, &letter2);
        if(code=='T' && letter2 == letter){
            fclose(fp);
            return 2;  // jogada antiga
        }
    }
    fclose(fp);
    sprintf(buffer, "RLG INV %d\n", trial_number2);
    return 0;
}

void actually_play(char *filename, char letter, int trial_number, int write){
    FILE *fp = fopen(filename, "r+");
    if(fp == NULL){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    fgets(buffer, MAX_READ_SIZE, fp);
    int offset = strlen(buffer);
    char word[MAX_WORD_LENGTH + 1];
    sscanf(buffer, "%s", word);
    int n = 0;
    char positions[MAX_WORD_LENGTH];
    for(int i = 0; i < strlen(word); i++){
        if(word[i] == letter){
            positions[n] = i;
            n++;
        }
    }
    if(n > 0){
        if(write){
            fgets(buffer, MAX_READ_SIZE, fp);
            int max_errors;
            sscanf(buffer, "%*d %d %s", &max_errors, word);
            for(int i=0; i < n; i++){
                word[positions[i]] = letter;
            }
            if(strchr(word, '_')==NULL){
                sprintf(buffer, "%d %d %s\n", trial_number, max_errors, word);
                fseek(fp, offset, SEEK_SET);
                fprintf(fp, "%s", buffer);
                sprintf(buffer, "RLG WIN %d\n", trial_number);
                 // meter o ficheiro no sitio certo
                return;
            }
            sprintf(buffer, "%d %d %s\n", trial_number + 1, max_errors, word);
            fseek(fp, offset, SEEK_SET);
            fprintf(fp, "%s", buffer);
        }
        sprintf(buffer, "RLG OK %d %d ", trial_number, n);
        for(int i=0; i < n; i++){
            printf("%d %d\n", n, positions[i]);
            char buf[4];
            sprintf(buf, "%d ", positions[i] + 1);
            strcat(buffer, buf);

        }
        strcat(buffer, "\n");
    } else{
        int max_errors;
        if(write){
            fgets(buffer, MAX_READ_SIZE, fp);
            int trial_number2;
            sscanf(buffer, "%d %d %s", &trial_number2, &max_errors, word);
            fseek(fp, offset, SEEK_SET);
            fprintf(fp, "%d %d %s\n", trial_number2 + 1, max_errors - 1, word);
            if(max_errors - 1 == 0){
                sprintf(buffer, "RLG OVR %d\n", trial_number2 + 1);
                // meter o ficheiro no sitio certo
                return;
            }
        }
        sprintf(buffer, "RLG NOK %d\n", trial_number);
    }
    fclose(fp);
}

void adicionar_jogada(char *filename, char code, char letter){
    FILE *fp = fopen(filename, "a");
    if(fp == NULL){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    fprintf(fp, "%c %c\n", code, letter);
    fclose(fp);
}

void play(){
    char *ptr = buffer + 4;
    char letter;
    int trial_number;
    char word[MAX_WORD_LENGTH + 1];
    char PLID[MAX_PLID_SIZE + 1];
    n = sscanf(ptr, "%s %c %d\n", PLID, &letter, &trial_number);
    if(n==3){ // Correct format
        char filename[MAX_PLAYER_FILENAME_SIZE];
        sprintf(filename, "GAME_%s.txt", PLID);
        if(!access(filename, F_OK)){ // File exists
            int result = correct_trial_not_duplicate(filename, letter, trial_number);
            if(result==1){
                adicionar_jogada(filename, 'T', letter);
                actually_play(filename, letter, trial_number, 1);
            } else if(result == 2){
                actually_play(filename, letter, trial_number, 0);
            }
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
    printf("Sent: %s\n", buffer);
}

void guess(){
    char *ptr = buffer + 4;
    char guess[MAX_WORD_LENGTH + 1];
    char PLID[MAX_PLID_SIZE + 1];
    int trial_number;
    n = sscanf(ptr, "%s %s %d\n", PLID, guess, &trial_number);
    if(n==3){ // Correct format
        char filename[MAX_PLAYER_FILENAME_SIZE];
        sprintf(filename, "GAME_%s.txt", PLID);
        if(!access(filename, F_OK)){ // File exists
            FILE *fp = fopen(filename, "r+");
            char word[MAX_WORD_LENGTH + 1];
            if(fp == NULL){
                fprintf(stderr, "error: %s\n", strerror(errno));
                exit(1);
            }
            fgets(buffer, MAX_READ_SIZE, fp);
            sscanf(buffer, "%s", word);
            int offset = strlen(buffer);
            fgets(buffer, MAX_READ_SIZE, fp);
            int trial_number2;
            int max_errors;

            sscanf(buffer, "%d %d", &trial_number2, &max_errors);
            if(trial_number2 == trial_number){
                fseek(fp, 0, SEEK_END);
                fprintf(fp, "G %s\n", guess);
                if(strcmp(word, guess)==0){
                    fseek(fp, offset, SEEK_SET);
                    fprintf(fp, "%d %d %s\n", trial_number, max_errors, word);
                    sprintf(buffer, "RWG WIN %d\n", trial_number);
                    // meter o ficheiro no sitio certo
                } else if(max_errors - 1 == 0){
                    sscanf(buffer, "%*d %*d %s", word);
                    fseek(fp, offset, SEEK_SET);
                    fprintf(fp, "%d %d %s\n", trial_number + 1, max_errors - 1, word);
                    sprintf(buffer, "RWG OVR %d\n", trial_number);
                } else {
                    sscanf(buffer, "%*d %*d %s", word);
                    fseek(fp, offset, SEEK_SET);
                    fprintf(fp, "%d %d %s\n", trial_number + 1, max_errors - 1, word);
                    sprintf(buffer, "RWG NOK %d\n", trial_number);
                }
            } else{
                sprintf(buffer, "RWG INV %d\n", trial_number);
            }
            fclose(fp);
        } else{
            sprintf(buffer, "RWG ERR\n");
        }
    } else{
        sprintf(buffer, "RWG ERR\n");
    }

    n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    printf("Sent: %s\n", buffer);
}

void quit(){
    char *ptr = buffer + 4;
    char PLID[MAX_PLID_SIZE + 1];
    n = sscanf(ptr, "%s\n", PLID);
    char filename[MAX_PLAYER_FILENAME_SIZE];
    sprintf(filename, "GAME_%s.txt", PLID);
    if(!access(filename, F_OK)){ // File exists
        // alterar o ficheiro para o nome certo
        sprintf(buffer, "RQT OK\n");
        //remove(filename);
    } else{
        sprintf(buffer, "RQT ERR\n");
    }

    n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    printf("Sent: %s\n", buffer);
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
        
        if(verbose)
            printf("Received: %s\n", buffer);

        sscanf(buffer, "%s", command);
        if(!strcmp(command, "SNG")){
            start();
        } else if(!strcmp(command, "PLG")){
            play();
        } else if(!strcmp(command, "PWG")){
            guess();
        } else if(!strcmp(command, "QUT")){
            quit();
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