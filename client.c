#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "util.h"
#include "interface.h"
#include "commandList.h"

//-------------- STRUTTURE PER SALVARE I DATI ---------------//
//variabile che serve per abilitare e disabilitare le stampe chiamando l'opzione -p
int flags = 0;
//-------------------------------------------------------------//




int main(int argc, char* argv[]){
    char opt;

    int time = 0;

    char *farg = NULL, *Darg = NULL, *darg = NULL, *targ = NULL, *warg, *Warg, *rarg, *Rarg, *larg, *uarg, *carg;
    int checkH = 0, checkP = 0, checkF = 0;

    node* lis = NULL;

    while((opt = getopt(argc, argv, "hpf:w:W:r:R:d:D:t:c:l:u:")) != -1){
        switch(opt){
            case 'h':
                if(checkH == 0){
                    if(DEBUGCLIENT) printf("Opzione -h\n");
                    checkH = 1;
                    addList(&lis, "h", NULL);
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
                    addList(&lis, "p", NULL);
                }else{
                    printf("L'opzione -p non puo' essere ripetuta\n");
                }
                if(DEBUGCLIENT) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'f':
                if (checkF == 0) {
                    checkF = 1;
                    farg = optarg;
                    addList(&lis, "f", farg);
                    if(DEBUGCLIENT) printf("Opzione -f con argomento : %s\n",farg);
                }else{
                    printf("L'opzione -f non puo' essere ripetuta\n");
                }
                if(DEBUGCLIENT) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'w':
                warg = optarg;
                if(DEBUGCLIENT) printf("Opzione -w con argomento : %s\n",warg);
                addList(&lis, "w", warg);
                if(DEBUGCLIENT) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'W':
                Warg = optarg;
                if(DEBUGCLIENT) printf("Opzione -W con argomento : %s\n",Warg);
                addList(&lis, "W", Warg);
                if(DEBUGCLIENT) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'r':
                rarg = optarg;
                if(DEBUG) printf("Opzione -r con argomento %s\n",rarg);
                addList(&lis,"r", rarg);
                if(DEBUG) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'R':
                Rarg = optarg;
                if(DEBUG) printf("Opzione -R con argomento %s\n",Rarg);
                addList(&lis,"R", Rarg);
                if(DEBUG) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'd':
                darg = optarg;
                if(DEBUGCLIENT) printf("Opzione -d con argomento : %s\n",darg);
                addList(&lis, "d", darg);
                if(DEBUGCLIENT) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'D':
                Darg = optarg;
                if(DEBUGCLIENT) printf("Opzione -D con argomento : %s\n",Darg);
                addList(&lis, "D", Darg);
                if(DEBUGCLIENT) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 't':
                targ = optarg;
                if(DEBUGCLIENT) printf("Opzione -f con argomento : %s\n",targ);
                addList(&lis, "t", targ);
                if(DEBUGCLIENT) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'l':
                larg = optarg;
                if(DEBUG) printf("Opzione -l con argomento %s\n",larg);
                addList(&lis,"l", larg);
                if(DEBUG) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'u':
                uarg = optarg;
                if(DEBUG) printf("Opzione -u con argomento %s\n",uarg);
                addList(&lis,"u", uarg);
                if(DEBUG) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'c':
                carg=optarg;
                if(DEBUG) printf("Opzione -c con argomento %s\n",carg);
                addList(&lis,"c",carg);
                if(DEBUG) fprintf(stdout, "Inserito %c\n", opt);
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

    char *temp = NULL;

    //esaurisco le richieste che possono esser chiamate una sola volta
    if(searchCommand(&lis, "h", &temp) == 1){
        //TODO: stampare la lista di tutte le opzioni accettate

        //libero la lista
        freeList(&lis);

        //TODO: liberare anche tutte le altre dichiarazioni!?
        return 0;
    }

    if(searchCommand(&lis, "p", &temp) == 1){
        flags = 1;
        printf("Stampe abilitate con successo\n");
    }

    if(searchCommand(&lis, "f", &temp) == 1){
        struct timespec t;
        clock_gettime(CLOCK_REALTIME, &t);
        t.tv_sec = t.tv_sec + 60;

        if(openConnection(farg, 1000, t) == -1){
             if(flags == 1){
                 printf("Operazione : -f (connessione)\nFile : %s\nEsito : negativo\n", farg);
             }
             perror("Errore apertura connessione");
         }
         else{
             printf("Connessione aperta\n");
             if(flags == 1){
                 printf("Operazione : -f (connessione)\nFile : %s\nEsito : positivo\n",farg);
             }
         }
    }

    if(searchCommand(&lis, "d", &temp) == 1){
        if (flags == 1) printf("Operazione : -d (salva file)\nDirectory : %s\nEsito : positivo\n",darg);
    }

    if(searchCommand(&lis, "D", &temp) == 1){
        if (flags == 1) printf("Operazione : -D (scrivi file rimossi)\nDirectory : %s\nEsito : positivo\n",Darg);
    }

    if(searchCommand(&lis, "t", &temp) == 1){
        if((time = (int) isNumberParser(targ)) == -1){
            if (flags == 1) printf("Operazione : -t (ritardo)\nTempo : %s\nEsito : negativo\n",targ);
            printf("L'opzione -t richiede un numero come argomento\n");
        }
        else{
            if (flags == 1) printf("Operazione : -t (ritardo)\nTempo : %d\nEsito : positivo\n",time);
        }
    }

    //per vedere se mi toglie i casi che hanno una sola occorrenza
    if(DEBUGCLIENT) printList(lis);

    node* curr = lis;

    //gestisco i comandi mancanti (-w -W -r -R -l -u -c)

    while(curr != NULL){
        usleep(1000* time);

        if (strcmp(curr->cmd, "w") == 0) {
            <#statements#>
        }

        if (strcmp(curr->cmd, "W") == 0) {
            <#statements#>
        }

        if (strcmp(curr->cmd, "r") == 0) {
            <#statements#>
        }

        if (strcmp(curr->cmd, "R") == 0) {
            <#statements#>
        }

        if (strcmp(curr->cmd, "l") == 0) {
            <#statements#>
        }

        if (strcmp(curr->cmd, "u") == 0) {
            <#statements#>
        }

        if (strcmp(curr->cmd, "c") == 0) {
            <#statements#>
        }

        curr = curr->next;
        if(DEBUGCLIENT) printList(lis);
    }

    freeList(&lis);
    closeConnection(farg);

    return 0;
}
