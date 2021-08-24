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

#include "util.h"
#include "conn.h"
#include "parsingFile.c"
//#include "handFile.c"


#define DIM_MSG 1000

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
    int client_write;               //FILE DESCRIPTOR DEL CLIENT CHE HA ESEGUITO COME ULTIMA OPERAZIONE UNA openFile con flag O_CREATE
    struct file * next;
} file;

pthread_mutex_t lock_cache = PTHREAD_MUTEX_INITIALIZER;
int dim_byte; //DIMENSIONE CACHE IN BYTE
int num_files; //NUMERO FILE IN CACHE

//LIMITI DI MEMORIA
int max_dim;
int max_files;

//x statistiche finali
int top_files=0;
int top_dim=0;
int replace = 0;
//-----------------------------------------------//


volatile sig_atomic_t term = 0; //FLAG SETTATO DAL GESTORE DEI SEGNALI DI TERMINAZIONE
static void gestore_term (int signum);

//STRUTTURA DATI PER SALVARE I FILE
file * cache = NULL;

config* configuration;

//CODA DI COMUNICAZIONE MANAGER --> WORKERS / RISORSA CONDIVISA / CODA FIFO
node * coda = NULL;
pthread_mutex_t lock_coda = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

void insertNode (node ** list, int data);
int removeNode (node ** list);
int updatemax(fd_set set, int fdmax);

//-------------------- FUNZIONI SOLO DICHIARATE --------------------//
int aggiungiFile(char* path, int flag, int cfd);
int rimuoviCliente(char* path, int cfd);
int rimuoviFile(char* path, int cfd);
int inserisciDati(char* path, char* data, int size, int cfd);
int appendDati(char* path, char* data, int size, int cfd);
char* prendiFile (char* path, int cfd);
void printFile (void);
void freeList(node** head);
int fileOpen(node* list, int cfd);
int resizeCache(int dim);

void execute (char * request, int cfd,int pfd);
//------------------------------------------------------------//

void* Workers(void *argument);

void cleanup() {
    unlink(configuration->socket_name);
}

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

    if(DEBUG) stampa(configuration);
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

    if(DEBUG) printf("Creazione dei %d thread Worker\n", configuration->num_thread);

    int err;
    for(int i=0; i<configuration->num_thread; i++){
        SYSCALL_PTHREAD(err, pthread_create(&master[i], NULL, Workers, (void*) (&comunication[1])), "pthread_create pool");
    }

    if(DEBUG) printf("Creazione andata a buon fine\n");

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
                    char buf [DIM_MSG];
                    memset(buf,0,DIM_MSG);
                    strcpy(buf,"BENVENUTI NEL SERVER!");
                    if (writen(cfd,buf,DIM_MSG)==-1) {
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
                        SYSCALL_EXIT("readn", notused, readn(comunication[0],&flag,sizeof(flag)),"Master : read pipe", "");
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
                    //printf("Master : Client pronto in read\n");
                    //QUINDI INSERISCO FD SOCKET CLIENT NELLA CODA

                    insertNode(&coda,fd);
                    FD_CLR(fd,&set);
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
        SYSCALL_PTHREAD(e,pthread_join(master[i],NULL),"Errore join thread");
    }

    printf("---------STATISTICHE SERVER----------\n");
    printf("Numero di file massimo = %d\n",top_files);
    printf("Dimensione massima = %f Mbytes\n",(top_dim/pow(10,6)));
    printf("Chiamate algoritmo di rimpiazzamento cache = %d\n",replace);
    printFile();
    printf("-------------------------------------\n");

    //TODO: da modificare
    if(close(listenfd) == -1){
        perror("close");
        return -1;
    }

    free(master);
    freeConfig(configuration);

    printf("connection done\n");


}

