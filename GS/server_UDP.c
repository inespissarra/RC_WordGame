#include "server_UDP.h"

int fd_UDP, errno, errcode_UDP;
ssize_t n_UDP;
socklen_t addrlen_UDP;
struct addrinfo hints_UDP, *res_UDP;
struct sockaddr_in addr_UDP;

int offset = 0;
char buffer_UDP[MAX_READ_SIZE + 1];


void UDP_command(char *word_file, char *port, int verbose){
    char command[MAX_COMMAND_SIZE + 1];

    while(1){
        UDP_connect(port);
        addrlen_UDP = sizeof(addr_UDP);
        n_UDP = recvfrom(fd_UDP, buffer_UDP, MAX_READ_SIZE, 0, (struct sockaddr*)&addr_UDP, &addrlen_UDP);
        if(n_UDP == -1){
            printf("ERROR\n");
            exit(1);
        }
        buffer_UDP[n_UDP] = '\0';

        sscanf(buffer_UDP, "%s", command);
        if(!strcmp(command, "SNG")){
            start(word_file, verbose);
        } else if(!strcmp(command, "PLG")){
            play(verbose);
        } else if(!strcmp(command, "PWG")){
            guess(verbose);
        } else if(!strcmp(command, "QUT")){
            quit(verbose);
        } else{
            sprintf(buffer_UDP, "ERR");
            sendtoUDP();
        }
        freeaddrinfo(res_UDP);
        close(fd_UDP);
    }
}


void UDP_connect(char *port){

    fd_UDP = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd_UDP == -1){
        printf("ERROR\n");
        exit(1);
    }

    memset(&hints_UDP, 0, sizeof hints_UDP);
    hints_UDP.ai_family = AF_INET;
    hints_UDP.ai_socktype = SOCK_DGRAM;
    hints_UDP.ai_flags = AI_PASSIVE;

    errcode_UDP = getaddrinfo(NULL, port, &hints_UDP, &res_UDP);
    if(errcode_UDP!=0){ 
        printf("ERROR\n");
        exit(1);
    }
    
    n_UDP = bind(fd_UDP, res_UDP->ai_addr, res_UDP->ai_addrlen);
    if(n_UDP == -1){
        printf("ERROR\n");
        exit(1);
    }

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    setsockopt(fd_UDP, SOL_SOCKET, SO_SNDTIMEO, (const char *) &timeout, sizeof(timeout));
    setsockopt(fd_UDP, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
}

void sendtoUDP(){
    n_UDP = sendto(fd_UDP, buffer_UDP, strlen(buffer_UDP), 0, (struct sockaddr*)&addr_UDP, addrlen_UDP);
    if(n_UDP == -1){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }
}


void getNewWord(char *word_file){
    FILE *fp = fopen(word_file, "r");
    if(fp==NULL){
        printf("ERROR\n");
        exit(1);
    }
    fseek(fp, offset, SEEK_SET);
    if(fgets(buffer_UDP, MAX_READ_SIZE, fp)==NULL){
        offset = 0;
        rewind(fp);
        fgets(buffer_UDP, MAX_READ_SIZE, fp);
    }
    offset += strlen(buffer_UDP);
    fclose(fp);
}

int isNumeric(char *str){
    while(*str){
        if(!isdigit(*str))
            return 0;
        str++;
    }
    return 1;
}

int validGuess(char* word){
    for(int i = 0; i < strlen(word); i++){
        if(!isalpha(word[i])){
            printf(INVALID_WORD);
            return 0;
        }
    }
    return 1;
}


