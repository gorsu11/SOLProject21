#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <libgen.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>

#include "util.h"
#include "interface.c"
#include "commandList.c"

//-------------- STRUTTURE PER SALVARE I DATI ---------------//
//variabile che serve per abilitare e disabilitare le stampe chiamando l'opzione -p
int flags = 0;
static int number = 0;
int p_time = 0;

void listDir (char * dirname, int n);
//-------------------------------------------------------------//

int main(int argc, char* argv[]){
    char opt;

    char *farg = NULL, *Darg = NULL, *darg = NULL, *targ = NULL, *warg, *Warg, *rarg, *Rarg, *larg, *uarg, *carg;
    int checkH = 0, checkP = 0, checkF = 0;

    int checkRead = 0;

    char* resolvedPath = NULL;
    char* dir = NULL;

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
                if(DEBUGCLIENT) printf("Opzione -r con argomento %s\n",rarg);
                addList(&lis,"r", rarg);
                if(DEBUGCLIENT) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'R':
                Rarg = optarg;
                if(DEBUGCLIENT) printf("Opzione -R con argomento %s\n",Rarg);
                addList(&lis,"R", Rarg);
                if(DEBUGCLIENT) fprintf(stdout, "Inserito %c\n", opt);
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
                if(DEBUGCLIENT) printf("Opzione -l con argomento %s\n",larg);
                addList(&lis,"l", larg);
                if(DEBUGCLIENT) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'u':
                uarg = optarg;
                if(DEBUGCLIENT) printf("Opzione -u con argomento %s\n",uarg);
                addList(&lis,"u", uarg);
                if(DEBUGCLIENT) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'c':
                carg = optarg;
                if(DEBUGCLIENT) printf("Opzione -c con argomento %s\n",carg);
                addList(&lis,"c",carg);
                if(DEBUGCLIENT) fprintf(stdout, "Inserito %c\n", opt);
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

    printList(lis);

    char *temp = NULL;

    //esaurisco le richieste che possono esser chiamate una sola volta
    if(searchCommand(&lis, "h", &temp) == 1){
        printf("Operazioni supportate : \n-h\n-f filename\n-w dirname[,n=0]\n-W file1[,file2]\n-r file1[,file2]\n-R [n=0]\n-d dirname\n-t time\n-c file1[,file2]\n-p\n");

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
                 printf("Operazione : -f (connessione) File : %s Esito : negativo\n", farg);
             }
             perror("Errore apertura connessione");
         }
         else{
             printf("Connessione aperta\n");
             if(flags == 1){
                 printf("Operazione : -f (connessione) File : %s Esito : positivo\n",farg);
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
        if((p_time = (int) isNumberParser(targ)) == -1){
            if (flags == 1) printf("Operazione : -t (ritardo)\nTempo : %s\nEsito : negativo\n",targ);
            printf("L'opzione -t richiede un numero come argomento\n");
        }
        else{
            if (flags == 1) printf("Operazione : -t (ritardo)\nTempo : %d\nEsito : positivo\n",p_time);
        }
    }

    //per vedere se mi toglie i casi che hanno una sola occorrenza
    if(DEBUGCLIENT) printList(lis);

    node* curr = lis;

    //gestisco i comandi mancanti (-w -W -r -R -l -u -c)
    while(curr != NULL){
        usleep(1000* p_time);

        if (strcmp(curr->cmd, "w") == 0) {
            char* save1 = NULL;
            char* token1 = strtok_r(curr->arg, ",", &save1);

            char* namedir = token1;
            int n;

            struct stat info_dir;

            //caso in cui non trova la directory
            if(stat(namedir, &info_dir) == -1){
                if(flags == 1){
                    printf("Operazione : -w (scrivi directory) Directory : %s Esito : negativo\n",namedir);
                }
                printf("Directory %s non esiste\n",namedir);
            }

            else{
                if(S_ISDIR(info_dir.st_mode)){
                    token1 = strtok_r(NULL, ",", &save1);
                    if(token1 != NULL){
                        n = (int) isNumberParser(token1);
                    }
                    else{       //di default per le richieste del progetto
                        n = 0;
                    }

                    if(n > 0){
                        listDir(namedir,n);
                        if (flags == 1){
                            printf("Operazione : -w (scrivi directory) Directory : %s Esito : positivo\n",namedir);
                        }
                    }
                    else if(n == 0){
                        listDir(namedir, INT_MAX);
                        if (flags==1){
                            printf("Operazione : -w (scrivi directory) Directory : %s Esito : positivo\n",namedir);
                        }
                    }
                    else{
                        if (flags == 1){
                            printf("Operazione : -w (scrivi directory) Directory : %s Esito : negativo\n",namedir);
                            printf("Utilizzo : -w dirname[,n]\n");
                        }
                    }
                }
                else{
                    if (flags == 1){
                        printf("Operazione : -w (scrivi directory) Directory : %s Esito : negativo\n",namedir);
                    }
                    printf("%s non e' una directory\n",namedir);
                }
            }
        }

        if (strcmp(curr->cmd, "W") == 0) {
            char* save2 = NULL;
            char* token2 = strtok_r(curr->arg, ",", &save2);

            while(token2){
                char* file = token2;

                if((resolvedPath = realpath(file, resolvedPath)) == NULL){
                    if (flags == 1){
                        printf("Operazione : -W (scrivi file) File : %s Esito : negativo\n",file);
                    }
                    printf("Il file %s non esiste\n",file);
                }
                else{
                    struct stat info_file;
                    stat(resolvedPath, &info_file);

                    if(S_ISREG(info_file.st_mode)){
                        if (openFile(resolvedPath, 1) == -1) {
                            if (flags == 1){
                                printf("Operazione : -W (scrivi file) File : %s Esito : negativo\n",file);
                            }
                            perror("Errore apertura file");
                        }
                        else{
                            if (writeFile(resolvedPath, NULL) == -1) {
                                if (flags == 1){
                                    printf("Operazione : -W (scrivi file) File : %s Esito : negativo\n",file);
                                }
                                perror("Errore scrittura file");
                            }
                            else if (closeFile(resolvedPath) == -1) {
                                if (flags == 1){
                                    printf("Operazione : -W (scrivi file) File : %s Esito : negativo\n",file);
                                }
                                perror("Errore chiusura file");
                            }
                            else{
                                if (flags == 1){
                                    printf("Operazione : -W (scrivi file) File : %s Esito : positivo\n",file);
                                }
                            }
                        }
                    }
                    else{
                        if (flags == 1){
                            printf("Operazione : -W (scrivi file) File : %s Esito : negativo\n",file);
                        }
                        printf("%s non e' un file regolare\n",file);
                    }
                }
                token2 = strtok_r(NULL, ",", &save2);
            }
        }

        if (strcmp(curr->cmd, "r") == 0) {
            char* save3 = NULL;
            char* token3 = strtok_r(curr->arg, ",", &save3);

            while(token3){
                char* file = token3;

                if(openFile(file, 0) == -1){
                    if (flags == 1){
                        printf("Operazione : -r (leggi file) File : %s Esito : negativo\n",file);
                    }
                    perror("Errore apertura file");
                }
                else{
                    char* buf = NULL;
                    size_t size;

                    if(readFile(file, (void**)buf, &size) == -1){
                        if (flags == 1){
                            printf("Operazione : -r (leggi file) File : %s Esito : negativo\n",file);
                        }
                        perror("Errore lettura file");
                    }
                    else{
                        if(checkRead == 1){
                            char path[PATH_MAX];
                            memset(path, 0, PATH_MAX);

                            char* file_name = basename(file);
                            sprintf(path,"%s/%s",dir,file_name);

                            //CREA DIR SE NON ESISTE
                            mkdir_p(dir);

                            //CREA FILE SE NON ESISTE
                            FILE* of = fopen(path, "w");
                            if (of==NULL) {
                                printf("Errore salvataggio file\n");
                            }
                            else {
                                fprintf(of,"%s",buf);
                                fclose(of);
                            }
                        }
                        free(buf);
                    }
                    if(closeFile(file) == -1){
                        if (flags == 1){
                            printf("Operazione : -r (leggi file) File : %s Esito : negativo\n",file);
                        }
                        perror("Errore chiusura file");
                    }
                    else{
                        if (flags == 1){
                            printf("Operazione : -r (leggi file) File : %s Esito : positivo\n",file);
                        }
                    }
                }

                token3 = strtok_r(NULL, ",", &save3);
            }
            if(token3 != NULL){
                free(token3);
            }
        }

        if (strcmp(curr->cmd, "R") == 0) {
            int val;

            if((val = (int) isNumberParser(curr->arg)) == -1){
                if (flags==1){
                    printf("Operazione : -R (leggi N file) Esito : negativo\n");
                }
                printf("L'opzione -R vuole un numero come argomento\n");
            }
            else{
                int n;
                if((n = readNFiles(val, dir)) == -1){
                    if (flags == 1){
                        printf("Operazione : -R (leggi N file) Esito : negativo\n");
                    }
                    perror("Errore lettura file");
                }
                else{
                    if (flags == 1){
                        printf("Operazione : -R (leggi N file) Esito : positivo File Letti : %d\n",n);
                    }
                }
            }
        }

        if (strcmp(curr->cmd, "l") == 0) {
            char *save4 = NULL;
            char *token4 = strtok_r(curr->arg, ",", &save4);

            while(token4){
                char* file = token4;

                if(lockFile(file) == -1){
                    if (flags == 1){
                        printf("Operazione : -l (blocco file) File : %s Esito : negativo\n",file);
                    }
                    perror("Errore blocco file");
                }
                else{
                    if (flags == 1){
                        printf("Operazione : -l (blocco file) File : %s Esito : positivo\n",file);
                    }
                }

                token4 = strtok_r(NULL, ",", &save4);
            }
        }

        if (strcmp(curr->cmd, "u") == 0) {
            char *save5 = NULL;
            char *token5 = strtok_r(curr->arg, ",", &save5);

            while(token5){
                char* file = token5;

                if(unlockFile(file) == -1){
                    if (flags == 1){
                        printf("Operazione : -u (sblocco file) File : %s Esito : negativo\n",file);
                    }
                    perror("Errore sblocco file");
                }
                else{
                    if (flags == 1){
                        printf("Operazione : -u (sblocco file) File : %s Esito : positivo\n",file);
                    }
                }

                token5 = strtok_r(NULL, ",", &save5);
            }
        }

        if (strcmp(curr->cmd, "c") == 0) {
            char *save6 = NULL;
            char *token6 = strtok_r(curr->arg, ",", &save6);

            while(token6){
                char* file = token6;

                if(removeFile(file) == -1){
                    if (flags == 1){
                        printf("Operazione : -c (rimuovi file) File : %s Esito : negativo\n",file);
                    }
                    perror("Errore rimozione file");
                }
                else{
                    if (flags == 1){
                        printf("Operazione : -c (rimuovi file) File : %s Esito : positivo\n",file);
                    }
                }

                token6 = strtok_r(NULL, ",", &save6);
            }
        }

        curr = curr->next;
        if(DEBUGCLIENT) printList(lis);
    }

    freeList(&lis);
    closeConnection(farg);

    return 0;
}


void listDir (char * dirname, int n){
    DIR* dir;

    struct dirent* entry;

    if ((dir = opendir(dirname)) == NULL || number == n) {
        return;
    }

    while((entry = readdir(dir)) != NULL || number < n){
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

        struct stat info;

        SYSCALL_EXIT("stat", notused, stat(path, &info), "stat", "");

        //se il file è una directory
        if (S_ISDIR(info.st_mode)) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            listDir(path,n);
        }

        else{
            char * rPath = NULL;
            if ((rPath = realpath(path,rPath)) == NULL) {
                perror("realpath");
            }

            else{
                if (openFile(rPath,1) == -1) perror("Errore apertura file");
                else {
                    number++;
                    if (writeFile(rPath, NULL) == -1) perror("Errore scrittura file");
                    if (closeFile(rPath) == -1) perror("Errore chiusura file");
                }
                if (rPath!=NULL) free(rPath);
            }
            usleep(1000* p_time);
        }
    }

    SYSCALL_EXIT("closedir", notused, closedir(dir), "closedir", "");

}