void* Workers(void *argument){
    if(DEBUG) printf("Entra\n");

    int pfd = *((int*) argument);
    int cfd;

    while(1){
        char request[DIM_MSG];
        memset(request, 0, DIM_MSG);

        //PRELEVA UN CLIENTE DALLA CODA
        cfd = removeNode(&coda);
        if(cfd == -1){
            break;
        }

        //SERVO IL CLIENT
        int len;
        int fine; //FLAG COMUNICATO AL MASTER PER SAPERE QUANDO TERMINA IL CLIENT

        if((len = readn(cfd, request, DIM_MSG)) == 0){
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
            execute(request, cfd, pfd);
        }
    }
    printf("Closing worker\n");
    fflush(stdout);
    return 0;
}

void execute (char * request, int cfd,int pfd){
    char response[DIM_MSG];
    memset(response, 0, DIM_MSG);

    char* token = NULL;
    int s;

    if(!request){
        token = strtok(request, ",");
    }

    if(token != NULL){
        if(strcmp(token, "openFile") == 0){
            //estraggo i valori
            token = strtok(NULL, ",");
            char path[PATH_MAX];

            //TODO: vedere se usare strncmp o strcmp
            strncmp(path, token, PATH_MAX);
            token = strtok(NULL, ",");
            int flag = atoi(token);

            int result;
            if((result = aggiungiFile(path, flag, cfd)) == -1){
                sprintf(response,"-1, %d",ENOENT);
            }
            else if (result == -2) {
                sprintf(response,"-1, %d",EEXIST);
            }
            else{
                sprintf(response,"0");
            }

            SYSCALL_WRITE(writen(cfd, response, DIM_MSG), "THREAD : socket write");
        }

        else if(strcmp(token, "closeFile") == 0){
            //estraggo il valore
            token = strtok(NULL, ",");
            char path[PATH_MAX];

            //TODO: vedere se usare strncmp o strcmp
            strncpy(path, token, PATH_MAX);

            int result;
            if((result = rimuoviCliente(path, cfd)) == -1){
                sprintf(response,"-1, %d",ENOENT);
            }
            else if (result == -2) {
                sprintf(response,"-1, %d",EPERM);
            }
            else{
                sprintf(response,"0");
            }

            SYSCALL_WRITE(writen(cfd, response, DIM_MSG), "THREAD : socket write");
        }

        else if(strcmp(token, "removeFile") == 0){
            //estraggo il valore
            token = strtok(NULL, ",");
            char path[PATH_MAX];
            strncpy(path, token, PATH_MAX);

            if(rimuoviFile(path, cfd) == -1){
                sprintf(response,"-1,%d",ENOENT);
            }else{
                sprintf(response,"0");
            }
            SYSCALL_WRITE(writen(cfd,response,DIM_MSG),"THREAD : socket write");
        }

        else if(strcmp(token, "writeFile") == 0){
            //estraggo il valore
            token = strtok(NULL, ",");
            char path[PATH_MAX];
            strncpy(path, token, PATH_MAX);

            //invia al client il permesso di inviare file
            sprintf(response, "0");
            SYSCALL_WRITE(writen(cfd, response, DIM_MSG), "writeFile: socket write");

            //ricevo size del file
            char buf1[DIM_MSG];
            memset(buf1, 0, DIM_MSG);
            SYSCALL_READ(s, readn(cfd, buf1, DIM_MSG), "writeFile: socket read");

            if(DEBUGSERVER) printf("Ricevuto dal client la size %s\n", buf1);

            int size = atoi(buf1);

            //invio la conferma al client
            SYSCALL_WRITE(writen(cfd, "0", DIM_MSG), "writeFile: conferma al client");

            //ricevo i file
            char* buf2;
            CHECKNULL(buf2, malloc((size+1)*sizeof(char)), "malloc buf2");

            SYSCALL_READ(s, readn(cfd, buf2, (size+1)), "writeFile: socket read");

            if(DEBUGSERVER) printf("Ricevuto dal client %s\n", buf2);

            //inserisco a questo punto i dati nella cache
            char result[DIM_MSG];
            memset(result, 0, DIM_MSG);

            int res;
            if((res = inserisciDati(path, buf2, size+1, cfd)) == -1){
                sprintf(result, "-1,%d", ENOENT);
            }
            else if(res == -2){
                sprintf(result, "-2,%d", EPERM);
            }
            else if(res == -3){
                sprintf(result, "-3,%d", EFBIG);
            }
            else{
                sprintf(result, "0");
            }
            free(buf2);
            SYSCALL_WRITE(writen(cfd, result, DIM_MSG), "writeFile: socket write result");
        }

        else if(strcmp(token, "appendToFile") == 0){
            //estraggo i valori
            token = strtok(NULL, ",");
            char path[PATH_MAX];
            strncpy(path, token, PATH_MAX);

            //invia al client il permesso di inviare file
            sprintf(response, "0");
            SYSCALL_WRITE(writen(cfd, response, DIM_MSG), "appendToFile: socket write");

            //ricevo size del file
            char buf1[DIM_MSG];
            SYSCALL_READ(s, readn(cfd, buf1, DIM_MSG), "appendToFile: socket read");

            if(DEBUGSERVER) printf("Ricevuto dal client la size %s\n", buf1);

            int size = atoi(buf1);

            //invio la conferma al client
            SYSCALL_WRITE(writen(cfd, "0", DIM_MSG), "appendToFile: conferma al client");

            //ricevo i file
            char* buf2;
            CHECKNULL(buf2, malloc((size+1)*sizeof(char)), "malloc buf2");

            SYSCALL_READ(s, readn(cfd, buf2, (size+1)), "appendToFile: socket read");

            if(DEBUGSERVER) printf("Ricevuto dal client %s\n", buf2);

            char result[DIM_MSG];
            int res;

            if((res = appendDati(path, buf2, size+1, cfd)) == -1){
                sprintf(result, "-1,%d", ENOENT);
            }
            else if(res == -2){
                sprintf(result, "-2,%d", EPERM);
            }
            else if(res == -3){
                sprintf(result, "-3,%d", EFBIG);
            }
            else{
                sprintf(result, "0");
            }

            free(buf2);
            SYSCALL_WRITE(writen(cfd, result, DIM_MSG), "appendToFile: socket write result");
        }

        else if(strcmp(token, "readFile") == 0){
            //estraggo gli argomenti

            token = strtok(NULL, ",");
            char path[PATH_MAX];
            strncpy(path, token, PATH_MAX);

            char* file = prendiFile(path, cfd);

            char buf[DIM_MSG];
            memset(buf, 0, DIM_MSG);

            if(file == NULL){
                //invio un errore al client
                sprintf(buf, "-1,%d", EPERM);
                SYSCALL_WRITE(writen(cfd, buf, DIM_MSG), "readFile: socket write");
            }
            else{
                //invio la size del file
                sprintf(buf, "%ld", strlen(file));
                SYSCALL_WRITE(writen(cfd, buf, DIM_MSG),"readFile: socket write size file");

                char buf1[DIM_MSG];
                memset(buf1, 0, DIM_MSG);
                SYSCALL_READ(s, readn(cfd, buf1, DIM_MSG), "readFile: socket read response");

                if(DEBUGSERVER) printf("Ricevuto dal client %s\n", buf1);
                fflush(stdout);

                if(strcmp(buf1, "0") == 0){
                    //se è andato tutto bene invio i file
                    SYSCALL_WRITE(writen(cfd, file, strlen(file)),"readFile: socket write file");
                }
            }
        }

        else if(strcmp(token, "readNFile") == 0){
            //estraggo gli argomenti
            token = strtok(NULL, ",");
            int num = atoi(token);
            int err;

            if(DEBUGSERVER) printf("File Richiesti: %d\nFile esistenti: %d\n", num, num_files);

            SYSCALL_PTHREAD(err, pthread_mutex_lock(&lock_cache), "Lock Cache");

            //controllo sul valore num
            if((num <= 0) || (num > num_files)){
                num = num_files;
            }
            //invio il numero al client
            char buf[DIM_MSG];
            memset(buf, 0, DIM_MSG);
            sprintf(buf, "%d", num);

            SYSCALL_WRITE(writen(cfd, buf, DIM_MSG), "readNFile: socket write num");

            //ricevo la conferma del client
            char conf[DIM_MSG];
            memset(conf, 0, DIM_MSG);

            SYSCALL_READ(s, readn(cfd, conf, DIM_MSG), "readNFile: socket read response");

            //invio gli N files
            file* curr = cache;
            for(int i=0; i<num; i++){
                //invio il path al client
                char path[DIM_MSG];
                memset(path, 0, DIM_MSG);
                sprintf(path, "%s", curr->path);
                SYSCALL_WRITE(write(cfd, path, DIM_MSG), "readNFile: socket write path");

                //ricevo la conferma
                char conf [DIM_MSG];
                memset(conf, 0, DIM_MSG);
                SYSCALL_READ(s,readn(cfd, conf, DIM_MSG), "readNFile: socket read response");

                //invio size
                char ssize [DIM_MSG];
                memset(ssize, 0, DIM_MSG);
                sprintf(ssize, "%ld", strlen(curr->data));
                SYSCALL_WRITE(writen(cfd, ssize, DIM_MSG), "readNFile: socket write size");

                //ricevo la conferma
                char conf2[DIM_MSG];
                memset(conf2, 0, DIM_MSG);
                SYSCALL_READ(s, readn(cfd, conf2, DIM_MSG), "readNFile: socket read response");

                //invio file
                SYSCALL_WRITE(writen(cfd, curr->data, strlen(curr->data)), "readNFile: socket write file");

                curr = curr->next;
            }

            pthread_mutex_unlock(&lock_cache);
        }

        else if(strcmp(token, "lockFile") == 0){

        }

        else if(strcmp(token, "unlockFile") == 0){

        }

        else{
            sprintf(response, "-1,%d", ENOSYS);
            SYSCALL_WRITE(writen(cfd, response, DIM_MSG), "THREAD : socket write");
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

//INSERIMENTO IN TESTA, RITORNO 0 SE HO SUCCESSO, -1 SE FALLITA APERTURA FILE, -2 SE FALLITA CRTEAZIONE FILE
int aggiungiFile(char* path, int flag, int cfd){
    int result = 0;
    int err;
    int trovato = 0;
    SYSCALL_PTHREAD(err, pthread_mutex_lock(&lock_cache), "Lock Cache");

    file** lis = &cache;

    file* curr = cache;

    while(curr != NULL && trovato == 0){
        if(strcmp(path, curr->path) == 0){
            trovato = 1;
        }
        curr = curr->next;
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
                free(temp);
                num_files --;
                replace ++;
            }
            else{
                file* prec = NULL;
                while(temp->next != NULL){
                    prec = temp;
                    temp = temp->next;
                }
                prec->next = NULL;
                free(temp->data);
                free(temp);
                freeList(&(temp->client_open));
                num_files --;
                replace ++;
            }
        }

        if(result == 0){
            if(DEBUGSERVER) printf("Creo il file su aggiungiFile\n");
            fflush(stdout);

            file* curr;
            CHECKNULL(curr, malloc(sizeof(file)), "malloc curr");

            strcpy(curr->path, path);
            curr->data = NULL;
            curr->client_write = cfd;
            curr->client_open = NULL;

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
        }
    }

    else if(flag == 0 && trovato == 1){
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
            result = -1;                    //il file non esiste e non puo essere aperto
        }
        if(flag == 1 && trovato == 1){
            result = -2;                    //il file esiste e non puo essere creato di nuovo
        }
    }

    pthread_mutex_unlock(&lock_cache);

    return result;
}


