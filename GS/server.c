#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#include "constants.h"


int fd, newfd, errno, errcode, offset = 0;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;

char *word_file;
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


void finish_game(char *PLID, char *filename, char state){
    char buf[MAX_FILENAME_SIZE];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(buf, "GAMES/%s", PLID);
    mkdir(buf, 0777); //-nao da de outra maneira?
    sprintf(buf, "GAMES/%s/%d%d%d_%d%d%d_%c.txt", PLID, tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, state);
    rename(filename, buf);
}


void start(){
    char *ptr = buffer + 4;
    char word[MAX_WORD_LENGTH + 1];
    char PLID[MAX_PLID_SIZE + 1];
    sscanf(ptr, "%s", PLID);
    FILE *fp;
    char filename[MAX_FILENAME_SIZE];
    sprintf(filename, "GAMES/GAME_%s.txt", PLID);

    if(!access(filename, F_OK)){ 
        // File exists
        fp = fopen(filename, "r");
        fgets(buffer, MAX_READ_SIZE, fp);
        sscanf(buffer, "%s", word);

        if(fgets(buffer, MAX_READ_SIZE, fp)!=NULL){ 
            // No play was yet received
            fclose(fp);
            sprintf(buffer, "RSG NOK\n");
            n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addrlen);
            if(n == -1){
                fprintf(stderr, "error: %s\n", strerror(errno));
                exit(1);
            }
            if (verbose)
                printf("Sent: %s\n", buffer);
            return;
        }
        fclose(fp);

    } else{
        get_new_word();
        sscanf(buffer, "%s", word);
        fp = fopen(filename, "w");
        fprintf(fp, "%s", buffer);
        fclose(fp);
    }

    int max_errors, len = strlen(word);
    
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
    if (verbose)
        printf("Sent: %s\n", buffer);
}


