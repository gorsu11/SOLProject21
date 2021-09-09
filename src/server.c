#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <libgen.h>

#include "../includes/util.h"
#include "../includes/conn.h"
#include "parsingFile.c"

int notused;

typedef struct node {
    int data;
    struct node  * next;
} node;


//--------------- GESTIONE FILE ----------------//
typedef struct file {
    char path[PATH_MAX];            //nome
    char * data;                    //contenuto file
    node * client_open;             //lista client che lo hanno aperto
    int lock_file;                  //variabile che mi tiene conto se un file è in lock da parte di un client
    int client_write;               //FILE DESCRIPTOR DEL CLIENT CHE HA ESEGUITO COME ULTIMA OPERAZIONE UNA openFile con flag O_CREATE
    struct file * next;
} file;

pthread_mutex_t lock_cache = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_file = PTHREAD_MUTEX_INITIALIZER;
int dim_byte; //DIMENSIONE CACHE IN BYTE
int num_files; //NUMERO FILE IN CACHE

//varibili globali per statistiche finali
int top_files=0;
int top_dim=0;
int replace = 0;
//-----------------------------------------------//


volatile sig_atomic_t term = 0; //FLAG SETTATO DAL GESTORE DEI SEGNALI DI TERMINAZIONE
static void gestore_term (int signum);

//STRUTTURA DATI PER SALVARE I FILE
file* cache = NULL;
char directory_name[LEN];

config* configuration;

//CODA DI COMUNICAZIONE MANAGER --> WORKERS / RISORSA CONDIVISA / CODA FIFO
node * coda = NULL;
pthread_mutex_t lock_coda = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

void insertNode (node ** list, int data);
int removeNode (node ** list);
int updatemax(fd_set set, int fdmax);

//-------------------- FUNZIONI SOLO DICHIARATE --------------------//
int aggiungiFile(char* path, int flag, int cfd, char* dirname);
int rimuoviCliente(char* path, int cfd);
int rimuoviFile(char* path, int cfd);
int inserisciDati(char* path, char* data, int size, int cfd, char* dirname);
int appendDati(char* path, char* data, int size, int cfd,  char* dirname);
int bloccaFile(char* path, int cfd);
int sbloccaFile(char* path, int cfd);
char* prendiFile (char* path, int cfd);
void printFile (void);
void freeList(node** head);
int fileOpen(node* list, int cfd);
int resizeCache(int dim, char* dirname);
void opzioneD(file* file, char* dirname);
int mkdir_p(const char *path);
void printClient (node * list);

void execute (char * request, int cfd,int pfd);

void* Workers(void *argument);

void cleanup() {
    unlink(configuration->socket_name);
}

//------------------------------------------------------------//

