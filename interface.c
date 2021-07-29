#include "interface.h"
#include "conn.h"
#include "util.h"

#include <sys/socket.h>
#include <time.h>
#include <errno.h>
#include <libgen.h>

//TODO: sostituire EINVAL con gli errori corretti

int openConnection(const char* sockname, int msec, const struct timespec abstime){
    if(!sockname || msec < 0){
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
        if(DEBUGAPI) printf("Connessione non avvenuta, riproverà tra %d secondi", msec);
    }

    if(compare_time(ct, abstime) > 0){
        errno = EINVAL;
        return -1;
    }

    memset(response, 0, LEN);

    SYSCALL_EXIT("readn", notused, readn(sockfd,response,LEN), "readn", "");

    if(!DEBUGAPI) printf("%s\n", response);
    strncpy(socket_name, sockname, LENSOCK);

    connection_socket = 1;

    if(DEBUGAPI) fprintf(stdout, "connessione avvenuta con successo\n");

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
    return 0;
}

int writeFile(const char* pathname, const char* dirname){
    return 0;
}

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname){
    return 0;
}

int lockFile(const char* pathname){
    return 0;
}

int unlockFile(const char* pathname){
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
