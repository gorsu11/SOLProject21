#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

//-------------- CHIAMATE ALLE LIBRERIE ---------------//
#include "util.h"
#include "parsingFile.c"
//-------------------------------------------------------------//

int notused;    //TODO: variabile da spostare successivamente in qualche libreria


//-------------- DICHIARAZIONE DELLE FUNZIONI ---------------//
void* Workers(void* argument);
//-------------------------------------------------------------//


//-------------- STRUTTURE PER SALVARE I DATI ---------------//
config* configuration;
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

}
