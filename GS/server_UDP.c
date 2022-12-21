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

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char *) &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
}


void getNewWord(char *word_file){
    FILE *fp = fopen(word_file, "r");
    if(fp==NULL){
        printf("ERROR\n");
        exit(1);
    }
    fseek(fp, offset, SEEK_SET);
    if(fgets(buffer, MAX_READ_SIZE, fp)==NULL){
        offset = 0;
        rewind(fp);
        fgets(buffer, MAX_READ_SIZE, fp);
    }
    offset += strlen(buffer);
    fclose(fp);
}


void start(char *word_file, int verbose){
    char *ptr = buffer + 4;
    char word[MAX_WORD_LENGTH + 1];
    char PLID[MAX_PLID_SIZE + 1];
    sscanf(ptr, "%s", PLID);
    FILE *fp;
    char filename[MAX_FILENAME_SIZE + strlen(FOLDER_GAMES) + 1];
    sprintf(filename, "%sGAME_%s.txt", FOLDER_GAMES, PLID);

    if(verbose){
        printf("----------------------\n");
        printVerbose("SNG", PLID);
        printf("----------------------\n\n");
    }

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
            return;
        }
        fclose(fp);

    } else{
        getNewWord(word_file);
        sscanf(buffer, "%s", word);
        fp = fopen(filename, "w");
        fprintf(fp, "%s", buffer);
        fclose(fp);
    }

    int len = strlen(word), max_errors = getMaxErrors(len);

    sprintf(buffer, "RSG OK %d %d\n", len, max_errors);
    
    n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
}


int countDiffChar(char word[MAX_WORD_LENGTH + 1]){
    char unique[MAX_WORD_LENGTH + 1];
    int length = strlen(word);
    int n_unique = 0;
    for (int i=0; i<length; i++){
        if (strchr(unique, word[i])==NULL){
            unique[n_unique] = word[i];
            n_unique++;
        }
    }
    return n_unique;
}


int getMaxErrors(int length){
    if(length < 7)
        return 7;
    else if(length < 11)
        return 8;
    else
        return 9;
}


void getCommand(char code, char *command){
    if(code=='T')
        strcpy(command, "RLG");
    else
        strcpy(command, "RWG");
}


void getState(FILE *fp, char *word, char *move, int state[4], char current_code){
    int n_trials = 1, errors = 0, corrects=0, dup = 0;
    char previous[MAX_WORD_LENGTH + 1], code;
    while(fgets(buffer, MAX_READ_SIZE, fp) != NULL){
        sscanf(buffer, "%c %s", &code, previous);
        
        if ((current_code == 'T' && code == 'T' && previous[0] == *move) || (current_code == 'G' && code == 'G' && !strcmp(word, previous)))
            // if the letter/word was sent in a previous trial
            dup = 1;

        if ((code == 'T' && strchr(word, previous[0])!=NULL) || (code=='G' && !strcmp(word,previous)))
            corrects ++;
        
        if ((code=='T' && strchr(word, previous[0])==NULL) || (code=='G' && strcmp(word,previous)))
            errors++;
        
        n_trials++;
    }
    state[N_TRIALS] = n_trials; 
    state[CORRECTS] = corrects; 
    state[ERRORS] = errors; 
    state[DUP] = dup;
}


void move(char *filename, char *move, char code, char *PLID, int trial_number){
    FILE *fp;
    char command[4];
    getCommand(code, command);
    if((fp = fopen(filename, "r")) != NULL){
        fgets(buffer, MAX_READ_SIZE, fp);
        
        char word[MAX_WORD_LENGTH + 1];
        sscanf(buffer, "%s ", word);

        int state[4];
        getState(fp, word, move, state, code);
        fclose(fp);

        if (state[N_TRIALS] != trial_number){
            if(state[N_TRIALS]-1==trial_number && state[DUP])
                validMove(word, move, code, state, PLID, filename);
            else
                sprintf(buffer, "%s INV %d\n", command, state[N_TRIALS]);
        }
        else if(state[DUP])
            sprintf(buffer, "%s DUP %d\n", command, state[N_TRIALS]);
        else{
            fp = fopen(filename, "a");
            sprintf(buffer, "%c %s\n", code, move);
            fputs(buffer, fp);
            fclose(fp);
            validMove(word, move, code, state, PLID, filename);
        }
    } else
        sprintf(buffer, "%s ERR\n", command);
}


