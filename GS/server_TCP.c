#include "server_TCP.h"

int fd, newfd, errno, errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;

char buffer[MAX_READ_SIZE + 1];

void TCP_command(char *port, int verbose){

    TCP_OpenSocket(port);
    while(1){
        
        addrlen = sizeof(addr);
        if((newfd = accept(fd, (struct sockaddr*)&addr, &addrlen)) == -1){
            printf("ERROR\n");
            exit(1);
        }

        pid_t c1_pid, wpid;

        c1_pid = fork();
        if(c1_pid == 0){
            // child process
            char *ptr = buffer;
    
            do {
                n = read(newfd, ptr, 1);
                if(n == -1){
                    printf("ERROR\n");
                    exit(1);
                }
                ptr+=n;
            } while(*(ptr-1)!=' ' && *(ptr-1)!='\n');
            *(ptr-1) = '\0';
            
            if(verbose)
                printf("Received: %s\n", buffer);

            if(!strcmp(buffer, "GSB")){
                sleep(10);
                scoreboard(verbose);
            } else if(!strcmp(buffer, "GHL")){
                hint(verbose);
            } else if(!strcmp(buffer, "STA")){
                state(verbose);
            } else{
                // Invalid command
                printf("ERROR\n");
                exit(1);
            }
            close(newfd);
            exit(0);
        } else if(c1_pid < 0){
            printf("ERROR\n");
            exit(1);
        }
    }
    freeaddrinfo(res);
    close(fd);
}



void TCP_OpenSocket(char *port){
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
        printf("ERROR\n");
        exit(1);
    }

    if(listen(fd, 5) == -1){ // 5 is the maximum number of pending connections, can be changed (?)
        printf("ERROR\n");
        exit(1);
    }

    int timeout = TIMEOUT;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char *) &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
}



void writeFile(char *filename, char *buffer, int verbose){
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
    if(verbose)
        printf("%s*file*\n\n", buffer);

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

void readPLID(char *PLID){
    int n_left = MAX_PLID_SIZE;
    while(n>0){
        n = read(newfd, PLID, n_left);
        if(n == -1){
            printf("ERROR\n");
            exit(1);
        }
        PLID += n;
        n_left -= n;
    }
    *PLID = '\0';
    n=0;
    while((n = read(newfd, buffer, 1))==0){ 
        if(n == -1){
            printf("ERROR\n");
            exit(1);
        }
    }
}

void hint(int verbose){
    char PLID[MAX_PLID_SIZE + 1];
    readPLID(PLID);
    if (verbose)
        printf("%s\n", PLID);
    FILE *fp;
    char filename[MAX_FILENAME_SIZE + strlen(FOLDER_GAMES) + 1];
    sprintf(filename, "%sGAME_%s.txt", FOLDER_GAMES, PLID);
    
    if((fp = fopen(filename, "r")) != NULL){
        // read the first line: word hint_file
        fgets(buffer, MAX_READ_SIZE, fp);
        fclose(fp);

        sscanf(buffer, "%*s %s", filename);
        sprintf(buffer, "RHL OK ");

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
        if(verbose)
            printf("Sent: %s", buffer);

        writeFile(filename, buffer, verbose);

    } else {
        sprintf(buffer, "RHL NOK\n");
        while((n = write(newfd, buffer, strlen(buffer)))!=0)
        if(n == -1){
            printf("ERROR\n");
            exit(1);
        }
        if(verbose)
            printf("Sent: %s\n", buffer);
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
                    sprintf(buffer, " %d - %03d  %s  %-40s%-14d%d\n", ++i_file, score, PLID, word, corrects, trials);
                    strcat(sb_file, buffer);
                    fclose(fp);
                }
            }
            free(filelist[n_entries]);
        }
        free(filelist);
        strcat(sb_file, "\n");
    }
    printf("findTopScores done\n");
    return i_file;
}

void scoreboard(int verbose){
    char sb_file[MAX_FILE_SIZE + 1], response[MAX_READ_SIZE+MAX_FILE_SIZE + 1];
    int n_scores = findTopScores(sb_file);
    printf("find top score done, n_scores=%d\n", n_scores);

    if (n_scores == 0)
        sprintf(response, "RSB EMPTY\n");
    
    else{
        sprintf(response, "RSB OK TOPSCORES_%07d.txt %zu %s\n", getppid(), strlen(sb_file), sb_file);
    }

    char *ptr = response;
    int n_left = strlen(response);
    while((n = write(newfd, ptr, n_left))!=0){
        if(n == -1){
            printf("ERROR\n");
            exit(1);
        } 
        ptr += n;
        n_left -= n;
    }
    if(verbose)
        printf("Sent: %s\n", response);
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
    
    fgets(buffer, MAX_READ_SIZE, fp);
    sscanf(buffer, "%s %s", word, hintfile);
    memset(game, '_', strlen(word));
    game[strlen(word)] = '\0';
    transactions[0] = '\0';

    while(fgets(buffer, MAX_READ_SIZE, fp) != NULL){
        sscanf(buffer, "%c %s", &code, previous);
        if (code == 'T'){
            if (strchr(word, previous[0])!=NULL){

                for (int i = 0; i<strlen(word); i++)
                    if (word[i] == previous[0])
                        game[i] = previous[0];
                
                sprintf(buffer, "     Letter trial: %c - TRUE\n", previous[0]);
            }
            else
                sprintf(buffer, "     Letter trial: %c - FALSE\n", previous[0]);
        }
        else if (code == 'G')
            sprintf(buffer, "     Word guess: %s\n", previous);
        
        strcat(transactions, buffer);
        n_trials ++;
    }
    file[0] = '\0';

    if (status){
        sprintf(buffer, "     Solved so far: %s\n", game);
        strcat(transactions, buffer);
        sprintf(file, "     Active  game found for player %s\n     --- Transactions found: %d ---\n", PLID, n_trials);
        strcat(file, transactions);
    }

    else{
        printf("yeah\n");
        sscanf(filename + strlen(FOLDER_GAMES) + MAX_PLID_SIZE + strlen("YYYYMMDD_HHMMSS_") + 1, "%c", &code);
        if (code == 'W')
            sprintf(buffer, "     Termination: WIN\n");
        else if (code == 'F')
            sprintf(buffer, "     Termination - FAIL\n");
        else if (code == 'Q')
            sprintf(buffer, "     Termination - QUIT\n");
        strcat(transactions, buffer);
        sprintf(file, "     Last finalized game for player %s\n     Word: %s; Hint file: %s\n     --- Transaction found: %d ---\n", PLID, word, hintfile, n_trials);
        strcat(file, transactions);

    }

}


void state(int verbose){
    char filename[MAX_FILENAME_SIZE + strlen(FOLDER_GAMES) + 1], state_filename[MAX_FILENAME_SIZE + 1];
    char PLID[MAX_PLID_SIZE+1], response[MAX_READ_SIZE+MAX_FILE_SIZE + 1];
    readPLID(PLID);
    if(verbose)
        printf("PLID: %s\n", PLID);
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
    while((n = write(newfd, ptr, n_left))!=0){
        if(n == -1){
            printf("ERROR\n");
            exit(1);
        } 
        ptr += n;
        n_left -= n;
    }
    if(verbose)
        printf("Sent: %s\n", response);
}