#include "server_UDP.h"

int fd, newfd, errno, errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;

int offset = 0;
char buffer[MAX_READ_SIZE + 1];


void UDP_command(char *word_file, char *port, int verbose){
    char command[MAX_COMMAND_SIZE + 1];

    while(1){
        UDP_connect(port);
        addrlen = sizeof(addr);
        n = recvfrom(fd, buffer, MAX_READ_SIZE, 0, (struct sockaddr*)&addr, &addrlen);
        if(n == -1){
            printf("ERROR\n");
            exit(1);
        }
        buffer[n] = '\0';
        
        if(verbose)
            printf("Received: %s", buffer);

        sscanf(buffer, "%s", command);
        if(!strcmp(command, "SNG")){
            start(word_file, verbose);
        } else if(!strcmp(command, "PLG")){
            play(verbose);
        } else if(!strcmp(command, "PWG")){
            guess(verbose);
        } else if(!strcmp(command, "QUT")){
            quit(verbose);
        } else{
            // Invalid command
            printf("ERROR\n");
            exit(1);
        }
        freeaddrinfo(res);
        close(fd);
    }
}


void UDP_connect(char *port){

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


void get_new_word(char *word_file){
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


void start(char *word_file, int verbose){
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
        get_new_word(word_file);
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


void play(int verbose){
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
                    if (strchr(word, letter)==NULL){
                        if (errors < max_errors){
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
                            char indexes[MAX_WORD_LENGTH*3], pos[4];
                            indexes[0] = '\0';
                            for (int i = 0; i<length; i++){
                                if (word[i] == letter){
                                    sprintf(pos, "%d ", i+1);
                                    strcat(indexes, pos);
                                }
                            }
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


void guess(int verbose){
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


void quit(int verbose){
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