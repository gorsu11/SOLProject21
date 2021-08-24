#include "interface.h"
#include "conn.h"
#include "util.h"

#include <sys/socket.h>
#include <time.h>
#include <errno.h>
#include <libgen.h>

//TODO: sostituire EINVAL con gli errori corretti

int openConnection(const char* sockname, int msec, const struct timespec abstime){
    if((!sockname) || (msec <=0)){
        errno = EINVAL;
        return -1;
    }

    struct sockaddr_un serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, sockname, UNIX_PATH_MAX);

    SYSCALL_EXIT("socket", sockfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket","");


    struct timespec ct;
    while((connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) == -1 && compare_time(ct, abstime) == -1){
        usleep(1000* msec);
    }

    if(compare_time(ct, abstime) > 0){
        return -1;
    }

    memset(response, 0, LEN);

    SYSCALL_EXIT("readn", notused, readn(sockfd,response,LEN), "readn", "");

    if(!DEBUG) printf("%s\n", response);
    strncpy(socket_name, sockname, LENSOCK);

    connection_socket = 1;

    if(DEBUG) fprintf(stdout, "connessione avvenuta con successo\n");

    return 0;

}


int closeConnection(const char* sockname){
    if(!sockname){
        errno = EINVAL;
        return -1;
    }

    if(connection_socket == 0){
        errno = EINVAL;
        return -1;
    }

    if(DEBUGAPI) fprintf(stdout, "Richiesta chiusura\n");


    if((close(sockfd)) == -1){
        return -1;
    }

    if(DEBUGAPI) fprintf(stdout, "chiusura avvenuta con successo\n");
    return 0;
}

int openFile(const char* pathname, int flags){
    if(DEBUGAPI) fprintf(stdout, "Apertura file...\n");

    if(!pathname){
        errno = EINVAL;
        return -1;
    }

    //piu o meno sono simili
    if(sockfd == -1 || connection_socket == 0){
        errno = ENOTCONN;
        return -1;
    }

    char buffer[LEN];
    memset(buffer, 0, LEN);

    sprintf(buffer, "openFile,%s,%d", pathname, flags);

    SYSCALL_EXIT("writen", notused, writen(sockfd, buffer, LEN), "writen", "");

    SYSCALL_EXIT("readn", notused, readn(sockfd, response, LEN), "readn", "");

    if(DEBUGAPI) printf("Ricevuto %s\n", response);

    char *t = strtok(response, ",");

    if(strcmp(t, "-1") == 0){       //Caso di fallimento dal server
        t = strtok(NULL, ",");
        errno = atoi(t);
        return -1;
    }

    return 0;
}

int readFile(const char* pathname, void** buf, size_t* size){

    if(!pathname || !buf || connection_socket == 0){
        fprintf(stderr, "Errore file\n");
        errno = EINVAL;
        return -1;
    }

    if(DEBUGAPI) fprintf(stdout, "Lettura file...\n");

    char buffer[LEN];
    memset(buffer, 0, LEN);

    sprintf(buffer, "readFile,%s", pathname);

    SYSCALL_EXIT("writen", notused, writen(sockfd, buffer, LEN), "writen", "");

    //ricevo la dimensione del file
    SYSCALL_EXIT("readn", notused, readn(sockfd, response, LEN), "readn", "");

    if(DEBUGAPI) printf("Ricevuto %s\n", response);

    char* t = strtok(response, ",");
    int size_file;

    if(strcmp(t, "-1") == 0){
        t = strtok(NULL, ",");
        errno = atoi(t);
        return -1;
    }
    else{
        size_file = atoi(t);
        *size = size_file;
    }

    //Invio la conferma al server
    SYSCALL_EXIT("writen", notused, writen(sockfd, "0", LEN), "writen", "");

    CHECKNULL(buf, malloc((size_file+1)*sizeof(char)), "malloc buf");

    //nel caso in cui il buffer sia vuoto, altrimenti leggo dal buffer
    if(*buf == NULL){
        errno = EINVAL;
        return -1;
    }
    SYSCALL_EXIT("readn", notused, readn(sockfd, buf, size_file), "readn", "");

    if(DEBUGAPI) printf("Lettura avvenuta con successo\n");
    return 0;
}

