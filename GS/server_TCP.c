#include "server_TCP.h"

int fd_TCP, newfd_TCP, errno_TCP, errcode_TCP;
ssize_t n_TCP;
socklen_t addrlen_TCP;
struct addrinfo hints_TCP, *res_TCP;
struct sockaddr_in addr_TCP;

char buffer_TCP[MAX_READ_SIZE + 1];

void TCP_command(char *port, int verbose){

    TCP_OpenSocket(port);
    while(1){
        
        addrlen_TCP = sizeof(addr_TCP);
        if((newfd_TCP = accept(fd_TCP, (struct sockaddr*)&addr_TCP, &addrlen_TCP)) == -1){
            printf("ERROR\n");
            exit(1);
        }

        pid_t c1_pid, wpid;

        c1_pid = fork();
        if(c1_pid == 0){
            // child process
            char *ptr = buffer_TCP;

            struct timeval timeout;
            timeout.tv_sec = TIMEOUT;
            timeout.tv_usec = 0;
            setsockopt(newfd_TCP, SOL_SOCKET, SO_SNDTIMEO, (const char *) &timeout, sizeof(timeout));
            setsockopt(newfd_TCP, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
    
            do {
                n_TCP= read(newfd_TCP, ptr, 1);
                if(n_TCP== -1){
                    printf("ERROR\n");
                    exit(1);
                }
                ptr+=n_TCP;
            } while(*(ptr-1)!=' ' && *(ptr-1)!='\n');
            *(ptr-1) = '\0';
            
            if(verbose){
                printf("----------------------\n");
                char host[NI_MAXHOST],service[NI_MAXSERV];

                if((errcode_TCP=getnameinfo((struct sockaddr *)&addr_TCP,addrlen_TCP,host,sizeof host,service,sizeof service,0))!=0) 
                    fprintf(stderr,"error: getnameinfo: %s\n",gai_strerror(errcode_TCP));
                else
                    printf("Sent by:\n\thost: %s\n\tport: %s\n", host, service);
                printf("Command: %s\n", buffer_TCP);
            }

            if(!strcmp(buffer_TCP, "GSB")){
                scoreboard();
            } else if(!strcmp(buffer_TCP, "GHL")){
                hint(verbose);
            } else if(!strcmp(buffer_TCP, "STA")){
                state(verbose);
            } else{
                // Invalid command
                printf("ERROR\n");
                exit(1);
            }
            if(verbose)
                printf("----------------------\n\n");
            close(newfd_TCP);
            exit(0);
        } else if(c1_pid < 0){
            printf("ERROR\n");
            exit(1);
        }
    }
    freeaddrinfo(res_TCP);
    close(fd_TCP);
}



void TCP_OpenSocket(char *port){
    fd_TCP = socket(AF_INET, SOCK_STREAM, 0);
    if(fd_TCP == -1){
        printf("ERROR\n");
        exit(1);
    }

    memset(&hints_TCP, 0, sizeof hints_TCP);
    hints_TCP.ai_family = AF_INET;
    hints_TCP.ai_socktype = SOCK_STREAM;
    hints_TCP.ai_flags = AI_PASSIVE;

    errcode_TCP = getaddrinfo(NULL, port, &hints_TCP, &res_TCP);
    if(errcode_TCP!=0){ 
        printf("ERROR\n");
        exit(1);
    }
    
    n_TCP= bind(fd_TCP, res_TCP->ai_addr, res_TCP->ai_addrlen);
    if(n_TCP== -1){
        printf("ERROR\n");
        exit(1);
    }

    if(listen(fd_TCP, 5) == -1){ // 5 is the maximum number of pending connections, can be changed (?)
        printf("ERROR\n");
        exit(1);
    }
}



void writeFile(char *filename, char *buffer_TCP){
    char imagename[MAX_FILENAME_SIZE + strlen(FOLDER_DATA) + 1];
    sprintf(imagename, "%s%s", FOLDER_DATA, filename);

    FILE *fp = fopen(imagename, "r");
    if(fp == NULL){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fclose(fp);

    // send filename and filesize
    sprintf(buffer_TCP, "%s %d ", filename, size);
    char *ptr = buffer_TCP;
    int n_left = strlen(buffer_TCP);
    while((n_TCP= write(newfd_TCP, ptr, n_left))!=0){
        if(n_TCP== -1){
            printf("ERROR\n");
            exit(1);
        }
        ptr += n_TCP;
        n_left -= n_TCP;
    }

    fp = fopen(imagename, "r");
    if(fp == NULL){
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(1);
    }

    // send file's content
    while (size > 0){
        n_left = fread(buffer_TCP, 1, MAX_READ_SIZE, fp);
        ptr = buffer_TCP;
        while((n_TCP= write(newfd_TCP, ptr, n_left))!=0){
            if(n_TCP== -1){
                printf("ERROR\n");
                exit(1);
            }
            ptr += n_TCP;
            n_left -= n_TCP;
            size -=n_TCP;
        }
    }
    buffer_TCP[0] = '\n';

    write(newfd_TCP, buffer_TCP, 1);

    fclose(fp);
}

void readPLID(char *PLID, int verbose){
    int n_left = MAX_PLID_SIZE;
    char *ptr = PLID;
    while(n_TCP>0){
        n_TCP= read(newfd_TCP, ptr, n_left);
        if(n_TCP== -1){
            printf("ERROR\n");
            exit(1);
        }
        ptr += n_TCP;
        n_left -= n_TCP;
    }
    *ptr = '\0';
    n_TCP=0;
    while((n_TCP= read(newfd_TCP, buffer_TCP, 1))==0){ 
        if(n_TCP== -1){
            printf("ERROR\n");
            exit(1);
        }
    }
    if (verbose)
        printf("PLID: %s\n", PLID);
}

void hint(int verbose){
    char PLID[MAX_PLID_SIZE + 1];
    readPLID(PLID, verbose);
    
    FILE *fp;
    char filename[MAX_FILENAME_SIZE + strlen(FOLDER_GAMES) + 1];
    sprintf(filename, "%sGAME_%s.txt", FOLDER_GAMES, PLID);
    
    if((fp = fopen(filename, "r")) != NULL){
        // read the first line: word hint_file
        fgets(buffer_TCP, MAX_READ_SIZE, fp);
        fclose(fp);

        sscanf(buffer_TCP, "%*s %s", filename);
        sprintf(buffer_TCP, "RHL OK ");

        char *ptr = buffer_TCP;
        int n_left = strlen(buffer_TCP);
        while((n_TCP= write(newfd_TCP, ptr, n_left))!=0){
            if(n_TCP== -1){
                printf("ERROR\n");
                exit(1);
            } 
            ptr += n_TCP;
            n_left -= n_TCP;
        }

        writeFile(filename, buffer_TCP);

    } else {
        sprintf(buffer_TCP, "RHL NOK\n");
        while((n_TCP= write(newfd_TCP, buffer_TCP, strlen(buffer_TCP)))!=0)
        if(n_TCP== -1){
            printf("ERROR\n");
            exit(1);
        }
    }
}

int findTopScores(char sb_file[MAX_FILE_SIZE + 1]){
    struct dirent **filelist;
    int n_entries, i_file, score, corrects, trials, i=0;
    char fname[SCORE_FILENAME_SIZE + 1], PLID[MAX_PLID_SIZE + 1], word[MAX_WORD_LENGTH + 1];
    FILE *fp;
    
    n_entries = scandir(FOLDER_SCORES, &filelist, 0, alphasort);
    i_file = 0;
    if (n_entries < 0)
        return (0);
    else{
        strcpy(sb_file, "\n-------------------------------- TOP 10 SCORES --------------------------------\n    SCORE PLAYER     WORD                             GOOD TRIALS  TOTAL TRIALS\n\0");
        while (i++ < 10 && n_entries != 0){
            n_entries--;
            if (filelist[n_entries]->d_name[0] != '.'){
                char filename[MAX_FILENAME_SIZE + strlen(FOLDER_SCORES) + 1];
                strcpy(filename, filelist[n_entries]->d_name);
                sprintf(fname, "%s%s", FOLDER_SCORES, filename);
                fp = fopen(fname, "r");
                if (fp != NULL){
                    fscanf(fp, "%d %s %s %d %d", &score, PLID, word, &corrects, &trials);
                    sprintf(buffer_TCP, " %d - %03d  %s  %-40s%-14d%d\n", ++i_file, score, PLID, word, corrects, trials);
                    strcat(sb_file, buffer_TCP);
                    fclose(fp);
                }
            }
            free(filelist[n_entries]);
        }
        free(filelist);
        strcat(sb_file, "\n");
    }
    return i_file;
}

void scoreboard(){
    char sb_file[MAX_FILE_SIZE + 1], response[MAX_READ_SIZE+MAX_FILE_SIZE + 1];
    int n_scores = findTopScores(sb_file);

    if (n_scores == 0)
        sprintf(response, "RSB EMPTY\n");
    
    else{
        sprintf(response, "RSB OK TOPSCORES_%07d.txt %zu %s\n", getppid(), strlen(sb_file), sb_file);
    }

    char *ptr = response;
    int n_left = strlen(response);
    while((n_TCP= write(newfd_TCP, ptr, n_left))!=0){
        if(n_TCP== -1){
            printf("ERROR\n");
            exit(1);
        } 
        ptr += n_TCP;
        n_left -= n_TCP;
    }
}


int findLastGame(char *PLID, char *filename){
    struct dirent *dir, **filelist;
    int n_entries, found;
    char dirname[strlen(FOLDER_GAMES) + MAX_PLID_SIZE + 2];


    sprintf(dirname, "%s%s/", FOLDER_GAMES, PLID);
    n_entries = scandir(dirname, &filelist, 0, alphasort);
    found = 0;

    if (n_entries <= 0)
        return (0);
    else{
        while (n_entries--){
            printf("n_entries=%d\n", n_entries);
            if (filelist[n_entries]->d_name[0] != '.'){
                char dname[MAX_FILENAME_SIZE + 1];
                strcpy(dname, filelist[n_entries]->d_name);
                printf("dname=%s\n", dname);
                sprintf(filename, "%s%s/%s", FOLDER_GAMES, PLID, dname);
                found = 1;
            }
            free(filelist[n_entries]);
            if (found == 1)
                break;
        }
        free(filelist);
    }
    return found;
}

void createStateFile(char *PLID, char *filename, char file[MAX_FILE_SIZE + 1], int status){
    FILE *fp = fopen(filename, "r");
    int n_trials=0;
    char code, previous[MAX_WORD_LENGTH + 1], transactions[MAX_FILE_SIZE + 1];
    char game[MAX_WORD_LENGTH + 1], word[MAX_WORD_LENGTH + 1], hintfile[MAX_FILENAME_SIZE + 1];
    
    fgets(buffer_TCP, MAX_READ_SIZE, fp);
    sscanf(buffer_TCP, "%s %s", word, hintfile);
    memset(game, '_', strlen(word));
    game[strlen(word)] = '\0';
    transactions[0] = '\0';

    while(fgets(buffer_TCP, MAX_READ_SIZE, fp) != NULL){
        sscanf(buffer_TCP, "%c %s", &code, previous);
        if (code == 'T'){
            if (strchr(word, previous[0])!=NULL){

                for (int i = 0; i<strlen(word); i++)
                    if (word[i] == previous[0])
                        game[i] = previous[0];
                
                sprintf(buffer_TCP, "     Letter trial: %c - TRUE\n", previous[0]);
            }
            else
                sprintf(buffer_TCP, "     Letter trial: %c - FALSE\n", previous[0]);
        }
        else if (code == 'G')
            sprintf(buffer_TCP, "     Word guess: %s\n", previous);
        
        strcat(transactions, buffer_TCP);
        n_trials ++;
    }
    file[0] = '\0';

    if (status){
        sprintf(buffer_TCP, "     Solved so far: %s\n", game);
        strcat(transactions, buffer_TCP);
        sprintf(file, "     Active  game found for player %s\n     --- Transactions found: %d ---\n", PLID, n_trials);
        strcat(file, transactions);
    }

    else{
        printf("yeah\n");
        sscanf(filename + strlen(FOLDER_GAMES) + MAX_PLID_SIZE + strlen("YYYYMMDD_HHMMSS_") + 1, "%c", &code);
        if (code == 'W')
            sprintf(buffer_TCP, "     Termination: WIN\n");
        else if (code == 'F')
            sprintf(buffer_TCP, "     Termination - FAIL\n");
        else if (code == 'Q')
            sprintf(buffer_TCP, "     Termination - QUIT\n");
        strcat(transactions, buffer_TCP);
        sprintf(file, "     Last finalized game for player %s\n     Word: %s; Hint file: %s\n     --- Transaction found: %d ---\n", PLID, word, hintfile, n_trials);
        strcat(file, transactions);

    }

}


void state(int verbose){
    char filename[MAX_FILENAME_SIZE + strlen(FOLDER_GAMES) + 1], state_filename[MAX_FILENAME_SIZE + 1];
    char PLID[MAX_PLID_SIZE+1], response[MAX_READ_SIZE+MAX_FILE_SIZE + 1];
    readPLID(PLID, verbose);
    sprintf(filename, "%sGAME_%s.txt", FOLDER_GAMES, PLID);
    sprintf(state_filename, "STATE_%s.txt", PLID);
    char statefile[MAX_FILE_SIZE + 1];

    if(!access(filename, F_OK)){ 
        createStateFile(PLID, filename, statefile, 1);
        sprintf(response, "RST ACT %s %zd %s\n", state_filename, strlen(statefile), statefile);
    }
    else {
        int found = findLastGame(PLID, filename);
        if (found){
            createStateFile(PLID, filename, statefile, 0);
            sprintf(response, "RST FIN %s %zd %s\n", state_filename, strlen(statefile), statefile);
        }
        else
            sprintf(response, "RST NOK\n");
    }


    char *ptr = response;
    int n_left = strlen(response);
    while((n_TCP= write(newfd_TCP, ptr, n_left))!=0){
        if(n_TCP== -1){
            printf("ERROR\n");
            exit(1);
        } 
        ptr += n_TCP;
        n_left -= n_TCP;
    }
}