#include "server_TCP.h"

int fd, newfd, errno, errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;

char buffer[MAX_READ_SIZE + 1];

void TCP_command(char *port, int verbose){

    TCP_open_socket(port);
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
                printf("Received: %s ", buffer);

            if(!strcmp(buffer, "GSB")){
                //scoreboard();
            } else if(!strcmp(buffer, "GHL")){
                hint(verbose);
            } else if(!strcmp(buffer, "STA")){
                //state();
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



void TCP_open_socket(char *port){
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
}



void write_file(char *filename, char *buffer, int verbose){
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

void read_PLID(char *PLID){
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
    read_PLID(PLID);
    if (verbose)
        printf("%s\n", PLID);
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
                printf("ERROR\n");
                exit(1);
            } 
            ptr += n;
            n_left -= n;
        }
        if(verbose)
            printf("Sent: %s", buffer);

        write_file(filename, buffer, verbose);

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