int readNFiles(int N, const char* dirname){
    if(connection_socket == 0){
        errno = EINVAL;
        return -1;
    }

    if(dirname != NULL){       //creo la directory se non esiste
        mkdir_p(dirname);
    }

    //INVIA IL COMANDO AL SERVER
    char bufsend[LEN];
    memset(bufsend, 0, LEN);
    sprintf(bufsend, "readNFile,%d", N);
    SYSCALL_EXIT("writen", notused, writen(sockfd, bufsend, LEN), "writen", "")

    //RICEVE IL NUMERO DI FILE CHE IL SERVER INVIA
    char bufrec[LEN];
    memset(bufrec, 0, LEN);
    SYSCALL_EXIT("readn", notused, readn(sockfd, bufrec, LEN), "readn", "");

    char* t = strtok(response, ",");

    if(strcmp(t, "-1") == 0){
        t = strtok(NULL, ",");
        errno = atoi(t);
        return -1;
    }

    int number = atoi(t);

    //Invio la conferma al server
    SYSCALL_EXIT("writen", notused, writen(sockfd, "ok", LEN), "writen", "");

    for(int i=0; i<number; i++){
        //RICEVO PATH
        char path[PATH_MAX];
        memset(path, 0, PATH_MAX);
        SYSCALL_EXIT("readn", notused, readn(sockfd, path, LEN), "readn", "");

        char* t1 = strtok(path, ",");
        if(strcmp(t1, "-1") == 0){
            t1 = strtok(NULL, ",");
            errno = atoi(t1);
            return -1;
        }

        //Invio la conferma al server
        SYSCALL_EXIT("writen", notused, writen(sockfd, "ok", LEN), "writen", "");

        //RICEVO SIZE
        char ssize[LEN];
        memset(ssize, 0, LEN);
        SYSCALL_EXIT("readn", notused, readn(sockfd, ssize, LEN), "readn", "");

        char *t2 = strtok(ssize,",");
        if (strcmp(t2,"-1")==0) { //ERRORE DAL SERVER
            t2 = strtok(NULL,",");
            errno = atoi(t2);
            return -1;
        }

        int size_file = atoi(ssize);
        //Invio la conferma al server
        SYSCALL_EXIT("writen", notused, writen(sockfd, "ok", LEN), "writen", "");

        //RICEVO FILE
        char* fbuf;
        CHECKNULL(fbuf, malloc(size_file*sizeof(char)), "malloc fbuf");
        SYSCALL_EXIT("readn", notused, readn(sockfd, fbuf, size_file), "readn", "");

        char *t3 = strtok(fbuf,",");
        if (t3!=NULL) {
            if (strcmp(t3,"-1")==0) { //ERRORE DAL SERVER
                t3 = strtok(NULL,",");
                errno = atoi(t3);
                return -1;
            }
        }

        if(dirname != NULL){
            //salva in dir
            char sp[PATH_MAX];
            memset(sp, 0, PATH_MAX);
            char* file_name = basename(path);
            sprintf(sp,"%s/%s",dirname,file_name);

            FILE* of;
            of = fopen(sp,"w");
            if (of==NULL) {
                printf("Errore aprendo il file\n");
            } else {
                fprintf(of,"%s",fbuf);
                fclose(of);
            }
        }
        free(fbuf);
    }

    return number;
}

int writeFile(const char* pathname, const char* dirname){
    if(!pathname || !dirname || connection_socket == 0){
        errno = EINVAL;
        return -1;
    }
    FILE *fi;
    int size_file;

    if((fi = fopen(pathname, "rb")) == NULL){
        errno = EINVAL;
        return -1;
    }

    char buffer[LEN];
    memset(buffer, 0, LEN);
    sprintf(buffer, "writeFile,%s", pathname);

    SYSCALL_EXIT("writen", notused, writen(sockfd, buffer, LEN), "writen", "");
    SYSCALL_EXIT("readn", notused, readn(sockfd, response, LEN), "readn", "");

    char* t = strtok(response, ",");

    if(strcmp(t, "-1") == 0){
        t = strtok(NULL, ",");
        errno = atoi(t);
        return -1;
    }

    struct stat st;
    stat(pathname, &st);
    size_file = (int) st.st_size;

    if(size_file > 0){
        char* file_buffer;
        CHECKNULL(file_buffer, malloc((size_file+1)*sizeof(char)), "malloc write");

        size_t newLen = fread(file_buffer, sizeof(char), size_file, fi);
        if(ferror(fi) != 0){
            errno = EINVAL;
            free(file_buffer);
            return -1;
        }
        else{
            file_buffer[newLen++] = '\0';
        }
        fclose(fi);

        //INVIO SIZE FILE
        char tmp[LEN];
        memset(tmp, 0, LEN);
        sprintf(tmp, "%d", size_file);
        SYSCALL_EXIT("writen", notused, writen(sockfd, tmp, LEN), "writen", "");

        //CONFERMA DAL SERVER
        char conf[LEN];
        memset(conf, 0, LEN);
        SYSCALL_EXIT("readn", notused, readn(sockfd, conf, LEN), "readn", "");

        //INVIO FILE
        SYSCALL_EXIT("writen", notused, writen(sockfd, file_buffer, size_file+1), "writen", "");
        free(file_buffer);

        //RISPOSTA SERVER
        char result[LEN];
        memset(result, 0, LEN);
        SYSCALL_EXIT("readn", notused, readn(sockfd, result, LEN), "readn", "");

        char * t1;
        t1 = strtok(result,",");

        if (strcmp(t1,"-1")==0) { //ERRORE DAL SERVER
            t1 = strtok(NULL,",");
            errno = atoi(t1);
            return -1;
        }
    }

    return 0;
}

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname){
    if(!pathname || !dirname || connection_socket == 0){
        errno = EINVAL;
        return -1;
    }

    char buffer[LEN];
    sprintf(buffer, "appendToFile,%s", pathname);

    SYSCALL_EXIT("writen", notused, writen(sockfd, buffer, LEN), "writen", "");
    SYSCALL_EXIT("readn", notused, readn(sockfd, response, LEN), "readn", "");

    char* t = strtok(response, ",");

    if(strcmp(t, "-1") == 0){
        t = strtok(NULL, ",");
        errno = atoi(t);
        return -1;
    }

    //INVIO SIZE FILE
    char tmp[LEN];
    memset(tmp, 0, LEN);
    SYSCALL_EXIT("writen", notused, writen(sockfd, tmp, LEN), "writen", "");

    //CONFERMA DAL SERVER
    char conf[LEN];
    memset(conf, 0, LEN);
    SYSCALL_EXIT("readn", notused, readn(sockfd, conf, LEN), "readn", "");

    //INVIO I FILE
    SYSCALL_EXIT("writen", notused, writen(sockfd, buf, size), "writen", "");

    //RISPOSTA DAL SERVER
    char result[LEN];
    memset(result, 0, LEN);
    SYSCALL_EXIT("readn", notused, readn(sockfd, result, LEN), "readn", "");

    char * t1;
    t1 = strtok(result,",");

    if (strcmp(t1,"-1")==0) { //ERRORE DAL SERVER
        t1 = strtok(NULL,",");
        errno = atoi(t1);
        //free(buf);
        return -1;
    }

    return 0;
}

