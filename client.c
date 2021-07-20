#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "util.h"
#include "interface.h"

//TODO: manca la gestione della lista su cui andare a mettere in coda le richieste del client

//variabile che serve per abilitare e disabilitare le stampe chiamando l'opzione -p
int flag_stampa = 0;

int main(int argc, char* argv[]){
    char opt;

    char *farg;
    int checkH = 0, checkP = 0, checkF = 0;

    while((opt = getopt(argc, argv, "hpf:w:W:r:R:d:D:t:c:l:u:")) != -1){
        switch(opt){
            case 'h':
                if(checkH == 0){
                    if(DEBUGCLIENT) printf("Opzione -h\n");
                    checkH = 1;
                    //aggiungere alla lista
                }
                else{
                    printf("L'opzione -h non puo' essere ripetuta\n");
                }
                if(DEBUGCLIENT) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'p':
                if (checkP == 0) {
                    if(DEBUGCLIENT) printf("Opzione -p\n");
                    checkP = 1;
                    //aggiungere alla lista
                }else{
                    printf("L'opzione -p non puo' essere ripetuta\n");
                }
                if(DEBUGCLIENT) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'f':
                if (checkF == 0) {
                    checkF = 1;
                    farg = optarg;
                    //aggiungere alla lista
                    if(DEBUGCLIENT) printf("Opzione -f con argomento : %s\n",farg);
                }else{
                    printf("L'opzione -f non puo' essere ripetuta\n");
                }
                if(DEBUGCLIENT) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'w':
                break;
            case 'W':
                break;
            case 'r':
                break;
            case 'R':
                break;
            case 'd':
                break;
            case 'D':
                break;
            case 't':
                break;
            case 'l':
                break;
            case 'u':
                break;
            case 'c':
                break;
            case '?':
                printf("l'opzione '-%c' non e' gestita\n", optopt);
                fprintf (stderr,"%s -h per vedere la lista delle operazioni supportate\n",argv[0]);
                break;

            case ':':
                printf("l'opzione '-%c' richiede un argomento\n", optopt);
                break;

            default:;
        }
    }

    return 0;
}