void start(char *word_file, int verbose){
    char word[MAX_WORD_LENGTH + 1];
    char PLID[MAX_PLID_SIZE + 1], n;

    if(sscanf(buffer_UDP + 4, "%s%c", PLID, &n)!=2 || n!='\n' || strlen(PLID) != MAX_PLID_SIZE || !isNumeric(PLID)){
        if(verbose)
            printVerbose(NULL, NULL, NULL, -1);
        
        sprintf(buffer_UDP, "RSG ERR\n");
    } else {

        FILE *fp;
        char filename[MAX_FILENAME_SIZE + strlen(FOLDER_GAMES) + 1];
        sprintf(filename, "%sGAME_%s.txt", FOLDER_GAMES, PLID);

        if(verbose)
            printVerbose("SNG", PLID, NULL, -1);

        if(!access(filename, F_OK)){ 
            // File exists
            fp = fopen(filename, "r");
            fgets(buffer_UDP, MAX_READ_SIZE, fp);
            sscanf(buffer_UDP, "%s", word);

            if(fgets(buffer_UDP, MAX_READ_SIZE, fp)!=NULL){
                fclose(fp);
                sprintf(buffer_UDP, "RSG NOK\n");
                sendtoUDP();
                return;
            }
            fclose(fp);

        } else{
            getNewWord(word_file);
            sscanf(buffer_UDP, "%s", word);
            fp = fopen(filename, "w");
            fprintf(fp, "%s", buffer_UDP);
            fclose(fp);
        }

        int len = strlen(word), max_errors = getMaxErrors(len);

        sprintf(buffer_UDP, "RSG OK %d %d\n", len, max_errors);
    }

    if(verbose){
        printf("\nSent: %s\n", buffer_UDP);
        printf("----------------------\n\n");
    }
    
    sendtoUDP();
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
    while(fgets(buffer_UDP, MAX_READ_SIZE, fp) != NULL){
        sscanf(buffer_UDP, "%c %s", &code, previous);
        
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
        fgets(buffer_UDP, MAX_READ_SIZE, fp);
        
        char word[MAX_WORD_LENGTH + 1];
        sscanf(buffer_UDP, "%s ", word);

        int state[4];
        getState(fp, word, move, state, code);
        fclose(fp);

        if (state[N_TRIALS] != trial_number){
            if(state[N_TRIALS]-1==trial_number && state[DUP])
                validMove(word, move, code, state, PLID, filename);
            else
                sprintf(buffer_UDP, "%s INV %d\n", command, state[N_TRIALS]);
        }
        else if(state[DUP])
            sprintf(buffer_UDP, "%s DUP %d\n", command, state[N_TRIALS]);
        else{
            fp = fopen(filename, "a");
            sprintf(buffer_UDP, "%c %s\n", code, move);
            fputs(buffer_UDP, fp);
            fclose(fp);
            validMove(word, move, code, state, PLID, filename);
        }
    } else
        sprintf(buffer_UDP, "%s ERR\n", command);
}


void validMove(char *word, char *move, char code, int state[4], char *PLID, char *filename){
    int len = strlen(word), max_errors = getMaxErrors(len);

    char command[4];
    getCommand(code, command);

    if ((code == 'T' && strchr(word, *move)!=NULL) || (code == 'G' && !strcmp(word, move))){
        // Correct
        if (code == 'G' || state[CORRECTS]+1 == countDiffChar(word)){
            sprintf(buffer_UDP, "%s WIN %d\n", command, state[N_TRIALS]);
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
            sprintf(buffer_UDP, "%s OK %d %d %s\n", command, state[N_TRIALS], n, indexes);
        }
    } else {
        // Not correct
        if (state[ERRORS] < max_errors)
            sprintf(buffer_UDP, "%s NOK %d\n", command, state[N_TRIALS]);
        else if (state[ERRORS] == max_errors){
            sprintf(buffer_UDP, "%s OVR %d\n", command, state[N_TRIALS]);
            finishGame(PLID, filename, 'F');
        }
    }
}


void play(int verbose){
    char letter[2], PLID[MAX_PLID_SIZE + 1], n;
    int trial_number;

    n_UDP = sscanf(buffer_UDP + 4, "%s %s %d%c", PLID, letter, &trial_number, &n);

    if(n_UDP==4 && n=='\n' && strlen(PLID) == MAX_PLID_SIZE && isNumeric(PLID) && isalpha(letter[0])){
        // Correct format
        if(verbose)
            printVerbose("PLG", PLID, letter, trial_number);

        char filename[MAX_FILENAME_SIZE + strlen(FOLDER_GAMES) + 1];
        sprintf(filename, "%sGAME_%s.txt", FOLDER_GAMES, PLID);
        if (access(filename, F_OK))
            sprintf(buffer_UDP, "ERR\n");
        else{
            sprintf(filename, "%sGAME_%s.txt", FOLDER_GAMES, PLID);
            move(filename, letter, 'T', PLID, trial_number);
        }
        
    } else{
        if(verbose)
            printVerbose(NULL, NULL, NULL, -1);
        sprintf(buffer_UDP, "RLG ERR\n");
    }

    if(verbose){
        printf("Sent: %s\n", buffer_UDP);
        printf("----------------------\n\n");
    }

    sendtoUDP();
}


