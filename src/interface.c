#include "../includes/interface.h"
#include "../includes/conn.h"
#include "../includes/util.h"

#include <sys/socket.h>
#include <time.h>
#include <errno.h>
#include <libgen.h>
//////////////////

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


    struct timespec ct = {0, 0};
    while((connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) == -1 && compare_time(ct, abstime) == -1){
        usleep(1000* msec);
    }

    if(compare_time(ct, abstime) > 0){
        return -1;
    }

    memset(response, 0, LEN);

    SYSCALL_EXIT("readn", notused, readn(sockfd,response,LEN), "readn", "");

    if(!DEBUGAPI) printf("[INTERFACE] %s\n", response);
    strncpy(socket_name, sockname, LENSOCK);

    connection_socket = 1;

    if(DEBUGAPI) fprintf(stdout, "[INTERFACE] Connessione avvenuta con successo\n");

    return 0;

}

int closeConnection(const char* sockname){
    if(!sockname || strncmp(sockname, SOCKNAME, LENSOCK)){
        errno = EINVAL;
        return -1;
    }

    if(connection_socket == 0){
        errno = EINVAL;     //TODO: da modificare opportunamente
        return -1;
    }

    if(DEBUGAPI) fprintf(stdout, "[INTERFACE] Richiesta chiusura\n");


    if((close(sockfd)) == -1){
        return -1;
    }

    if(DEBUGAPI) fprintf(stdout, "[INTERFACE] Chiusura avvenuta con successo su closeConnection\n");
    return 0;
}

int openFile(const char* pathname, int flags){
    if(DEBUGAPI) fprintf(stdout, "[INTERFACE] Apertura file...\n");

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

    if(DEBUGAPI) printf("[INTERFACE] Ricevuto %s da openFile\n", response);

    char *t = strtok(response, ",");

    if(strcmp(t, "-1") == 0){       //Caso di fallimento dal server
        t = strtok(NULL, ",");
        errno = atoi(t);
        return -1;
    }

    if(DEBUGAPI) printf("[INTERFACE] File %s aperto con successo\n", pathname);
    return 0;
}

int closeFile(const char* pathname){

    if(DEBUGAPI) printf("[INTERFACE] Entra in closeFile\n");
    if(!pathname || connection_socket == 0){
        errno = EINVAL;
        return -1;
    }

    char buffer[LEN];
    memset(buffer, 0, LEN);

    sprintf(buffer, "closeFile,%s", pathname);

    SYSCALL_EXIT("writen", notused, writen(sockfd, buffer, LEN), "writen", "");

    SYSCALL_EXIT("readn", notused, readn(sockfd, response, LEN), "readn", "");

    if(DEBUGAPI) printf("[INTERFACE] Ricevuto %s\n", response);

    char *t = strtok(response, ",");

    if(strcmp(t, "-1") == 0){       //Caso di fallimento dal server
        t = strtok(NULL, ",");
        errno = atoi(t);
        return -1;
    }

    return 0;
}

int readFile(const char* pathname, void** buf, size_t* size){
    if(DEBUGAPI) fprintf(stdout, "[INTERFACE] Inizio readFile\n");
    if(!pathname || connection_socket == 0){
        fprintf(stderr, "Errore file\n");
        errno = EINVAL;
        return -1;
    }

    char buffer[LEN];
    memset(buffer, 0, LEN);

    sprintf(buffer, "readFile,%s", pathname);

    SYSCALL_EXIT("writen", notused, writen(sockfd, buffer, LEN), "writen", "");

    if(DEBUGAPI) fprintf(stdout, "[INTERFACE] Scritto al server il file %s\n", pathname);

    //ricevo la dimensione del file
    SYSCALL_EXIT("readn", notused, readn(sockfd, response, LEN), "readn", "");

    if(DEBUGAPI) printf("[INTERFACE] Ricevuto %s\n", response);

    char* t = strtok(response, ",");
    int size_file;

    if(DEBUGAPI) printf("[INTERFACE] Il valore di t è %s\n", t);
    if(strcmp(t, "-1") == 0){
        t = strtok(NULL, ",");
        errno = atoi(t);
        return -1;
    }
    else{
        if(DEBUGAPI) printf("[INTERFACE] Entra nell'else\n");
        size_file = atoi(t);
        *size = size_file;
    }

    //Invio la conferma al server
    SYSCALL_EXIT("writen", notused, writen(sockfd, "0", LEN), "writen", "");

    if(DEBUGAPI) fprintf(stdout, "[INTERFACE] Inviata al server la conferma\n");

    CHECKNULL(buf, malloc((size_file+1)*sizeof(char)), "malloc buf");

    //if(DEBUGAPI) printf("[INTERFACE] Il buffer contiene %s\n", (char*) buf);
    //nel caso in cui il buffer sia vuoto, altrimenti leggo dal buffer
    //if(*buf == NULL){
      //  errno = EINVAL;
      //  return -1;
    //}
    SYSCALL_EXIT("readn", notused, readn(sockfd, buf, size_file), "readn", "");

    if(DEBUGAPI) printf("[INTERFACE] Il buffer contiene\n%s\n", (char*) buf);
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


    char* t = strtok(bufrec, ",");

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

            //TODO: usare i DEFINE per aprire il file
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
    if(DEBUGAPI) printf("[INTERFACE] Il pathname è: %s\n[INTERFACE] La dirname è: %s\n", pathname, dirname);

    if(!pathname || connection_socket == 0){
        errno = EINVAL;
        return -1;
    }
    FILE *fi;
    int size_file;

    //SYSCALL_EXIT("fopen", fi, fopen(pathname, "rb"), "fopen writeFile", "");
    if((fi = fopen(pathname, "rb")) == NULL){
        errno = EINVAL;
        return -1;
    }

    char buffer[LEN];
    memset(buffer, 0, LEN);
    sprintf(buffer, "writeFile,%s", pathname);

    if(DEBUGAPI) printf("[INTERFACE] Il buffer è: %s\n", buffer);
    SYSCALL_EXIT("writen", notused, writen(sockfd, buffer, LEN), "writen", "");
    SYSCALL_EXIT("readn", notused, readn(sockfd, response, LEN), "readn", "");

    if(DEBUGAPI) printf("[INTERFACE] Il response è: %s\n", response);

    char* t = strtok(response, ",");

    if(DEBUGAPI) printf("[INTERFACE] Il valore di t è: %s\n", t);

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

        //CONFERMA DAL SERVER
        char conf1[LEN];
        memset(conf1, 0, LEN);
        SYSCALL_EXIT("readn", notused, readn(sockfd, conf1, LEN), "readn", "");

        //INVIO LA DIRECTORY
        char tmp1[LEN];
        memset(tmp1, 0, LEN);
        sprintf(tmp1, "%s", dirname);
        SYSCALL_EXIT("writen", notused, writen(sockfd, tmp1, LEN), "writen", "");

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
    //memset(buffer, 0, DIM_MSG);
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

  if(DEBUGAPI) printf("[INTERFACE] Inizio lockFile di %s\n", pathname);

  //INVIA IL COMANDO AL SERVER
  char buf[LEN];
  memset(buf, 0, LEN);
  sprintf(buf, "lockFile,%s", pathname);
  SYSCALL_EXIT("writen", notused, writen(sockfd, buf, LEN), "writen", "")

  if(DEBUGAPI) printf("[INTERFACE] Inviato al server il buffer %s\n", buf);

  //RICEVE IL RESPONSO DAL SERVER
  char buf1[LEN];
  memset(buf1, 0, LEN);
  SYSCALL_EXIT("readn", notused, readn(sockfd, buf1, LEN), "readn", "");

  if(DEBUGAPI) printf("[INTERFACE] Riceve il responso %s\n", buf1);

  char* t = strtok(buf1, ",");

  if(strcmp(t, "-1") == 0){
      t = strtok(NULL, ",");
      errno = atoi(t);
      return -1;
  }

  if(DEBUGAPI) printf("[INTERFACE] lockFile avvenuta con successo\n");
  return 0;
}