int main(int argc, char *argv[]) {

    //-------------------- PARSING FILE --------------------//
    if (argc != 2) {
        fprintf(stderr, "Error data in %s\n", argv[0]);
        return -1;
    }

    //se qualcosa non va a buon fine prende i valori di default
    if((configuration = getConfig(argv[1])) == NULL){
        configuration = default_configuration();
        printf("Presi i valori di default\n");
    }

    if(DEBUGSERVER) stampa(configuration);
    //------------------------------------------------------------//


    //-------------------- GESTIONE SEGNALI --------------------//
    struct sigaction s;
    sigset_t sigset;

    SYSCALL_EXIT("sigfillset", notused, sigfillset(&sigset),"sigfillset", "");
    SYSCALL_EXIT("pthread_sigmask", notused, pthread_sigmask(SIG_SETMASK,&sigset,NULL),"pthread_sigmask", "");
    memset(&s,0,sizeof(s));
    s.sa_handler = gestore_term;

    SYSCALL_EXIT("sigaction", notused, sigaction(SIGINT, &s, NULL),"sigaction", "");
    SYSCALL_EXIT("sigaction", notused, sigaction(SIGQUIT, &s, NULL),"sigaction", "");
    SYSCALL_EXIT("sigaction", notused, sigaction(SIGHUP, &s, NULL),"sigaction", ""); //TERMINAZIONE SOFT

    //ignoro SIGPIPE
    s.sa_handler = SIG_IGN;
    SYSCALL_EXIT("sigaction", notused, sigaction(SIGPIPE, &s, NULL),"sigaction", "");

    SYSCALL_EXIT("sigemptyset", notused, sigemptyset(&sigset),"sigemptyset", "");
    int e;
    SYSCALL_PTHREAD(e,pthread_sigmask(SIG_SETMASK, &sigset, NULL),"pthread_sigmask");
    //-------------------------------------------------------------//


    //-------------------- CREAZIONE PIPE ---------------------//

    int comunication[2]; //comunica tra master e worker
    SYSCALL_EXIT("pipe", notused, pipe(comunication), "pipe", "");


    //-------------------- CREAZIONE THREAD WORKERS --------------------//
    pthread_t *master;
    CHECKNULL(master, malloc(configuration->num_thread*sizeof(pthread_t)), "malloc pthread_t");

    if(DEBUGSERVER) printf("[SERVER] Creazione dei %d thread Worker\n", configuration->num_thread);

    int err;
    for(int i=0; i<configuration->num_thread; i++){
        SYSCALL_PTHREAD(err, pthread_create(&master[i], NULL, Workers, (void*) (&comunication[1])), "pthread_create pool");
    }

    if(DEBUGSERVER) printf("[SERVER] Creazione andata a buon fine\n");

    //-------------------------------------------------------------//



    //-------------------- CREAZIONE SOCKET --------------------//

    // se qualcosa va storto ....
    atexit(cleanup);

    // cancello il socket file se esiste
    cleanup();

    int listenfd,
    fd;
    int num_fd = 0;
    fd_set set;
    fd_set rdset;

    //PER LA TERMINAZIONE SOFT --> con SIGHUP aspetta che tutti i client si disconnettano
    int num_client = 0;
    int soft_term = 0;

    // setto l'indirizzo
    struct sockaddr_un serv_addr;
    strncpy(serv_addr.sun_path, configuration->socket_name, UNIX_PATH_MAX);

    serv_addr.sun_family = AF_UNIX;

    // creo il socket
    SYSCALL_EXIT("socket", listenfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket", "");

    // assegno l'indirizzo al socket
    SYSCALL_EXIT("bind", notused, bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr)), "bind", "");

    // setto il socket in modalita' passiva e definisco un n. massimo di connessioni pendenti
    SYSCALL_EXIT("listen", notused, listen(listenfd, SOMAXCONN), "listen", "");

    //MANTENGO IL MAX INDICE DI DESCRITTORE ATTIVO IN NUM_FD
    if (listenfd > num_fd) num_fd = listenfd;
    //REGISTRO IL WELCOME SOCKET
    FD_ZERO(&set);
    FD_SET(listenfd,&set);
    //REGISTRO LA PIPE
    if (comunication[0] > num_fd) num_fd = comunication[0];
    FD_SET(comunication[0],&set);

    printf("Listen for clients...\n");

    while (1) {
        rdset = set;
        if (select(num_fd+1,&rdset,NULL,NULL,NULL) == -1) {
            if (term==1) break;
            else if (term==2) {
                if (num_client==0) break;
                else {
                    printf("Chiusura Soft...\n");
                    FD_CLR(listenfd,&set);
                    if (listenfd == num_fd) num_fd = updatemax(set,num_fd);
                    close(listenfd);
                    rdset = set;
                    SYSCALL_EXIT("select", notused, select(num_fd+1, &rdset, NULL, NULL, NULL),"Errore select", "");
                }
            }else {
                perror("select");
                break;
            }
        }

        int cfd;
        for (fd=0;fd<= num_fd;fd++) {
            if (FD_ISSET(fd,&rdset)) {
                if (fd == listenfd) {  //WELCOME SOCKET PRONTO X ACCEPT
                    if ((cfd = accept(listenfd, NULL, 0)) == -1) {
                        if (term==1) break;
                        else if (term==2) {
                            if (num_client==0) break;
                        }else {
                            perror("Errore accept client");
                        }
                    }
                    FD_SET(cfd,&set);
                    if (cfd > num_fd) num_fd = cfd;
                    num_client++;
                    printf ("Connection accepted from client!\n");
                    char buf [LEN];
                    memset(buf,0,LEN);
                    strcpy(buf,"BENVENUTI NEL SERVER!");
                    if (writen(cfd,buf,LEN)==-1) {
                        perror("Errore write welcome message");
                        FD_CLR(cfd,&set);
                        if (cfd == num_fd) num_fd = updatemax(set,num_fd);
                        close(cfd);
                        num_client--;
                    }

                } else if (fd == comunication[0]) { //CLIENT DA REINSERIRE NEL SET -- PIPE PRONTA IN LETTURA
                    int cfd1;
                    int len;
                    int flag;
                    if ((len = (int) read(comunication[0],&cfd1,sizeof(cfd1))) > 0) { //LEGGO UN INTERO == 4 BYTES
                        //printf ("Master : client %d ritornato\n",cfd1);
                        SYSCALL_EXIT("readn", notused, readn(comunication[0], &flag, sizeof(flag)),"Master : read pipe", "");
                        if (flag == -1) { //CLIENT TERMINATO LO RIMUOVO DAL SET DELLA SELECT
                            printf("Closing connection with client...\n");
                            FD_CLR(cfd1,&set);
                            if (cfd1 == num_fd) num_fd = updatemax(set,num_fd);
                            close(cfd1);
                            num_client--;
                            if (term==2 && num_client==0) {
                                printf("Chiusura soft\n");
                                soft_term=1;
                            }
                        }else{
                            FD_SET(cfd1,&set);
                            if (cfd1 > num_fd) num_fd = cfd1;
                        }
                    }else if (len == -1){
                        perror("Master : read pipe");
                        exit(EXIT_FAILURE);
                    }

                } else { //SOCKET CLIENT PRONTO X READ
                    //QUINDI INSERISCO FD SOCKET CLIENT NELLA CODA
                    insertNode(&coda,fd);
                    FD_CLR(fd,&set);
                    //printf("Il cfd del client connesso è %d\n", fd);
                }
            }
        }
        if (soft_term==1) break;
    }

    printf("Closing server...\n");
    for (int i=0;i<configuration->num_thread;i++) {
        insertNode(&coda,-1);
    }
    for (int i=0;i<configuration->num_thread;i++) {
        SYSCALL_PTHREAD(e,pthread_join(master[i],NULL), "Errore join thread");
    }

    printf("------------------STATISTICHE SERVER-------------------\n");
    printf("Numero di file massimo = %d\n",top_files);
    printf("Dimensione massima = %f Mbytes\n",(top_dim/pow(10,6)));
    printf("Chiamate algoritmo di rimpiazzamento cache = %d\n",replace);
    printFile();
    printf("-------------------------------------------------------\n");

    SYSCALL_EXIT("close", notused, close(listenfd), "close socket", "");

    free(master);
    freeConfig(configuration);

    printf("connection done\n");


}

void* Workers(void *argument){
    if(DEBUGSERVER) printf("[SERVER] Entra nel thread Workers\n");

    int pfd = *((int*) argument);
    int cfd;

    while(1){
        char request[LEN];
        memset(request, 0, LEN);

        //PRELEVA UN CLIENTE DALLA CODA
        cfd = removeNode(&coda);
        if(cfd == -1){
            break;
        }

        //SERVO IL CLIENT
        int len;
        int fine; //FLAG COMUNICATO AL MASTER PER SAPERE QUANDO TERMINA IL CLIENT

        if((len = readn(cfd, request, LEN)) == 0){
            fine = -1;
            SYSCALL_EXIT("writen", notused, writen(pfd, &cfd, sizeof(cfd)), "thread writen", "");
            SYSCALL_EXIT("writen", notused, writen(pfd, &fine, sizeof(fine)), "thread writen", "");
        }
        else if(len == -1){
            fine = -1;
            SYSCALL_EXIT("writen", notused, writen(pfd, &cfd, sizeof(cfd)), "thread writen", "");
            SYSCALL_EXIT("writen", notused, writen(pfd, &fine, sizeof(fine)), "thread writen", "");
        }
        else{
            if(DEBUGSERVER) printf("[SERVER] Richiesta: %s\n", request);
            execute(request, cfd, pfd);
        }
    }
    printf("Closing worker\n");
    fflush(stdout);
    return 0;
}

