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

    int len = strlen(word), max_errors = get_max_errors(len);

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
    int n_unique = 0;
    for (int i=0; i<length; i++){
        if (strchr(unique, word[i])==NULL){
            unique[n_unique] = word[i];
            n_unique++;
        }
    }
    return n_unique;
}

int get_max_errors(int length){
    if(length < 7)
        return 7;
    else if(length < 11)
        return 8;
    else
        return 9;
}

void get_command(char code, char *command){
    if(code=='T')
        strcpy(command, "RLG");
    else
        strcpy(command, "RWG");
}

void get_state(FILE *fp, char *word, char *move, int state[4], char actual_code){
    int n_trials = 1, errors = 0, corrects=0, dup = 0;
    char previous[MAX_WORD_LENGTH], code;
    while(fgets(buffer, MAX_WORD_LENGTH+3, fp) != NULL){
        sscanf(buffer, "%c %s", &code, previous);
        if (actual_code == 'T' && code == 'T'){
            // if the letter was sent in a previous trial
            if (previous[0] == *move)
                dup = 1;
            if (strchr(word, previous[0])!=NULL)
                corrects ++;
        }
        
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
    get_command(code, command);
    if((fp = fopen(filename, "r")) != NULL){
        fgets(buffer, MAX_READ_SIZE, fp);
        
        char word[MAX_WORD_LENGTH + 1];
        sscanf(buffer, "%s ", word);

        int state[4];
        get_state(fp, word, move, state, code);
        fclose(fp);

        if (state[N_TRIALS] != trial_number){
            if(state[N_TRIALS]-1==trial_number && state[DUP])
                valid_move(word, move, code, state, PLID, filename);
            else
                sprintf(buffer, "%s INV %d\n", command, state[N_TRIALS]);
        }
        else if(state[DUP])
            sprintf(buffer, "%s DUP %d\n", command, state[N_TRIALS]);
        else{
            fp = fopen(filename, "a+");
            sprintf(buffer, "%c %s\n", code, move);
            fputs(buffer, fp);
            fclose(fp);
            valid_move(word, move, code, state, PLID, filename);
        }
    } else
        sprintf(buffer, "%s ERR\n", command);
}

void valid_move(char *word, char *move, char code, int state[4], char *PLID, char *filename){
    int len = strlen(word), max_errors = get_max_errors(len);

    char command[4];
    get_command(code, command);

    if ((code == 'T' && strchr(word, *move)!=NULL) || (code == 'G' && !strcmp(word, move))){
        // Correct
        if (code == 'G' || state[CORRECTS]+1 == count_unique_char(word)){
            sprintf(buffer, "%s WIN %d\n", command, state[N_TRIALS]);
            finish_game(PLID, filename, 'W');
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
            finish_game(PLID, filename, 'F');
        }
    }
}


void play(int verbose){
    char letter[2], PLID[MAX_PLID_SIZE + 1];;
    int trial_number;

    n = sscanf(buffer + 4, "%s %s %d\n", PLID, letter, &trial_number);

    if(n==3){
        // Correct format
        char filename[MAX_FILENAME_SIZE];
        sprintf(filename, "GAMES/GAME_%s.txt", PLID);
        move(filename, letter, 'T', PLID, trial_number);
    } else
        sprintf(buffer, "RLG ERR\n");

    n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addrlen);
    if(n == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
    if (verbose)
        printf("Sent: %s\n", buffer);
}

void guess(int verbose){
    int trial_number;
    char PLID[MAX_PLID_SIZE + 1], guess[MAX_WORD_LENGTH];

    n = sscanf(buffer + 4, "%s %s %d\n", PLID, guess, &trial_number);

    if(n==3){ 
        // Correct format
        char filename[MAX_FILENAME_SIZE];
        sprintf(filename, "GAMES/GAME_%s.txt", PLID);
        move(filename, guess, 'G', PLID, trial_number);
    } else
        sprintf(buffer, "RLG ERR\n");

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

void finish_game(char *PLID, char *filename, char state){
    char buf[MAX_FILENAME_SIZE];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    sprintf(buf, "GAMES/%s", PLID);

    DIR* dir = opendir(buf);
    if(dir){
        closedir(dir);
    } else if(ENOENT == errno){
        mkdir(buf, 0777);
    } else{
        perror("opendir");
        exit(1);
    }
    sprintf(buf, "GAMES/%s/%d%d%d_%d%d%d_%c.txt", PLID, tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, state);
    rename(filename, buf);
}