void guess(int verbose){
    int trial_number;
    char PLID[MAX_PLID_SIZE + 1], guess[MAX_WORD_LENGTH + 1], n;

    n_UDP = sscanf(buffer_UDP + 4, "%s %s %d%c", PLID, guess, &trial_number, &n);

    if(n_UDP==4 && n=='\n' && strlen(PLID) == MAX_PLID_SIZE && isNumeric(PLID) && strlen(guess) <= MAX_WORD_LENGTH && strlen(guess) >= MIN_WORD_LENGTH && validGuess(guess)){
        // Correct format

        if(verbose)
            printVerbose("PWG", PLID, guess, trial_number);

        char filename[MAX_FILENAME_SIZE + strlen(FOLDER_GAMES) + 1];
        sprintf(filename, "%sGAME_%s.txt", FOLDER_GAMES, PLID);
        if (access(filename, F_OK)){
            printf("here\n");
            sprintf(buffer_UDP, "ERR\n");}

        else{
            sprintf(filename, "%sGAME_%s.txt", FOLDER_GAMES, PLID);
            move(filename, guess, 'G', PLID, trial_number);
        }
    } else {
        if(verbose)
            printVerbose(NULL, NULL, NULL, -1);
        sprintf(buffer_UDP, "RLG ERR\n");
    }

    if(verbose){
        printf("Sent: %s\n", buffer_UDP);
        printf("----------------------\n\n");
    }

    sendtoUDP();
}


void quit(int verbose){
    char PLID[MAX_PLID_SIZE + 1], n;

    if(sscanf(buffer_UDP + 4, "%s%c", PLID, &n)!=2 || n!='\n' || strlen(PLID) != MAX_PLID_SIZE || !isNumeric(PLID)){
        if(verbose)
            printVerbose(NULL, NULL, NULL, -1);
        sprintf(buffer_UDP, "RQT ERR\n");
    } else{
        char filename[MAX_FILENAME_SIZE + strlen(FOLDER_GAMES) + 1];
        sprintf(filename, "%sGAME_%s.txt", FOLDER_GAMES, PLID);

        if(verbose)
            printVerbose("QUT", PLID, NULL, -1);

        if(!access(filename, F_OK)){ 
            // File exists
            finishGame(PLID, filename, 'Q');
            sprintf(buffer_UDP, "RQT OK\n");
        } else{
            sprintf(buffer_UDP, "RQT NOK\n");
        }
    }

    if(verbose){
        printf("Sent: %s\n", buffer_UDP);
        printf("----------------------\n\n");
    }
    sendtoUDP();
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

void printVerbose(char *command, char *PLID, char *move, int trial_number){
    char host[NI_MAXHOST],service[NI_MAXSERV];

    printf("----------------------\n");

    if((errcode_UDP=getnameinfo((struct sockaddr *)&addr_UDP,addrlen_UDP,host,sizeof host,service,sizeof service,0))!=0) 
        fprintf(stderr,"error: getnameinfo: %s\n",gai_strerror(errcode_UDP));
    else
        printf("Sent by:\n\thost: %s\n\tport: %s\n\n", host, service);
        
    if(command!=NULL){
        printf("Command: %s\n", command);
        if(PLID!=NULL)
            printf("PLID: %s\n", PLID);
        if(move!=NULL)
            printf("Move: %s\n", move);
        if(trial_number!=-1)
            printf("Trial number: %d\n", trial_number);
    } else{
        printf("Received: %s", buffer_UDP);
    }
}