void execute (char * request, int cfd,int pfd){
    char response[LEN];
    memset(response, 0, LEN);

    char* token = strtok(request, ",");
    int s;

    if(DEBUGSERVER) printf("[SERVER] Token estratto: %s\n", token);

    if(token != NULL){
        if(strcmp(token, "openFile") == 0){
            if(DEBUGSERVER) printf("[SERVER] Entro in openFile\n");
            //estraggo i valori
            token = strtok(NULL, ",");
            char path[PATH_MAX];

            if(DEBUGSERVER) printf("[SERVER] L'argomento è %s\n", token);

            strcpy(path, token);
            token = strtok(NULL, ",");

            if(DEBUGSERVER) printf("[SERVER] Dopo la strcpy ho path:%s e token:%s\n", path, token);

            int flag = atoi(token);

            int result;

            if(DEBUGSERVER) printf("[SERVER] Chiamo aggiungiFile con path %s\n", path);

            if(DEBUGSERVER) printf("La directory è %s\n", directory_name);

            if((result = aggiungiFile(path, flag, cfd, directory_name)) == -1){
                sprintf(response,"-1, %d",ENOENT);
            }
            else if (result == -2) {
                sprintf(response,"-2, %d",EEXIST);
            }
            else if (result == -3) {
                sprintf(response,"-3, %d",ENOLCK);
            }
            else{
                sprintf(response,"0");
            }

            if(DEBUGSERVER) printf("[SERVER] Il responso di openFile è %s ed il risultato è %d\n", response, result);
            SYSCALL_WRITE(writen(cfd, response, LEN), "THREAD : socket write");
        }

        else if(strcmp(token, "closeFile") == 0){
            //estraggo il valore
            token = strtok(NULL, ",");
            char path[PATH_MAX];

            strncpy(path, token, PATH_MAX);

            if(DEBUGSERVER) printf("[SERVER] Dopo la strcpy di closeFile ho path:%s e token:%s\n", path, token);
            int result;
            if((result = rimuoviCliente(path, cfd)) == -1){
                sprintf(response,"-1, %d",ENOENT);
            }
            else if (result == -2) {
                sprintf(response,"-2, %d",EPERM);
            }
            else{
                sprintf(response,"0");
            }

            if(DEBUGSERVER) printf("[SERVER] Il risultato della closeFile è: %d\n", result);

            SYSCALL_WRITE(writen(cfd, response, LEN), "THREAD : socket write");
        }

        else if(strcmp(token, "removeFile") == 0){

            if(DEBUGSERVER) printf("[SERVER] Entro in removeFile\n");
            //estraggo il valore
            token = strtok(NULL, ",");
            char path[PATH_MAX];
            strncpy(path, token, PATH_MAX);

            if(DEBUGSERVER) printf("[SERVER] Dopo la strcpy di removeFile ho path:%s e token:%s\n", path, token);

            int res;
            if((res = rimuoviFile(path, cfd)) == -1){
                sprintf(response,"-1,%d",ENOENT);
            }
            else if(res == -2){
                sprintf(response,"-2,%d",ENOLCK);
            }
            else{
                sprintf(response,"0");
            }

            if(DEBUGSERVER) printf("[SERVER] Il response della removeFile è: %s\n", response);

            SYSCALL_WRITE(writen(cfd,response,LEN),"THREAD : socket write");
        }

        else if(strcmp(token, "writeFile") == 0){

            if(DEBUGSERVER) printf("[SERVER] Entro in writeFile\n");
            //estraggo il valore
            token = strtok(NULL, ",");
            char path[PATH_MAX];
            strncpy(path, token, PATH_MAX);

            //invia al client il permesso di inviare file
            sprintf(response, "0");
            SYSCALL_WRITE(writen(cfd, response, LEN), "writeFile: socket write");

            //ricevo size del file
            char buf1[LEN];
            memset(buf1, 0, LEN);
            SYSCALL_READ(s, readn(cfd, buf1, LEN), "writeFile: socket read");

            if(DEBUGSERVER) printf("[SERVER] Ricevuto dal client la size %s\n", buf1);

            int size = atoi(buf1);

            //invio la conferma al client
            SYSCALL_WRITE(writen(cfd, "0", LEN), "writeFile: conferma al client");

            //ricevo i file
            char* buf2;
            CHECKNULL(buf2, malloc((size+1)*sizeof(char)), "malloc buf2");

            SYSCALL_READ(s, readn(cfd, buf2, (size+1)), "writeFile: socket read");

            if(DEBUGSERVER) printf("[SERVER] Ricevuto dal client il testo\n");

            //invio la conferma al client
            SYSCALL_WRITE(writen(cfd, "0", LEN), "writeFile: conferma file ricevuto dal client");

            //char buf3[LEN];
            memset(directory_name, 0, LEN);
            SYSCALL_READ(s, readn(cfd, directory_name, LEN), "writeFile: socket read directory");

            if(DEBUGSERVER) printf("[SERVER] Ricevuto dal client la directory %s\n", directory_name);

            //inserisco a questo punto i dati nella cache
            char result[LEN];
            memset(result, 0, LEN);

            int res;
            if((res = inserisciDati(path, buf2, size+1, cfd, directory_name)) == -1){
                sprintf(result, "-1,%d", ENOENT);
            }
            else if(res == -2){
                sprintf(result, "-2,%d", EPERM);
            }
            else if(res == -3){
                sprintf(result, "-3,%d", EFBIG);
            }
            else if(res == -4){
                sprintf(result, "-4,%d", ENOLCK);
            }
            else{
                sprintf(result, "0");
            }

            if(DEBUGSERVER) printf("[SERVER] Il responso di writeFile è %s ed il risultato è %d\n", result, res);

            free(buf2);
            SYSCALL_WRITE(writen(cfd, result, LEN), "writeFile: socket write result");
        }

        else if(strcmp(token, "appendToFile") == 0){
            //estraggo i valori
            token = strtok(NULL, ",");
            char path[PATH_MAX];
            strncpy(path, token, PATH_MAX);

            //invia al client il permesso di inviare file
            sprintf(response, "0");
            SYSCALL_WRITE(writen(cfd, response, LEN), "appendToFile: socket write");

            //ricevo size del file
            char buf1[LEN];
            SYSCALL_READ(s, readn(cfd, buf1, LEN), "appendToFile: socket read");

            if(DEBUGSERVER) printf("[SERVER] Ricevuto dal client la size %s\n", buf1);

            int size = atoi(buf1);

            //invio la conferma al client
            SYSCALL_WRITE(writen(cfd, "0", LEN), "appendToFile: conferma al client");

            //ricevo i file
            char* buf2;
            CHECKNULL(buf2, malloc((size+1)*sizeof(char)), "malloc buf2");

            SYSCALL_READ(s, readn(cfd, buf2, (size+1)), "appendToFile: socket read");

            if(DEBUGSERVER) printf("[SERVER] Ricevuto dal client %s\n", buf2);

            //invio la conferma al client
            SYSCALL_WRITE(writen(cfd, "0", LEN), "writeFile: conferma file ricevuto dal client");

            //char buf3[LEN];
            memset(directory_name, 0, LEN);
            SYSCALL_READ(s, readn(cfd, directory_name, LEN), "appendToFile: socket read directory");

            char result[LEN];
            int res;

            if((res = appendDati(path, buf2, size+1, cfd, directory_name)) == -1){
                sprintf(result, "-1,%d", ENOENT);
            }
            else if(res == -2){
                sprintf(result, "-2,%d", EPERM);
            }
            else if(res == -3){
                sprintf(result, "-3,%d", EFBIG);
            }
            else if(res == -4){
                sprintf(result, "-4,%d", ENOLCK);
            }
            else{
                sprintf(result, "0");
            }

            free(buf2);
            SYSCALL_WRITE(writen(cfd, result, LEN), "appendToFile: socket write result");
        }

        else if(strcmp(token, "readFile") == 0){
            //estraggo gli argomenti

            if(DEBUGSERVER) printf("[SERVER] Entro in readFile\n");

            token = strtok(NULL, ",");
            char path[PATH_MAX];
            strcpy(path, token);

            if(DEBUGSERVER) printf("[SERVER] Dopo la strcpy di readFile ho path:%s e token:%s\n", path, token);

            char* file = prendiFile(path, cfd);

            if(DEBUGSERVER) printf("[SERVER] prendiFile mi ritorna %s\n", file);

            char buf[LEN];
            memset(buf, 0, LEN);

            if(file == NULL){
                //invio un errore al client
                sprintf(buf, "-1,%d", EPERM);
                SYSCALL_WRITE(writen(cfd, buf, LEN), "readFile: socket write");
            }
            else{
                //invio la size del file
                sprintf(buf, "%ld", strlen(file));
                SYSCALL_WRITE(writen(cfd, buf, LEN),"readFile: socket write size file");

                if(DEBUGSERVER) printf("[SERVER] La dimensione del file nella readFile è %s\n", buf);

                char buf1[LEN];
                memset(buf1, 0, LEN);
                SYSCALL_READ(s, readn(cfd, buf1, LEN), "readFile: socket read response");

                if(DEBUGSERVER) printf("[SERVER] Ricevuto il responso dal client %s\n", buf1);
                fflush(stdout);

                if(strcmp(buf1, "0") == 0){
                    if(DEBUGSERVER) printf("[SERVER] La dimensione del file è %lu\n", strlen(file));
                    //se è andato tutto bene invio i file
                    SYSCALL_WRITE(writen(cfd, file, strlen(file)),"readFile: socket write file");
                    if(DEBUGSERVER) printf("[SERVER] Inviato al client il testo\n");
                }
            }
        }

        else if(strcmp(token, "readNFile") == 0){
            //estraggo gli argomenti
            token = strtok(NULL, ",");
            int num = atoi(token);
            int err;

            if(DEBUGSERVER) printf("[SERVER] File Richiesti: %d\n[SERVER] File esistenti: %d\n", num, num_files);

            SYSCALL_PTHREAD(err, pthread_mutex_lock(&lock_cache), "Lock Cache");

            //controllo sul valore num
            if((num <= 0) || (num > num_files)){
                num = num_files;
            }

            if(DEBUGSERVER) printf("[SERVER] Il valore di num è %d\n", num);
            //invio il numero al client
            char buf[LEN];
            memset(buf, 0, LEN);
            sprintf(buf, "%d", num);

            SYSCALL_WRITE(writen(cfd, buf, LEN), "readNFile: socket write num");

            if(DEBUGSERVER) printf("[SERVER] Invio al client il numero di file che mandera, cioe %s\n", buf);
            //ricevo la conferma del client
            char conf[LEN];
            memset(conf, 0, LEN);

            SYSCALL_READ(s, readn(cfd, conf, LEN), "readNFile: socket read response");

            if(DEBUGSERVER) printf("[SERVER] Il responso del client è %s\n", conf);
            //invio gli N files
            file* curr = cache;
            for(int i=0; i<num; i++){
                //invio il path al client
                char path[LEN];
                memset(path, 0, LEN);
                sprintf(path, "%s", curr->path);
                SYSCALL_WRITE(write(cfd, path, LEN), "readNFile: socket write path");

                //ricevo la conferma
                char conf [LEN];
                memset(conf, 0, LEN);
                SYSCALL_READ(s,readn(cfd, conf, LEN), "readNFile: socket read response");

                //invio size
                char ssize [LEN];
                memset(ssize, 0, LEN);
                sprintf(ssize, "%ld", strlen(curr->data));
                SYSCALL_WRITE(writen(cfd, ssize, LEN), "readNFile: socket write size");

                //ricevo la conferma
                char conf2[LEN];
                memset(conf2, 0, LEN);
                SYSCALL_READ(s, readn(cfd, conf2, LEN), "readNFile: socket read response");

                //invio file
                SYSCALL_WRITE(writen(cfd, curr->data, strlen(curr->data)), "readNFile: socket write file");

                curr = curr->next;
            }

            pthread_mutex_unlock(&lock_cache);
        }

        else if(strcmp(token, "lockFile") == 0){
            //estraggo gli argomenti

            if(DEBUGSERVER) printf("[SERVER] Entro in lockFile\n");

            //SYSCALL_WRITE(writen(cfd, response, LEN), "lockFile: socket write response");
            token = strtok(NULL, ",");
            char path[PATH_MAX];
            strcpy(path, token);

            if(DEBUGSERVER) printf("[SERVER] Dopo la strcpy di lockFile ho path:%s e token:%s\n", path, token);

            int res;

            if((res = bloccaFile(path, cfd)) == -1){
                sprintf(response, "-1,%d", ENOENT);
            }
            else if(res == -2){
                sprintf(response, "-2,%d", EPERM);
            }
            else{
                sprintf(response, "0");
            }

            if(DEBUGSERVER) printf("[SERVER] Il responso di lockFile è %s\n", response);
            SYSCALL_WRITE(writen(cfd, response, LEN), "lockFile: socket write response");
        }

        else if(strcmp(token, "unlockFile") == 0){
            //estraggo gli argomenti

            if(DEBUGSERVER) printf("[SERVER] Entro in unlockFile\n");

            //SYSCALL_WRITE(writen(cfd, response, LEN), "lockFile: socket write response");
            token = strtok(NULL, ",");
            char path[PATH_MAX];
            strcpy(path, token);

            if(DEBUGSERVER) printf("[SERVER] Dopo la strcpy di unlockFile ho path:%s e token:%s\n", path, token);

            int res;

            if((res = sbloccaFile(path, cfd)) == -1){
                sprintf(response, "-1,%d", ENOENT);
            }
            else if(res == -2){
                sprintf(response, "-2,%d", EPERM);
            }
            else{
                sprintf(response, "0");
            }

            if(DEBUGSERVER) printf("[SERVER] Il responso di unlockFile è %s\n", response);
            SYSCALL_WRITE(writen(cfd, response, LEN), "unlockFile: socket write response");
        }

        else{
            sprintf(response, "-1,%d", ENOSYS);
            SYSCALL_WRITE(writen(cfd, response, LEN), "THREAD : socket write");
        }
        SYSCALL_EXIT("writen", notused, writen(pfd,&cfd,sizeof(cfd)),"THREAD : pipe write", "");
        int fine=0;
        SYSCALL_EXIT("writen", notused,writen(pfd,&fine,sizeof(fine)),"THREAD : pipe write", "");
    }

    else{
        SYSCALL_EXIT("writen", notused, writen(pfd, &cfd, sizeof(cfd)), "thread: pipe writen", "");
        int fine = -1;
        SYSCALL_EXIT("writen", notused, writen(pfd, &fine, sizeof(fine)), "thread: pipe writen", "");
    }
}


