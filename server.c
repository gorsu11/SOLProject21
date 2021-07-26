#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <signal.h>

//-------------- CHIAMATE ALLE LIBRERIE ---------------//
#include "util.h"
#include "parsingFile.c"
#include "conn.h"
//-------------------------------------------------------------//

int notused;    //TODO: variabile da spostare successivamente in qualche libreria
//#define LEN 1000

//-------------- STRUTTURE PER SALVARE I DATI E DICHIARAZIONI DI VARIABILI GLOBALI ---------------//
config* configuration;

volatile sig_atomic_t term;
//-------------------------------------------------------------//


//-------------- DICHIARAZIONE DELLE FUNZIONI ---------------//
void* Workers(void* argument);
static void signal_handler(int num_signal);
int max_index(fd_set set, int fdmax);

void cleanup() {
    unlink(configuration->socket_name);
}
//-------------------------------------------------------------//




int main(int argc, char* argv[]){
    if(argc != 2){
        fprintf(stderr, "Error data in %s\n", argv[0]);
        return -1;
    }

    //CHIAMATA DELLE FUNZIONI PER PARSARE IL FILE DATO IN INPUT
    //-------------------- PARSING FILE --------------------//
    if (argc != 2) {
        fprintf(stderr, "Error data in %s\n", argv[0]);
        return -1;
    }

    //se qualcosa non va a buon fine prende i valori di default
    //TODO: potrei anche utilizzare le define per vedere se è NULL, da rivedere
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
    s.sa_handler = signal_handler;

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


    //CREO PIPE E THREADS PER IMPLEMENTARE IL MASTER-WORKER
    //-------------------- CREAZIONE PIPE ---------------------//
    int comunication[2];    //per comunicare tra master e worker
    SYSCALL_EXIT("pipe", notused, pipe(comunication), "pipe", "");

    //-------------------- CREAZIONE THREAD WORKERS --------------------//

    pthread_t *master;
    CHECKNULL(master, malloc(configuration->num_thread * sizeof(pthread_t)), "malloc pthread_t");

    if(DEBUGSERVER) printf("Creazione dei %d thread Worker\n", configuration->num_thread);

    int err;
    for(int i=0; i<configuration->num_thread; i++){
        SYSCALL_PTHREAD(err, pthread_create(&master[i], NULL, Workers, (void*) (&comunication[1])), "pthread_create pool");
    }

    if(DEBUGSERVER) printf("Creazione andata a buon fine\n");
    //-------------------------------------------------------------//

    //-------------------- CREAZIONE SOCKET --------------------//

    // se qualcosa va storto ....
    atexit(cleanup);

    // cancello il socket file se esiste
    cleanup();


    int listenfd, num_fd = 0;
    fd_set set, rdset;


    //DICHIARAZIONE DI VARIABILI PER TERMINARE
    int num_client = 0;             //per la SIGHUP dove aspetta di terminare tutti i clienti
    int termina_soft = 0;

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

    printf("Listen for clients ...\n");

    while(1){
        rdset = set;

        if(select(num_fd+1, &rdset, NULL, NULL, NULL) == -1){
            if(term == 1){
                break;
            }
            else if(term == 2){
                if(num_client == 0){
                    break;
                }
                else{
                    printf("Verrà eseguita una chiusura soft\n");
                    FD_CLR(listenfd, &set);

                    if(listenfd == num_fd){
                        num_fd = max_index(set,num_fd);
                    }
                    close(listenfd);
                    rdset = set;
                    SYSCALL_EXIT("select", notused, select(num_fd+1, &rdset, NULL, NULL, NULL),"Errore select", "");
                }
            }
            else{
                perror("select");
                break;
            }
        }

        int check;

        for(int fd=0; fd<num_fd; fd++){
            if(FD_ISSET(fd, &rdset)){
                if(fd == listenfd){
                    //connesso al socket e pronto per accettare richieste
                    if((check = accept(listenfd, NULL, 0)) == -1){
                        if(term == 1){
                            break;
                        }
                        else if(term == 2){
                            if(num_client == 0){
                                break;
                            }
                            else{
                                perror("Errore client accept");
                            }
                        }
                    }
                    FD_SET(check, &set);
                    if(check > num_fd){
                        num_fd = check;
                    }
                    num_client++;
                    printf ("Connection accepted from client!\n");

                    char buffer[LEN];
                    memset(buffer, 0, LEN);
                    strcpy(buffer, "BENVENUTI NEL SERVER DI ORSUCCI GIANLUCA");

                    if((writen(check, buffer, LEN)) == -1){
                        perror("Errore write welcome message");
                        FD_CLR(check, &set);
                        if (check == num_fd){
                            num_fd = max_index(set,num_fd);
                        }
                        close(check);
                        num_client--;
                    }

                }

                else if(fd == comunication[0]){
                    //client da rimettere nel set e la pipe è pronta in lettura
                    int check1, len, flag;
                    if ((len = (int) read(comunication[0], &check1, sizeof(check1))) > 0){
                        SYSCALL_EXIT("readn", notused, readn(comunication[0], &flag, sizeof(flag)),"Master : read pipe", "");
                        if(flag == -1){
                            //il client è terminato, lo rimuovo quindi dal set
                            printf("Closing connection with client...\n");
                            if (check1 == num_fd){
                                num_fd = max_index(set,num_fd);
                            }
                            close(check1);
                            num_client--;
                            if (term==2 && num_client==0) {
                                printf("Chiusura soft\n");
                                termina_soft = 1;
                            }
                        }
                        else{
                            FD_SET(check1, &set);
                            if (check1 > num_fd){
                                num_fd = check1;
                            }
                        }
                    }
                    else if (len == -1){
                        perror("Master : read pipe");
                        exit(EXIT_FAILURE);
                    }
                }

                else{
                    //socket client pronto per la read, inserisco il socket client in coda
                    //TODO: inserire client in coda, del tipo inserisci(Coda_client, &fd)
                }
            }
        }

        if(termina_soft == 1){
            break;
        }

    }

    printf("Closing server ...\n");


    SYSCALL_EXIT("close", notused, close(listenfd), "close", "");
    freeConfig(configuration);

    if(DEBUGSERVER) printf("Connessione chiusa");
    return 0;

}

//SIGINT E SIGQUIT TERMINANO SUBITO (GENERA STATISTICHE)
//SIGHUP INVECE NON ACCETTA NUOVI CLIENT, ASPETTA CHE I CLIENT COLLEGATI CHIUDANO CONNESSIONE
static void signal_handler(int num_signal){
    if(num_signal == SIGINT || num_signal == SIGQUIT){
        term = 1;
    }
    else if (num_signal == SIGHUP){
        term = 2;
    }
}

//Funzione di utility per la gestione della select
//ritorno l'indice massimo tra i descrittori attivi
int max_index(fd_set set, int fdmax){
    for(int i=(fdmax-1); i>=0; --i){
        if (FD_ISSET(i, &set)){
            return i;
        }
    }
    return -1;
}
