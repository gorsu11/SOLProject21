#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "util.h"

int notused;    //TODO: variabile da spostare successivamente in qualche libreria
void* Workers(void* argument);

int main(int argc, char* argv[]){
  if(argc != 2){
    fprintf(stderr, "Error data in %s\n", argv[0]);
    return -1;
  }

  //CHIAMATA DELLE FUNZIONI PER PARSARE IL FILE DATO IN INPUT
  //TODO: creare libreria per il parsing

  //CREO PIPE E THREADS PER IMPLEMENTARE IL MASTER-WORKER
  //-------------------- CREAZIONE PIPE ---------------------//
  int comunication[2];    //per comunicare tra master e worker
  SYSCALL_EXIT("pipe", notused, pipe(comunication), "pipe", "");

  //-------------------- CREAZIONE THREAD WORKERS --------------------//

  int temp = 5;       //numero provvisorio da sostituire quando parserò il file
  pthread_t *master;
  CHECKNULL(master, malloc(temp * sizeof(pthread_t)), "malloc pthread_t");

  if(DEBUG) printf("Creazione dei %d thread Worker\n", temp);

  int err;
  for(int i=0; i<temp; i++){
    SYSCALL_PTHREAD(err, pthread_create(&master[i], NULL, Workers, (void*) (&comunication[1])), "pthread_create pool");
  }

  if(DEBUG) printf("Creazione andata a buon fine\n");
  //-------------------------------------------------------------//

  //-------------------- CREAZIONE SOCKET --------------------//
}