int unlockFile(const char* pathname){
  if(!pathname || connection_socket == 0){
      errno = EINVAL;
      return -1;
  }

  if(DEBUGAPI) printf("[INTERFACE] Inizio unlockFile di %s\n", pathname);

  //INVIA IL COMANDO AL SERVER
  char buf[LEN];
  memset(buf, 0, LEN);
  sprintf(buf, "unlockFile,%s", pathname);
  SYSCALL_EXIT("writen", notused, writen(sockfd, buf, LEN), "writen", "")

  if(DEBUGAPI) printf("[INTERFACE] Inviato al server il path %s\n", pathname);

  //RICEVE IL RESPONSO DAL SERVER
  char buf1[LEN];
  memset(buf1, 0, LEN);
  SYSCALL_EXIT("readn", notused, readn(sockfd, buf1, LEN), "readn", "");

  if(DEBUGAPI) printf("[INTERFACE] Riceve il responso %s\n", buf1);

  char* t = strtok(buf1, ",");

  if(DEBUGAPI) printf("[INTERFACE] Viene inviato al client %s\n", t);

  if(strcmp(t, "-1") == 0){
      t = strtok(NULL, ",");
      errno = atoi(t);
      return -1;
  }

  if(DEBUGAPI) printf("[INTERFACE] unlockFile avvenuta con successo\n");
  return 0;
}

int removeFile(const char* pathname){
    if(DEBUGAPI) printf("[INTERFACE] Entra in removeFile\n");

    if(!pathname || connection_socket == 0){
        errno = EINVAL;
        return -1;
    }

    char buffer[LEN];
    memset(buffer, 0, LEN);

    sprintf(buffer, "removeFile,%s", pathname);

    SYSCALL_EXIT("writen", notused, writen(sockfd, buffer, LEN), "writen", "");

    SYSCALL_EXIT("readn", notused, readn(sockfd, response, LEN), "readn", "");

    if(DEBUGAPI) printf("[INTERFACE] Ricevuto %s\n", response);

    char *t = strtok(response, ",");

    if(strcmp(t, "-1") == 0){       //Caso di fallimento dal server
        t = strtok(NULL, ",");
        errno = atoi(t);
        return -1;
    }

    if(DEBUGAPI) printf("[INTERFACE] removeFile avvenuta con successo\n");
    return 0;
}

//0 se sono uguali, 1 se a>b, -1 se a<b
int compare_time (struct timespec a, struct timespec b) {
    clock_gettime(CLOCK_REALTIME,&a);
    if (a.tv_sec == b.tv_sec) {
        if (a.tv_nsec > b.tv_nsec) return 1;
        else if (a.tv_nsec == b.tv_nsec) return 0;
        else return -1;
    }else if (a.tv_sec > b.tv_sec) return 1;
    else return -1;
}

int mkdir_p(const char *path) {

    const size_t len = strlen(path);
    char _path[1024];
    char *p;

    errno = 0;

    if (len > sizeof(_path)-1) {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(_path, path);

    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';

            if (mkdir(_path, S_IRWXU) != 0) {
                if (errno != EEXIST)
                    return -1;
            }

            *p = '/';
        }
    }

    if (mkdir(_path, S_IRWXU) != 0) {
        if (errno != EEXIST)
            return -1;
    }

    return 0;
}