//--------UTILITY PER GESTIONE SERVER----------//

//INSERIMENTO IN TESTA
void insertNode (node ** list, int data) {
    //printf("Inserisco in coda\n");
    //fflush(stdout);
    int err;
    //PRENDO LOCK
    SYSCALL_PTHREAD(err,pthread_mutex_lock(&lock_coda),"Lock coda");
    node * new = malloc (sizeof(node));
    if (new==NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    new->data = data;
    new->next = *list;

    //INSERISCI IN TESTA
    *list = new;
    //INVIO SIGNAL
    SYSCALL_PTHREAD(err,pthread_cond_signal(&not_empty),"Signal coda");
    //RILASCIO LOCK
    pthread_mutex_unlock(&lock_coda);

}

//RIMOZIONE IN CODA
int removeNode (node ** list) {
    int err;
    //PRENDO LOCK
    SYSCALL_PTHREAD(err,pthread_mutex_lock(&lock_coda),"Lock coda");
    //ASPETTO CONDIZIONE VERIFICATA
    while (coda==NULL) {
        pthread_cond_wait(&not_empty,&lock_coda);
        //printf("Consumatore Svegliato\n");
        //fflush(stdout);
    }
    int data;
    node * curr = *list;
    node * prev = NULL;
    while (curr->next != NULL) {
        prev = curr;
        curr = curr->next;
    }
    data = curr->data;

    //LO RIMUOVO
    if (prev == NULL) {
        free(curr);
        *list = NULL;
    }else{
        prev->next = NULL;
        free(curr);
    }
    //RILASCIO LOCK
    pthread_mutex_unlock(&lock_coda);
    return data;
}


//SIGINT,SIGQUIT --> TERMINA SUBITO (GENERA STATISTICHE)
//SIGHUP --> NON ACCETTA NUOVI CLIENT, ASPETTA CHE I CLIENT COLLEGATI CHIUDANO CONNESSIONE
static void gestore_term (int signum) {
    if (signum==SIGINT || signum==SIGQUIT) {
        term = 1;
    } else if (signum==SIGHUP) {
        //gestisci terminazione soft
        term = 2;
    }
}

//Funzione di utility per la gestione della select
//ritorno l'indice massimo tra i descrittori attivi
int updatemax(fd_set set, int fdmax) {
    for(int i=(fdmax-1);i>=0;--i)
    if (FD_ISSET(i, &set)) return i;
    assert(1==0);
    return -1;
}


//------------- FUNZIONI PER GESTIONE LA CACHE FILE -------------//

//INSERIMENTO IN TESTA, RITORNO 0 SE HO SUCCESSO, -1 SE FALLITA APERTURA FILE, -2 SE FALLITA CREAZIONE FILE
int aggiungiFile(char* path, int flag, int cfd, char* dirname){
    if(DEBUGSERVER) printf("[SERVER] Entro su aggiungiFile con path %s e con flag %d\n", path, flag);
    int result = 0;
    int err;
    int trovato = 0;
    SYSCALL_PTHREAD(err, pthread_mutex_lock(&lock_cache), "Lock Cache");

    file** lis = &cache;

    file* curr = cache;

    while(curr != NULL && trovato == 0){
        //if(DEBUGSERVER) printf("[SERVER] Nel while ho il path %s e scorro con %s\n", path, curr->path);
        if(strcmp(path, curr->path) == 0){
            trovato = 1;
            if(curr->lock_file != cfd && curr->lock_file != 0){
                result = -3;
                pthread_mutex_unlock(&lock_cache);
                return result;
            }
        }
        else{
            curr = curr->next;
        }
    }

    //caso in cui non viene trovato
    //creo il file e lo inserisco in testa
    if(flag == 1 && trovato == 0){
        if(num_files+1 > configuration->num_files){     //applico l'algoritmo di rimozione di file dalla cache
            file* temp = *lis;

            if(temp == 0){
                result = -1;
            }
            else if(temp->next == NULL){
                *lis = NULL;
                if(DEBUGSERVER) printf("[SERVER] Elimino il file %s\n", temp->path);

                if(DEBUGSERVER) printf("[SERVER] La lunghezza della directory è %lu\n", strlen(dirname));
                if(strcmp(dirname, "(null)") != 0){
                    if(DEBUGSERVER) printf("[SERVER] Rimuovo il file %s\n", temp->path);
                    if(DEBUGSERVER) printf("[SERVER] La dirname è %s\n", dirname);
                    char path[PATH_MAX];
                    memset(path, 0, PATH_MAX);

                    char* file_name = basename(temp->path);
                    sprintf(path, "%s/%s", dirname, file_name);

                    if(DEBUGSERVER) printf("Il file_name è %s\n", file_name);

                    mkdir_p(dirname);

                    FILE* of = fopen(path, "w");
                    if (of == NULL) {
                        printf("Errore salvataggio file\n");
                    }
                    else {
                        fprintf(of,"%s",temp->data);
                        fclose(of);
                        if(DEBUGSERVER) printf("[SERVER] Scritto nel file il buffer passato\n");
                    }
                }
                else{
                    if(DEBUGSERVER) printf("[SERVER] Directory non specificata\n");
                }

                free(temp);
                num_files --;
                replace ++;
                if(DEBUGSERVER) printf("[SERVER] Eseguito il rimpiazzo per la %d volta\n", replace);
            }
            else{
                file* prec = NULL;
                while(temp->next != NULL){
                    prec = temp;
                    temp = temp->next;
                }
                prec->next = NULL;

                if(DEBUGSERVER) printf("[SERVER] Elimino il file %s\n", temp->path);

                if(strcmp(dirname, "(null)") != 0){
                    if(DEBUGSERVER) printf("[SERVER] Rimuovo il file %s\n", temp->path);
                    if(DEBUGSERVER) printf("[SERVER] La dirname è %s\n", dirname);
                    char path[PATH_MAX];
                    memset(path, 0, PATH_MAX);

                    char* file_name = basename(temp->path);
                    sprintf(path, "%s/%s", dirname, file_name);

                    if(DEBUGSERVER) printf("Il file_name è %s\n", file_name);

                    mkdir_p(dirname);

                    FILE* of = fopen(path, "w");
                    if (of == NULL) {
                        printf("Errore salvataggio file\n");
                    }
                    else {
                        fprintf(of,"%s",temp->data);
                        fclose(of);
                        if(DEBUGSERVER) printf("[SERVER] Scritto nel file il buffer passato\n");
                    }
                }
                else{
                  if(DEBUGSERVER) printf("[SERVER] Directory non specificata\n");
                }

                free(temp->data);
                free(temp);
                freeList(&(temp->client_open));
                num_files --;
                replace ++;
                if(DEBUGSERVER) printf("[SERVER] Eseguito il rimpiazzo per la %d volta\n", replace);
            }
        }

        if(result == 0){
            if(DEBUGSERVER) printf("[SERVER] Creo il file su aggiungiFile\n");
            fflush(stdout);

            file* curr;
            CHECKNULL(curr, malloc(sizeof(file)), "malloc curr");

            strcpy(curr->path, path);
            curr->data = NULL;
            curr->client_write = cfd;
            curr->client_open = NULL;
            curr->lock_file = 0;

            if(DEBUGSERVER) printf("[SERVER] Inserisco il file %s\n", curr->path);

            node* new;
            CHECKNULL(new, malloc(sizeof(node)), "malloc new");
            new->data = cfd;
            new->next = curr->client_open;

            curr->client_open = new;
            curr->next = *lis;
            *lis = curr;
            num_files ++;
            if (num_files > top_files){
                top_files = num_files;
            }

            if(DEBUGSERVER) printf("[SERVER] File creato con successo\n");

        }
    }

    else if(flag == 0 && trovato == 1){

        if(DEBUGSERVER) printf("[SERVER] Entro qui perche flag è %d e trovato è %d\n", flag, trovato);
        //apro il file per cfd, controllo se nella lista non è gia presente cfd e nel caso lo inserisco
        if(fileOpen(curr->client_open, cfd) == 0){
            node* new;
            CHECKNULL(new, malloc(sizeof(node)), "malloc new");

            new->data = cfd;
            new->next = curr->client_open;
            curr->client_open = new;
        }
    }

    else{
        if(flag == 0 && trovato == 0){
            if(DEBUGSERVER) printf("[SERVER] Entro nel primo caso perche trovato è %d\n", trovato);
            result = -1;                    //il file non esiste e non puo essere aperto
        }
        if(flag == 1 && trovato == 1){
            if(DEBUGSERVER) printf("[SERVER] Entro nel secondo caso\n");
            result = -2;                    //il file esiste e non puo essere creato di nuovo
        }
    }

    pthread_mutex_unlock(&lock_cache);

    if(DEBUGSERVER) printf("[SERVER] Il risultato è %d\n", result);
    return result;
}

int rimuoviCliente(char* path, int cfd){

    if(DEBUGSERVER) printf("[SERVER] Entro in rimuoviCliente\n");
    int result = 0;
    int err;
    int trovato = 0;
    int rimosso = 0;

    SYSCALL_PTHREAD(err, pthread_mutex_lock(&lock_cache), "Lock Cache");

    file* curr = cache;

    if(DEBUGSERVER) printClient(curr->client_open);

    while(curr != NULL && trovato == 0){
        if(strcmp(path, curr->path) == 0){
            trovato = 1;
        }
        else{
            curr = curr->next;
        }
    }

    if(DEBUGSERVER) printf("[SERVER] Trovato è uguale ad %d\n", trovato);


    if(trovato == 1){
        node* temp = curr->client_open;
        node* prec = NULL;

        if(DEBUGSERVER) printf("[SERVER] Il cfd è %d, mentre di temp è %d\n", cfd, temp->data);

        while(temp != NULL){
            if(temp->data == cfd){
                rimosso = 1;
                if(prec == NULL){
                    curr->client_open = temp->next;
                }
                else{
                    prec->next = temp->next;
                }
                free(temp);
                curr->client_write = -1;
                break;
            }
            prec = temp;
            temp = temp->next;
        }

        if(DEBUGSERVER) printf("[SERVER] Trovato è %d, esce anche dalla eliminazione\n", trovato);
    }

    if(trovato == 0){
        result = -1;                //il file non esiste
    }
    else if(rimosso == 0){
        result = -1;                //il file esiste ma non è stato rimosso
    }

    pthread_mutex_unlock(&lock_cache);

    if(DEBUGSERVER) printf("[SERVER] Esco da rimuoviCliente con risultato %d\n", result);
    return result;
}

int rimuoviFile(char* path, int cfd){
    int result = 0;
    int err;
    int rimosso = 0;

    SYSCALL_PTHREAD(err, pthread_mutex_lock(&lock_cache), "Lock Cache");

    file** lis = &cache;

    file* curr = *lis;
    file* prec = NULL;

    while (curr != NULL) {
        if(strcmp(curr->path, path) == 0){
            if(curr->lock_file == 0 || curr->lock_file == cfd){
                rimosso = 1;
                if (prec == NULL) {
                    *lis = curr->next;
                }
                else{
                    prec->next = curr->next;
                }
                dim_byte = dim_byte - (int) strlen(curr->data);
                num_files --;
                free(curr->data);
                freeList(&(curr->client_open));
                free(curr);
                break;
            }
            else{
                result = -2;
                pthread_mutex_unlock(&lock_cache);
                return result;
            }
        }
        else{
            prec = curr;
            curr = curr->next;
        }
    }

    if(rimosso == 0){
        result = -1;
    }

    pthread_mutex_unlock(&lock_cache);

    return result;
}

//RITONA 0 SE SI HA SUCCESSO, -1 SE IL FILE NON ESISTE, -2 SE L OPERAIONE NON È PERMESSA, -3 SE FILE È TROPPO GRANDE
int inserisciDati(char* path, char* data, int size, int cfd, char* dirname){
    if(DEBUGSERVER) printf("[SERVER] ******************************************************************\n");
    if(DEBUGSERVER) printf("[SERVER] Entra in inserisci dati\n");
    int result = 0;
    int err;

    SYSCALL_PTHREAD(err, pthread_mutex_lock(&lock_cache), "Lock Cache");

    int trovato = 0;
    int scritto = 0;
    file* curr = cache;
    while(curr != NULL){
        if(strcmp(path, curr->path) == 0){
            if(curr->lock_file == 0 || curr->lock_file == cfd){
                trovato = 1;
                //controllo se sia stata fatta la OPEN_CREATE sul file
                if(curr->client_write == cfd){
                    //controllo i limiti di memoria
                    if(size > configuration->sizeBuff){
                        result = -3;
                        break;
                    }
                    else if(dim_byte + size > configuration->sizeBuff){
                        if(resizeCache(size, dirname) == -1){
                            result = -3;
                            break;
                        }
                        else{
                            replace ++;
                            //if(DEBUGSERVER) printf("[SERVER] Eseguito il rimpiazzo per la %d volta\n", replace);
                        }
                        if(DEBUGSERVER) printf("[SERVER] ********* Chiama resizeCache con size %d e directory %s *********\n", size, dirname);
                    }

                    if(result == 0){
                        CHECKNULL(curr->data, malloc(size*sizeof(char)), "malloc curr->data");
                        strcpy(curr->data, data);
                        curr->client_write = -1;
                        scritto = 1;
                        dim_byte = dim_byte + size;
                        if(dim_byte > top_dim){
                            top_dim = dim_byte;
                        }
                    }
                }
                break;
            }
            else{
                result = -4;
                pthread_mutex_unlock(&lock_cache);
                return result;
            }
        }
        curr = curr->next;
    }

    if(trovato == 0 && result == 0){
        result = -1;
    }
    else if (scritto == 0 && result == 0){
        result = -2;
    }
    pthread_mutex_unlock(&lock_cache);
    return result;
}

int appendDati(char* path, char* data, int size, int cfd, char* dirname){
    int result = 0;
    int err;

    SYSCALL_PTHREAD(err, pthread_mutex_lock(&lock_cache), "Lock Cache");

    int trovato = 0;
    int scritto = 0;
    file* curr = cache;
    while(curr != NULL){
        if(strcmp(path, curr->path) == 0){
            trovato = 1;
            if(curr->lock_file == 0 || curr->lock_file == cfd){
                //controllo di aver gia aperto i file
                if(fileOpen(curr->client_open, cfd) == 1){
                    char* temp = realloc(curr->data, (strlen(curr->data)+size+1) *sizeof(char));
                    if(temp != NULL){
                        //controllo i limiti della memoria
                        if(dim_byte + size > configuration->sizeBuff){
                            if(resizeCache(size, dirname) == -1){
                                result = -3;
                                break;
                            }
                            else{
                                replace++;
                            }
                        }
                        if(result == 0){
                            strcat(temp, data);
                            scritto = 1;
                            curr->data = temp;
                            if(curr->client_write == cfd){
                                curr->client_write = -1;
                            }
                            dim_byte = dim_byte+size;
                            if(dim_byte>top_dim){
                                top_dim = dim_byte;
                            }
                        }
                    }
                }
                else{
                    //caso in cui il file è stato lockato da un altro client
                    result = -4;
                }
            }
            break;
        }
        curr = curr->next;
    }

    if(trovato == 0){
        result = -1;
    }
    else if (scritto == 0){
        result = -2;
    }
    pthread_mutex_unlock(&lock_cache);
    return result;
}

char* prendiFile (char* path, int cfd){
    if(DEBUGSERVER) printf("[SERVER] Entro in prendifile con path %s\n", path);
    int err;
    char* response = NULL;

    SYSCALL_PTHREAD(err, pthread_mutex_lock(&lock_cache), "Lock Cache");

    //int trovato = 0;
    file* curr = cache;

    while(curr != NULL){
        if(strcmp(curr->path, path) == 0){
            //trovato = 1;
            if(curr->lock_file == 0 || curr->lock_file == cfd){
                if(fileOpen(curr->client_open, cfd) == 1){
                    response = curr->data;
                }
            }
            else{
                sprintf(response, "-1");
            }
            break;
        }
        else{
            curr = curr->next;
        }
    }

    pthread_mutex_unlock(&lock_cache);
    return response;
}

//RITORNA -1 NEL CASO IN CUI IL FILE È GIA IN LOCK, RITORNA -2 NEL CASO IN CUI NON TROVI IL FILE, RITORNA 0 SE HA SUCCESSO
//TODO: aggiungere il caso in cui lo stesso che ha fatto la lock riprova a fare la lock
int bloccaFile(char* path, int cfd){
    if(DEBUGSERVER) printf("[SERVER] Entra in bloccaFile\n");

    int result = 0;
    int err;

    SYSCALL_PTHREAD(err, pthread_mutex_lock(&lock_file), "Lock File");

    int trovato = 0;
    file* curr = cache;

    while(curr != NULL){
        if(strcmp(curr->path, path) == 0){
            trovato = 1;
            if(DEBUGSERVER) printf("[SERVER] File %s presente nella cache\n", path);
            break;
        }
        else{
            curr = curr->next;
        }
    }

    if(trovato == 1){
        if(curr->lock_file == 0){
            curr->lock_file = cfd;
            if(DEBUGSERVER) printf("[SERVER] File %s ha avuto la lock da %d (%d)\n", curr->path, curr->lock_file, cfd);
        }
        else{
            result = -1;
        }
    }
    else{
        result = -2;
    }
    pthread_mutex_unlock(&lock_file);
    if(DEBUGSERVER) printf("[SERVER] Il risultato di bloccaFile è %d\n", result);
    return result;
}

int sbloccaFile(char* path, int cfd){
    if(DEBUGSERVER) printf("[SERVER] Entra in sbloccaFile\n");

    int result = 0;
    int err;

    SYSCALL_PTHREAD(err, pthread_mutex_lock(&lock_file), "Lock File");

    int trovato = 0;
    file* curr = cache;

    while(curr != NULL){
        if(strcmp(curr->path, path) == 0){
            trovato = 1;
            if(DEBUGSERVER) printf("[SERVER] File %s presente nella cache\n", path);
            break;
        }
        else{
            curr = curr->next;
        }
    }

    if(trovato == 1){
        if(DEBUGSERVER) printf("[SERVER] File %s ha avuto la unlock da %d (%d)\n", curr->path, curr->lock_file, cfd);
        if(curr->lock_file == cfd){
            curr->lock_file = 0;
            if(DEBUGSERVER) printf("[SERVER] File %s ha avuto la unlock da %d (%d)\n", curr->path, curr->lock_file, cfd);
        }
        else{
            result = -1;
        }
    }
    else{
        result = -2;
    }
    pthread_mutex_unlock(&lock_file);
    if(DEBUGSERVER) printf("[SERVER] Il risultato di sbloccaFile è %d\n", result);
    return result;
}
//-------------------- FUNZIONI DI UTILITY PER LA CACHE --------------------//
void printFile () {
    printf ("Lista File : \n");
    fflush(stdout);
    file * curr = cache;
    while (curr != NULL) {
        printf("%s ",curr->path);
        if (curr->data!=NULL) {
            printf("size = %ld, locked = %d\n", strlen(curr->data), curr->lock_file);
        } else {
            printf("size = 0\n");
        }

        curr = curr->next;
    }
}

void freeList(node ** head) {
    node* temp;
    node* curr = *head;
    while (curr != NULL) {
        temp = curr;
        curr = curr->next;
        free(temp);
    }
    *head = NULL;
}

int fileOpen(node* list, int cfd) {
    node* curr = list;
    while (curr != NULL) {
        if (curr->data == cfd){
            return 1;
        }
    }
    return 0;
}

//funzione che elimina l'ultimo file della lista finche non c è abbastanza spazio per il nuovo file
int resizeCache(int dim, char* dirname){
    if(DEBUGSERVER) printf("[SERVER] ******************************************************************\n");
    if(DEBUGSERVER) printf("[SERVER] Chiamato resizeCache con dimensione %d e directory passata %s\n", dim, dirname);
    file** lis = &cache;
    while(dim_byte + dim > configuration->sizeBuff){
        //elimino in coda
        file* temp = *lis;
        if(temp == NULL){
            break;
        }
        else if(temp->next == NULL){
            *lis = NULL;
        }
        else{
            file* prec = NULL;
            while(temp->next != NULL){
                prec = temp;
                temp = temp->next;
            }
            prec->next = NULL;
        }

        if(DEBUGSERVER) printf("[SERVER] Rimuovo il file %s\n", temp->path);
        char path[PATH_MAX];
        memset(path, 0, PATH_MAX);

        char* file_name = basename(temp->path);
        sprintf(path, "%s/%s", dirname, file_name);

        if(DEBUGSERVER) printf("Il file_name è %s\n", file_name);

        mkdir_p(dirname);

        FILE* of = fopen(path, "w");
        if (of == NULL) {
            printf("Errore salvataggio file\n");
        }
        else {
            fprintf(of,"%s",temp->data);
            fclose(of);
            if(DEBUGSERVER) printf("[SERVER] Scritto nel file il buffer passato\n");
        }


        //opzioneD(temp, dirname);
        dim_byte = dim_byte - (int)strlen(temp->data);
        num_files--;
        free(temp->data);
        freeList(&(temp->client_open));
        free(temp);
    }

    if(*lis == NULL && (dim_byte + dim > configuration->sizeBuff)){
        return -1;
    }
    if(DEBUGSERVER) printf("[SERVER] ******************************************************************\n");
    return 0;
}

void printClient (node * list) {
    node * curr = list;
    printf("APERTO DA : ");
    fflush(stdout);
    while (curr!=NULL) {
        printf("%d ",curr->data);
        fflush(stdout);
        curr = curr->next;
    }
    printf("\n");
}

int mkdir_p(const char *path) {
    if(DEBUGSERVER) printf("[SERVER] ******************************************************************\n");
    if(DEBUGSERVER) printf("[SERVER] Entra in mkdir_p con path %s\n", path);
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
    if(DEBUGSERVER) printf("[SERVER] ******************************************************************\n");
    return 0;
}