int rimuoviCliente(char* path, int cfd){
    int result = 0;
    int err;
    int trovato = 0;
    int rimosso = 0;

    SYSCALL_PTHREAD(err, pthread_mutex_lock(&lock_cache), "Lock Cache");

    file* curr = cache;

    while(curr != NULL && trovato == 0){
        if(strcmp(path, curr->path) == 0){
            trovato = 1;
        }
        curr = curr->next;
    }

    if(trovato == 1){
        node* temp = curr->client_open;
        node* prec = NULL;
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
    }

    if(trovato == 0){
        result = -1;                //il file non esiste
    }
    else if(rimosso == 0){
        result = -1;                //il file esiste ma non è stato rimosso
    }

    pthread_mutex_unlock(&lock_cache);

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
        prec = curr;
        curr = curr->next;
    }

    if(rimosso == 0){
        result = -1;
    }

    pthread_mutex_unlock(&lock_cache);

    return result;
}

//RITONA 0 SE SI HA SUCCESSO, -1 SE IL FILE NON ESISTE, -2 SE L OPERAIONE NON È PERMESSA, -3 SE FILE È TROPPO GRANDE
int inserisciDati(char* path, char* data, int size, int cfd){
    int result = 0;
    int err;

    SYSCALL_PTHREAD(err, pthread_mutex_lock(&lock_cache), "Lock Cache");

    int trovato = 0;
    int scritto = 0;
    file* curr = cache;
    while(curr != NULL){
        if(strcmp(path, curr->path) == 0){
            trovato = 1;
            //controllo se sia stata fatta la OPEN_CREATE sul file
            if(curr->client_write == cfd){
                //controllo i limiti di memoria
                if(size > configuration->sizeBuff){
                    result = -3;
                    break;
                }
                else if(dim_byte + size > configuration->sizeBuff){
                    if(resizeCache(size) == -1){
                        result = -3;
                        break;
                    }
                    else{
                        replace ++;
                    }
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

int appendDati(char* path, char* data, int size, int cfd){
    int result = 0;
    int err;

    SYSCALL_PTHREAD(err, pthread_mutex_lock(&lock_cache), "Lock Cache");

    int trovato = 0;
    int scritto = 0;
    file* curr = cache;
    while(curr != NULL){
        if(strcmp(path, curr->path) == 0){
            trovato = 1;

            //controllo di aver gia aperto i file
            if(fileOpen(curr->client_open, cfd) == 1){
                char* temp = realloc(curr->data, (strlen(curr->data)+size+1) *sizeof(char));
                if(temp != NULL){
                    //controllo i limiti della memoria
                    if(dim_byte + size > configuration->sizeBuff){
                        if(resizeCache(size) == -1){
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
    int err;
    char* response = NULL;

    SYSCALL_PTHREAD(err, pthread_mutex_lock(&lock_cache), "Lock Cache");

    int trovato = 0;
    file* curr = cache;

    while(curr != NULL){
        if(strcmp(curr->path, path) == 0){
            trovato = 1;
            if(fileOpen(curr->client_open, cfd) == 1){
                response = curr->data;
            }
            break;
        }
        curr = curr->next;
    }

    pthread_mutex_unlock(&lock_cache);
    return response;
}

//-------------------- FUNZIONI DI UTILITY PER LA CACHE --------------------//
void printFile () {
    printf ("Lista File : \n");
    fflush(stdout);
    file * curr = cache;
    while (curr != NULL) {
        printf("%s ",curr->path);
        if (curr->data!=NULL) {
            printf("size = %ld\n", strlen(curr->data));
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
int resizeCache(int dim){
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
        dim_byte = dim_byte - (int)strlen(temp->data);
        num_files--;
        free(temp->data);
        freeList(&(temp->client_open));
        free(temp);
    }

    if(*lis == NULL && (dim_byte + dim > configuration->sizeBuff)){
        return -1;
    }

    return 0;
}
