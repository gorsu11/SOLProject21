#include "../includes/log.h"


//Stampa nel file di log il thread che sta lavorando, il client che ha fatto la richiesta e l'ora e la data di quando la richiesta è stata fatta.
void writeLogFd(FILE* log, int cfd){
    if(log != NULL){
        time_t timer;
        char timeString[20];
        struct tm* tm_info;
        timer = time(NULL);
        tm_info = localtime(&timer);
        strftime(timeString, 20, "%d-%m-%Y %H:%M:%S", tm_info);

        CONTROLLA(fprintf(log, "Thread Worker: %ld \tClient fd: %d \tData: %s\n", (long) pthread_self(), cfd, timeString));
    }
    else{
        fprintf(stderr, "Nessun file passato");
    }
}

//Restituisce la stringa con la data e l'ora attuale
char* getTime(){
    time_t timer;
    char *timeString = malloc(sizeof(char) * 20);
    struct tm* tm_info;
    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(timeString, 20, "%d-%m-%Y %H:%M:%S", tm_info);

    return timeString;
}


//Stampa ESITO POSITIVO se il valore passato in ingresso "value" è diverso da -1, altrimenti stampa ESITO NEGATIVO
void valutaEsito(FILE* log, int value, char* text){
    if(value >= 0){
        if(fprintf(log, "Esito %s: POSITIVO\n\n", text) == -1){
            exit(errno);
        }
    }
    else{
        if(fprintf(log, "Esito %s: NEGATIVO\n\n", text) == -1){
            exit(errno);
        }
    }
}
