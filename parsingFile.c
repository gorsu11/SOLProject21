#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define SOCKNAME "socket_name"
#define LEN 100

typedef struct config_File{
    int num_thread;                         //numero thread
    size_t size_Buff;                        //spazio di memoria
    unsigned int num_files;                 //numero massimo dei file possibili
    char *socket_name;                      //nome della socket
} config;

config* getConfig(const char* string){
    FILE *file;

    config *configuration; //Variabile che ospita la configurazione
    CHECKNULL(configuration, malloc(sizeof(config)), "malloc configuration");
    char str[200];
    char arg [LEN];
    char val [LEN];

    if((file = fopen(string, "r")) == NULL){
        perror("Error open file");
        return NULL;
    }

    while(fgets(str, 200, file) != NULL){

        if (str[0]!='\n') {

            int nt;
            nt = sscanf (str,"%[^=] = %s",arg,val);
            if (nt!=2) {
                printf("Errore config file : formato non corretto\n");
                printf ("Server avviato con parametri di default\n");
                break;
            }

            if (strcmp(arg,"Numero thread")==0) {

                int n;
                if ((n = (int) isNumberParser(val))==-1 || n<=0) {
                    printf("Errore config file : num_thread richiede un numero>0 come valore\n");
                    printf ("Server avviato con parametri di DEFAULT\n");
                    break;
                }
                else{
                    configuration->num_thread = n;
                }

            } else if (strcmp(arg,"File Massimi")==0) {

                int n;
                if ((n = (int) isNumberParser(val))==-1 || n<=0) {
                    printf("Errore config file : max_files richiede un numero>0 come valore\n");
                    printf ("Server avviato con parametri di DEFAULT\n");
                    break;
                }
                else{
                    configuration->num_files = n;
                }

            } else if (strcmp(arg,"Dimensione Massima")==0) {

                int n;
                if ((n = (int) isNumberParser(val))==-1 || n<=0) {
                    printf("Errore config file : max_dim richiede un numero>0 come valore\n");
                    printf ("Server avviato con parametri di DEFAULT\n");
                    break;
                }
                else{
                    configuration->size_Buff = n;
                }

            } else if (strcmp(arg,"Nome Socket")==0) {
                configuration->socket_name = malloc(20 * sizeof(char));
                strcpy(configuration->socket_name,val);
            }
        }

    }
    fclose(file);

    return configuration;
}


//Nel caso di errore vado a prendere dei parametri di default
config* default_configuration(){
    config* configuration = malloc(sizeof(config));
    configuration->num_files = 10;
    configuration->num_thread = 1;
    configuration->size_Buff = 10000;
    CHECKNULL(configuration->socket_name, malloc(BUFSIZE), "malloc");
    strcmp(configuration->socket_name, SOCKNAME);
    return configuration;
}

//Stampa la struttura
void stampa(config* configuration){
    printf("Numero thread: %d\n", configuration->num_thread);
    printf("Spazio memoria: %zu\n", configuration->size_Buff);
    printf("Numero files: %d\n", configuration->num_files);
    printf("Nome socket: %s\n", configuration->socket_name);
}

//Libera lo spazio in memoria
int freeConfig(config* configuration){
    if(configuration != NULL){
        configuration->num_files = 0;
        configuration->size_Buff = 0;
        configuration->num_thread = 0;
        if(configuration->socket_name != NULL){
            char *string = configuration->socket_name;
            free(string);
        }
        free(configuration);
        return 0;
    }
    return -1;
}