int lockFile(const char* pathname){
    if(!pathname || connection_socket == 0){
        errno = EINVAL;
        return -1;
    }

    char buffer[LEN];
    memset(buffer, 0, LEN);

    if(DEBUGAPI) printf("Inizio lockFile di %s\n", pathname);

    sprintf(buffer, "lockFile,%s", pathname);

    SYSCALL_EXIT("writen", notused, writen(sockfd, buffer, LEN), "writen", "");

    SYSCALL_EXIT("readn", notused, readn(sockfd, response, LEN), "readn", "");

    if(DEBUGAPI) printf("Ricevuto %s\n", response);

    char* t = strtok(response, ",");
    if(strcmp(t, "-1") == 0){
        t = strtok(NULL, ",");
        errno = atoi(t);
        return -1;
    }

    if(DEBUGAPI) printf("lockFile avvenuta con successo\n");
    return 0;
}

int unlockFile(const char* pathname){
    if(!pathname || connection_socket == 0){
        errno = EINVAL;
        return -1;
    }

    char buffer[LEN];
    memset(buffer, 0, LEN);

    if(DEBUGAPI) printf("Inizio unlockFile di %s\n", pathname);

    sprintf(buffer, "unlockFile,%s", pathname);

    SYSCALL_EXIT("writen", notused, writen(sockfd, buffer, LEN), "writen", "");

    SYSCALL_EXIT("readn", notused, readn(sockfd, response, LEN), "readn", "");

    if(DEBUGAPI) printf("Ricevuto %s\n", response);

    char* t = strtok(response, ",");
    if(strcmp(t, "-1") == 0){
        t = strtok(NULL, ",");
        errno = atoi(t);
        return -1;
    }

    if(DEBUGAPI) printf("unlockFile avvenuta con successo\n");
    return 0;
}

int closeFile(const char* pathname){
    if(!pathname || connection_socket == 0){
        errno = EINVAL;
        return -1;
    }

    if(DEBUGAPI) fprintf(stdout, "Chiusura file...\n");

    char buffer[LEN];
    memset(buffer, 0, LEN);

    sprintf(buffer, "closeFile,%s", pathname);

    SYSCALL_EXIT("writen", notused, writen(sockfd, buffer, LEN), "writen", "");

    SYSCALL_EXIT("readn", notused, readn(sockfd, response, LEN), "readn", "");

    if(DEBUGAPI) printf("Ricevuto %s\n", response);

    char *t = strtok(response, ",");

    if(strcmp(t, "-1") == 0){       //Caso di fallimento dal server
        t = strtok(NULL, ",");
        errno = atoi(t);
        return -1;
    }

    return 0;
}

int removeFile(const char* pathname){
    if(!pathname || connection_socket == 0){
        errno = EINVAL;
        return -1;
    }

    if(DEBUGAPI) fprintf(stdout, "Rimozione file...\n");

    char buffer[LEN];
    memset(buffer, 0, LEN);

    sprintf(buffer, "removeFile,%s", pathname);

    SYSCALL_EXIT("writen", notused, writen(sockfd, buffer, LEN), "writen", "");

    SYSCALL_EXIT("readn", notused, readn(sockfd, response, LEN), "readn", "");

    if(DEBUGAPI) printf("Ricevuto %s\n", response);

    char *t = strtok(response, ",");

    if(strcmp(t, "-1") == 0){       //Caso di fallimento dal server
        t = strtok(NULL, ",");
        errno = atoi(t);
        return -1;
    }

    return 0;
}
