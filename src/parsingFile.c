#include "../includes/parsingFile.h"

long isNumberParser(const char* s) {
   char* e = NULL;
   long val = strtol(s, &e, 0);
   if (e != NULL && *e == (char)0) return val;
   return -1;
}

config* getConfig(const char* string){
    FILE *file;

    config *configuration; //Variabile che ospita la configurazione
    CHECKNULL(configuration, malloc(sizeof(config)), "malloc configuration");
    char str[200];
    char arg [100];
    char val [100];

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
                    configuration->sizeBuff = n;
                }

            } else if (strcmp(arg,"Nome Socket")==0) {
                configuration->socket_name = malloc(LENSOCK * sizeof(char));
                strcpy(configuration->socket_name,val);
            }

            else if (strcmp(arg,"File Log")==0) {
                configuration->fileLog = malloc(BUFSIZE * sizeof(char));
                strcpy(configuration->fileLog,val);
            }
        }

    }
    fclose(file);

    return configuration;
}

//Stampa la struttura
void stampa(config* configuration){
    printf("Numero thread: %d\n", configuration->num_thread);
    printf("Spazio memoria: %zu\n", configuration->sizeBuff);
    printf("Numero files: %d\n", configuration->num_files);
    printf("Nome socket: %s\n", configuration->socket_name);
    printf("File di log: %s\n", configuration->fileLog);
}

//Libera lo spazio in memoria
int freeConfig(config* configuration){
    if(configuration != NULL){
        configuration->num_files = 0;
        configuration->sizeBuff = 0;
        configuration->num_thread = 0;

        if(configuration->socket_name != NULL){
            char *string = configuration->socket_name;
            free(string);
        }

        if(configuration->fileLog != NULL){
            char *string1 = configuration->fileLog;
            free(string1);
        }
        free(configuration);
        return 0;
    }
    return -1;
}