int count_unique_char(char word[MAX_WORD_LENGTH]){
    char unique[MAX_WORD_LENGTH];
    int length = strlen(word);
    for (int i=0; i<length; i++){
        if (strchr(unique, word[i])==NULL)
            unique[strlen(unique)] = word[i];
    }
    return strlen(unique);
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
        char filename[MAX_FILENAME_SIZE];

        sprintf(filename, "GAMES/GAME_%s.txt", PLID);

        if((fp = fopen(filename, "r+")) != NULL){
            fgets(buffer, MAX_READ_SIZE, fp);
            
            sscanf(buffer, "%s %*s", word); // get the word
            printf("%s \n%s\n", buffer, word);
            int num_unique = count_unique_char(word), max_errors, len = strlen(word);

            if(len < 7)
                max_errors = 7;
            else if(len < 11)
                max_errors = 8;
            else
                max_errors = 9;
            
            int n_trials = 1, errors = 0, corrects=0, dup = 0;
            char previous[MAX_WORD_LENGTH], code, trial_list[MAX_WORD_LENGTH];
            while(fgets(buffer, MAX_WORD_LENGTH+3, fp) != NULL){
                sscanf(buffer, "%c %s", &code, previous);
                if (code == 'T'){
                    printf("previous code: %s\n", previous);
                    // if the letter was sent in a previous trial
                    if (previous[0] == letter)
                        dup = 1;
                    if (strchr(word, previous[0])!=NULL)
                        corrects ++;
                }
                
                if ((code=='T' && strchr(word, previous[0])==NULL) || (code=='G' && strcmp(word,previous)))
                    errors++;
                
                n_trials++;
            }

            if (n_trials != trial_number){
                sprintf(buffer, "RLG INV %d\n", n_trials);
            }
            else {
                if (dup == 1)
                    sprintf(buffer, "RLG DUP %d\n", n_trials);
        
                else {
                    printf("%s %c\n", word, letter);
                    if (strchr(word, letter)==NULL){
                        if (errors < max_errors){
                            printf("%s %c\n", word, letter);
                            sprintf(buffer, "RLG NOK %d\n", n_trials);
                        }
                        else if (errors == max_errors){
                            sprintf(buffer, "RLG OVR %d\n", n_trials);
                            finish_game(PLID, filename, 'F');
                        }
                    }
                    else if (strchr(word, letter)!=NULL){
                        if (corrects+1 == num_unique){
                            sprintf(buffer, "RLG WIN %d\n", n_trials);
                            finish_game(PLID, filename, 'W');
                        }
                        else{
                            int length = strlen(word), occ_len;
                            char indexes[MAX_WORD_LENGTH*4], pos[4];
                            indexes[0] = '\0';
                            for (int i = 0; i<length; i++){
                                if (word[i] == letter){
                                    sprintf(pos, "%d ", i+1);
                                    strcat(indexes, pos);
                                }
                            }
                            printf("pos selected: %s\n", indexes);
                            sprintf(buffer, "RLG OK %d %ld %s\n", n_trials, strlen(indexes)/2, indexes);
                        }
                    }
                    char line[MAX_WORD_LENGTH+MAX_FILENAME_SIZE+1];
                    sprintf(line, "T %c\n", letter);
                    fputs(line, fp);
                }
            }
            fclose(fp);

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
    if (verbose)
        printf("Sent: %s\n", buffer);
}


void guess(){
    char *ptr = buffer + 4;
    char letter;
    int trial_number;
    char word[MAX_WORD_LENGTH + 1];
    char PLID[MAX_PLID_SIZE + 1], guess[MAX_WORD_LENGTH];

    n = sscanf(ptr, "%s %s %d\n", PLID, guess, &trial_number);

    if(n==3){ 
        // Correct format
        FILE *fp;
        char filename[MAX_FILENAME_SIZE];

        sprintf(filename, "GAMES/GAME_%s.txt", PLID);

        if((fp = fopen(filename, "r+")) != NULL){
            fgets(buffer, MAX_WORD_LENGTH+MAX_FILENAME_SIZE+2, fp);
            
            sscanf(buffer, "%s %*s", word);
            int num_unique = count_unique_char(word), max_errors, len = strlen(word);

            if(len < 7)
                max_errors = 7;
            else if(len < 11)
                max_errors = 8;
            else
                max_errors = 9;
            
            int n_trials = 1, errors = 0;
            char previous[MAX_WORD_LENGTH], code;
            while(fgets(buffer, MAX_WORD_LENGTH+3, fp) != NULL){
                sscanf(buffer, "%c %s", &code, previous);
                if ((code=='T' && strchr(word, previous[0])==NULL) || (code=='G' && strcmp(word,previous)))
                    errors++;
                n_trials++; // increment trial number
            }

            // check the trial number
            if (n_trials != trial_number){
                sprintf(buffer, "RWG INV %d\n", n_trials);
            }
        
            else if (strcmp(word, guess)){
                if (errors < max_errors){
                    sprintf(buffer, "RWG NOK %d\n", n_trials);
                }
                else if (errors == max_errors){
                    sprintf(buffer, "RWG OVR %d\n", n_trials);
                    finish_game(PLID, filename, 'F');
                }
            }
            
            else if (!strcmp(word, guess)){
                sprintf(buffer, "RWG WIN %d\n", n_trials);
                finish_game(PLID, filename, 'W');
            }
            
            char line[MAX_WORD_LENGTH+MAX_FILENAME_SIZE+1];
            sprintf(line, "G %s\n", guess);
            fputs(line, fp);
            fclose(fp);

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

    if (verbose)
        printf("Sent: %s\n", buffer);
}


void quit(){
    char *ptr = buffer + 4;
    char PLID[MAX_PLID_SIZE + 1];
    n = sscanf(ptr, "%s\n", PLID);
    char filename[MAX_FILENAME_SIZE];
    sprintf(filename, "GAMES/GAME_%s.txt", PLID);
    if(!access(filename, F_OK)){ 
        // File exists
        finish_game(PLID, filename, 'Q');
        sprintf(buffer, "RQT OK\n");
    } else{
        sprintf(buffer, "RQT ERR\n");
    }

    n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    if(verbose)
        printf("Sent: %s\n", buffer);
}


void write_file(char *filename, char *buffer){
    char imagename[MAX_FILENAME_SIZE + 1];
    sprintf(imagename, "Dados_GS/%s", filename);

    FILE *fp = fopen(imagename, "r");
    if(fp == NULL){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fclose(fp);
    printf("tamanho do ficheiro: %d\n", size);

    // send filename and filesize
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

    fp = fopen(imagename, "r");
    if(fp == NULL){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    // send file's content
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
    char filename[MAX_FILENAME_SIZE + 1];
    sprintf(filename, "GAMES/GAME_%s.txt", PLID);
    
    if((fp = fopen(filename, "r")) != NULL){
        // read the first line: word hint_file
        fgets(buffer, MAX_WORD_LENGTH+MAX_FILENAME_SIZE+2, fp);
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