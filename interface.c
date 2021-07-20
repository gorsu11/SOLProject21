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