void validMove(char *word, char *move, char code, int state[4], char *PLID, char *filename){
    int len = strlen(word), max_errors = getMaxErrors(len);

    char command[4];
    getCommand(code, command);

    if ((code == 'T' && strchr(word, *move)!=NULL) || (code == 'G' && !strcmp(word, move))){
        // Correct
        if (code == 'G' || state[CORRECTS]+1 == countDiffChar(word)){
            sprintf(buffer, "%s WIN %d\n", command, state[N_TRIALS]);
            finishGame(PLID, filename, 'W');
            createScoreFile(PLID, word, state[CORRECTS]+1, state[N_TRIALS]);
        }
        else{
            char indexes[MAX_WORD_LENGTH*3], pos[4];
            indexes[0] = '\0';
            int n=0;
            for (int i = 0; i<len; i++){
                if (word[i] == *move){
                    sprintf(pos, "%d ", i+1);
                    strcat(indexes, pos);
                    n++;
                }
            }
            sprintf(buffer, "%s OK %d %d %s\n", command, state[N_TRIALS], n, indexes);
        }
    } else {
        // Not correct
        if (state[ERRORS] < max_errors)
            sprintf(buffer, "%s NOK %d\n", command, state[N_TRIALS]);
        else if (state[ERRORS] == max_errors){
            sprintf(buffer, "%s OVR %d\n", command, state[N_TRIALS]);
            finishGame(PLID, filename, 'F');
        }
    }
}


void play(int verbose){
    char letter[2], PLID[MAX_PLID_SIZE + 1];;
    int trial_number;

    n = sscanf(buffer + 4, "%s %s %d\n", PLID, letter, &trial_number);

    if(verbose){
        printf("----------------------\n");
        printVerbose("PLG", PLID);
        printf("Letter: %s\n", letter);
        printf("Trial number: %d\n", trial_number);
        printf("----------------------\n\n");
    }

    if(n==3){
        // Correct format
        char filename[MAX_FILENAME_SIZE + strlen(FOLDER_GAMES) + 1];
        sprintf(filename, "%sGAME_%s.txt", FOLDER_GAMES, PLID);
        move(filename, letter, 'T', PLID, trial_number);
    } else
        sprintf(buffer, "RLG ERR\n");

    n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
}


void guess(int verbose){
    int trial_number;
    char PLID[MAX_PLID_SIZE + 1], guess[MAX_WORD_LENGTH + 1];

    n = sscanf(buffer + 4, "%s %s %d\n", PLID, guess, &trial_number);

    if(verbose){
        printf("----------------------\n");
        printVerbose("PWG", PLID);
        printf("Guess: %s\n", guess);
        printf("Trial number: %d\n", trial_number);
        printf("----------------------\n\n");
    }

    if(n==3){ 
        // Correct format
        char filename[MAX_FILENAME_SIZE + strlen(FOLDER_GAMES) + 1];
        sprintf(filename, "%sGAME_%s.txt", FOLDER_GAMES, PLID);
        move(filename, guess, 'G', PLID, trial_number);
    } else
        sprintf(buffer, "RLG ERR\n");

    n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
}


void quit(int verbose){
    char *ptr = buffer + 4;
    char PLID[MAX_PLID_SIZE + 1];
    n = sscanf(ptr, "%s\n", PLID);
    char filename[MAX_FILENAME_SIZE + strlen(FOLDER_GAMES) + 1];
    sprintf(filename, "%sGAME_%s.txt", FOLDER_GAMES, PLID);

    if(verbose){
        printf("----------------------\n");
        printVerbose("QUT", PLID);
        printf("----------------------\n\n");
    }

    if(!access(filename, F_OK)){ 
        // File exists
        finishGame(PLID, filename, 'Q');
        sprintf(buffer, "RQT OK\n");
    } else{
        sprintf(buffer, "RQT ERR\n");
    }

    n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
}


void finishGame(char *PLID, char *filename, char state){
    char buf[MAX_FILENAME_SIZE + strlen(FOLDER_GAMES) + MAX_PLID_SIZE + 2];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    sprintf(buf, "%s%s", FOLDER_GAMES, PLID);

    DIR* dir = opendir(buf);
    if(dir){
        closedir(dir);
    } else if(ENOENT == errno){
        mkdir(buf, 0777);
    } else{
        perror("opendir");
        exit(1);
    }
    sprintf(buf, "%s%s/%04d%02d%02d_%02d%02d%02d_%c.txt", FOLDER_GAMES, PLID, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, state);
    rename(filename, buf);
}


void createScoreFile(char* PLID, char *word, int corrects, int trials){
    char filename[SCORE_FILENAME_SIZE + 1];
    
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    int score = (int)(((float)corrects/(float)trials)*100);

    sprintf(filename, "%s%03d_%s_%04d%02d%02d_%02d%02d%02d.txt", FOLDER_SCORES, score, PLID, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    
    FILE* fp = fopen(filename, "w");
    fprintf(fp, "%03d %s %s %d %d\n", score, PLID, word, corrects, trials);
    fclose(fp);
}

void printVerbose(char *command, char *PLID){
    char host[NI_MAXHOST],service[NI_MAXSERV];

    if((errcode=getnameinfo((struct sockaddr *)&addr,addrlen,host,sizeof host,service,sizeof service,0))!=0) 
        fprintf(stderr,"error: getnameinfo: %s\n",gai_strerror(errcode));
    else
        printf("Sent by:\n\thost: %s\n\tport: %s\n\n", host, service);
    printf("Command: %s\nPLID: %s\n", command, PLID);
}