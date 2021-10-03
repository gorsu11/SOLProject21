#include "../includes/util.h"
#include "../includes/conn.h"
#include "../includes/interface.h"
#include "../includes/commandList.h"


//-------------- STRUTTURE PER SALVARE I DATI ---------------//
//variabile che serve per abilitare e disabilitare le stampe chiamando l'opzione -p
int flags = 0;
static int num_files = 0;
int tms = 0;

void listDir (char * dirname, int n, char* my_directory);
long isNumberParser(const char* s);
//-------------------------------------------------------------//


int main(int argc, char *argv[]) {
    char opt;

    char *farg = NULL, *Darg = NULL, *darg = NULL, *targ = NULL, *warg, *Warg, *rarg, *Rarg, *larg, *uarg, *carg;
    int checkH = 0, checkP = 0, checkF = 0, checkw = 0, checkW = 0, checkr = 0, checkR = 0;

    int checkRead = 0;

    char* resolvedPath = NULL;

    node* lis = NULL;

    while((opt = getopt(argc, argv, "hpf:w:W:r:R:d:D:t:c:l:u:")) != -1){
        switch(opt){
            case 'h':
                if(checkH == 0){
                    if(DEBUGCLIENT) printf("[CLIENT] Opzione -h\n");
                    checkH = 1;
                    addList(&lis, "h", NULL);
                }
                else{
                    printf("L'opzione -h non puo' essere ripetuta\n");
                }
                if(DEBUGCLIENT) fprintf(stdout, "[CLIENT] Inserito %c\n", opt);
                break;

            case 'p':
                if (checkP == 0) {
                    if(DEBUGCLIENT) printf("[CLIENT] Opzione -p\n");
                    checkP = 1;
                    addList(&lis, "p", NULL);
                }else{
                    printf("L'opzione -p non puo' essere ripetuta\n");
                }
                if(DEBUGCLIENT) fprintf(stdout, "[CLIENT] Inserito %c\n", opt);
                break;

            case 'f':
                if (checkF == 0) {
                    checkF = 1;
                    farg = optarg;
                    addList(&lis, "f", farg);
                    if(DEBUGCLIENT) printf("[CLIENT] Opzione -f con argomento : %s\n",farg);
                }else{
                    printf("L'opzione -f non puo' essere ripetuta\n");
                }
                if(DEBUGCLIENT) fprintf(stdout, "[CLIENT] Inserito %c\n", opt);
                break;

            case 'w':
                warg = optarg;
                checkw = 1;
                if(DEBUGCLIENT) printf("[CLIENT] Opzione -w con argomento : %s\n",warg);
                addList(&lis, "w", warg);
                if(DEBUGCLIENT) fprintf(stdout, "[CLIENT] Inserito %c\n", opt);
                break;

            case 'W':
                Warg = optarg;
                checkW = 1;
                if(DEBUGCLIENT) printf("[CLIENT] Opzione -W con argomento : %s\n",Warg);
                addList(&lis, "W", Warg);
                if(DEBUGCLIENT) fprintf(stdout, "[CLIENT] Inserito %c\n", opt);
                break;

            case 'r':
                rarg = optarg;
                checkr = 1;
                if(DEBUGCLIENT) printf("[CLIENT] Opzione -r con argomento %s\n",rarg);
                addList(&lis,"r", rarg);
                if(DEBUGCLIENT) fprintf(stdout, "[CLIENT] Inserito %c\n", opt);
                break;

            case 'R':
                Rarg = optarg;
                checkR = 1;
                if(DEBUGCLIENT) printf("[CLIENT] Opzione -R con argomento %s\n",Rarg);
                addList(&lis,"R", Rarg);
                if(DEBUGCLIENT) fprintf(stdout, "[CLIENT] Inserito %c\n", opt);
                break;

            case 'd':
                darg = optarg;
                if(DEBUGCLIENT) printf("[CLIENT] Opzione -d con argomento : %s\n",darg);
                addList(&lis, "d", darg);
                if(DEBUGCLIENT) fprintf(stdout, "[CLIENT] Inserito %c\n", opt);
                break;

            case 'D':
                Darg = optarg;
                if(DEBUGCLIENT) printf("[CLIENT] Opzione -D con argomento : %s\n",Darg);
                addList(&lis, "D", Darg);
                if(DEBUGCLIENT) fprintf(stdout, "[CLIENT] Inserito %c\n", opt);
                break;

            case 't':
                targ = optarg;
                if(DEBUGCLIENT) printf("[CLIENT] Opzione -t con argomento : %s\n",targ);
                addList(&lis, "t", targ);
                if(DEBUGCLIENT) fprintf(stdout, "Inserito %c\n", opt);
                break;

            case 'l':
                larg = optarg;
                if(DEBUGCLIENT) printf("[CLIENT] Opzione -l con argomento %s\n",larg);
                addList(&lis,"l", larg);
                if(DEBUGCLIENT) fprintf(stdout, "[CLIENT] Inserito %c\n", opt);
                break;

            case 'u':
                uarg = optarg;
                if(DEBUGCLIENT) printf("[CLIENT] Opzione -u con argomento %s\n",uarg);
                addList(&lis,"u", uarg);
                if(DEBUGCLIENT) fprintf(stdout, "[CLIENT] Inserito %c\n", opt);
                break;

            case 'c':
                carg = optarg;
                if(DEBUGCLIENT) printf("[CLIENT] Opzione -c con argomento %s\n",carg);
                addList(&lis,"c",carg);
                if(DEBUGCLIENT) fprintf(stdout, "[CLIENT] Inserito %c\n", opt);
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

    if(DEBUGCLIENT) printList(lis);
    if(DEBUGCLIENT) printf("[CLIENT] Il valore di checkr %d e il valore di checkR è %d\n", checkr, checkR);
    char* arg = NULL;

    //gestione delle chiamate semplici
    if(containCMD(&lis, "h", &arg) == 1){
        printf("Operazioni supportate: \n-h\n-f filename\n-w dirname[,n=0]\n-W file1[,file2]\n-D dirname\n-r file1[,file2]\n-R [n=0]\n-d dirname\n-t time\n-l file1[,file2]\n-u file1[,file2]\n-c file1[,file2]\n-p\n");

        freeList(&lis);
        return 0;
    }

    if(containCMD(&lis, "p", &arg) == 1){
        flags = 1;
        printf("Stampe abilitate con successo\n");
    }

    if(containCMD(&lis, "t", &arg) == 1){
        if((tms = (int) isNumberParser(targ)) == -1){
            if (flags == 1) printf("Operazione : -t (ritardo) Tempo : %s Esito : negativo\n",targ);
            printf("L'opzione -t richiede un numero come argomento\n");
        }
        else{
            if (flags == 1) printf("Operazione : -t (ritardo) Tempo : %d Esito : positivo\n",tms);
        }
    }

    if(containCMD(&lis, "f", &arg) == 1){
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
             if(DEBUGCLIENT) printf("Connessione aperta\n");
             if(flags == 1){
                 printf("Operazione : -f (connessione) File : %s Esito : positivo\n",farg);
             }
         }
    }

    if(DEBUGCLIENT) printList(lis);

    node* curr = lis;

    //gestisco i comandi mancanti (-w -W -r -R -c -l -u -D -d)
    while(curr != NULL){
        usleep(1000* tms);

        if(strcmp(curr->cmd, "w") == 0){
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
                    if(DEBUGCLIENT) printf("[CLIENT] Il numero passato nell'opzione -w è %d\n", n);
                    if(n > 0){
                        listDir(namedir, n, Darg);
                        if (flags == 1){
                            printf("Operazione : -w (scrivi directory) Directory : %s Esito : positivo\n",namedir);
                        }
                    }
                    else if(n == 0){
                        //{INT_MAX} Maximum value of an int.\ [CX] [Option Start] Minimum Acceptable Value: 2 147 483 647 [Option End]
                        listDir(namedir, INT_MAX, Darg);
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

        if(strcmp(curr->cmd, "W") == 0){
            char* save2 = NULL;
            char* token2 = strtok_r(curr->arg, ",", &save2);

            while(token2){
                if(DEBUGCLIENT) printf("[CLIENT] Token passato: %s\n", token2);

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
                            if(DEBUGCLIENT) printf("[CLIENT] Entra su openFile\n");
                            if (flags == 1){
                                printf("Operazione : -W (scrivi file) File : %s Esito : negativo\n",file);
                            }
                            perror("Errore apertura file");
                        }
                        else{
                            if(DEBUGCLIENT) printf("[CLIENT] resolvedPath è: %s\n", resolvedPath);
                            if (writeFile(resolvedPath, Darg) == -1) {
                                if(DEBUGCLIENT) printf("[CLIENT] Entra su writeFile\n");
                                if (flags == 1){
                                    printf("Operazione : -W (scrivi file) File : %s Esito : negativo\n",file);
                                }
                                perror("Errore scrittura file");
                            }
                            else if (closeFile(resolvedPath)==-1) {
                                if(DEBUGCLIENT) printf("[CLIENT] Entra su closeFile\n");
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

                    if(DEBUGCLIENT) printf("[CLIENT] Opzione -W svolta con successo su %s\n", token2);
                }
                token2 = strtok_r(NULL, ",", &save2);
            }
        }

        if(strcmp(curr->cmd, "r") == 0){
            char* save3 = NULL;
            char* token3 = strtok_r(curr->arg, ",", &save3);

            while(token3){
                if(DEBUGCLIENT) printf("[CLIENT] Token passato nell'opzione r: %s\n", token3);
                char* file = token3;

                if(openFile(file, 0) == -1){
                    if (flags == 1){
                        printf("Operazione : -r (leggi file) File : %s Esito : negativo\n",file);
                    }
                    perror("Errore apertura file");
                }
                else{

                    if(DEBUGCLIENT) printf("[CLIENT] Entra su openFile\n");
                    char* buf = NULL;
                    size_t size;

                    if(readFile(file, (void**) buf, &size) == -1){

                        if(DEBUGCLIENT) printf("[CLIENT] Il risultato della readFile è -1\n");
                        if (flags == 1){
                            printf("Operazione : -r (leggi file) File : %s Esito : negativo\n",file);
                        }
                        perror("Errore lettura file");
                    }
                    else{
                        if(DEBUGCLIENT) printf("[CLIENT] Entra su readFile leggendo il file %s\n", file);
                        if(checkRead == 1){
                            if(DEBUGCLIENT) printf("[CLIENT] checkRead ha valore 1 e dir ha valore %s\n", darg);
                            char path[PATH_MAX];
                            memset(path, 0, PATH_MAX);

                            char* file_name = basename(file);
                            sprintf(path,"%s/%s",darg,file_name);
                            if(DEBUGCLIENT) printf("[CLIENT] file_name ha valore %s\n", file_name);
                            if(DEBUGCLIENT) printf("[CLIENT] Path ha valore %s\n", path);
                            //CREA DIR SE NON ESISTE
                            mkdir_p(darg);

                            //CREA FILE SE NON ESISTE
                            FILE* of = fopen(path, "w");
                            if (of == NULL) {
                                printf("Errore salvataggio file\n");
                            }
                            else {
                                fprintf(of,"%s",buf);
                                fclose(of);
                                if(DEBUGCLIENT) printf("[CLIENT] Scritto nel file il buffer passato\n");
                            }
                        }
                        if(DEBUGCLIENT) printf("[CLIENT] Il buffer contiene %s\n", buf);
                        free(buf);
                        if(closeFile(file) == -1){
                            if (flags == 1){
                                printf("Operazione : -r (leggi file) File : %s Esito : negativo\n",file);
                            }
                            perror("Errore chiusura file");
                        }
                        else{
                            if(DEBUGCLIENT) printf("[CLIENT] Entra su closeFile\n");
                            if (flags == 1){
                                printf("Operazione : -r (leggi file) File : %s Esito : positivo\n",file);
                            }
                        }
                    }

                }

                token3 = strtok_r(NULL, ",", &save3);
            }

            if(token3 != NULL){
                free(token3);
            }
        }

        if(strcmp(curr->cmd, "R") == 0){
            int val;

            if((val = (int) isNumberParser(curr->arg)) == -1){
                if (flags==1){
                    printf("Operazione : -R (leggi N file) Esito : negativo\n");
                }
                printf("L'opzione -R vuole un numero come argomento\n");
            }
            else{
                int n;
                if((n = readNFiles(val, darg)) == -1){
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

        if(strcmp(curr->cmd, "l") == 0) {
          char *save4 = NULL;
          char *token4 = strtok_r(curr->arg, ",", &save4);

          if(DEBUGCLIENT) printf("[CLIENT] Token passato nell'opzione l: %s\n", token4);
          //printf("1: Token:%s, Save:%s\n", token4, save4);
          while(token4){
              char* file = token4;
              //printf("File in carica: %s\n", file);
              if(openFile(file, 0) == -1){
                  if (flags == 1){
                      printf("Operazione : -r (leggi file) File : %s Esito : negativo\n",file);
                  }
                  perror("Errore apertura file");
              }

              else{
                  if(DEBUGCLIENT) printf("[CLIENT] Entrato su openFile con successo\n");
                  if(lockFile(file) == -1){
                    //printf("Faccio la LockFile su %s\n", file);
                      if (flags == 1){
                          printf("Operazione : -l (blocco file) File : %s Esito : negativo\n",file);
                      }
                      perror("Errore blocco file");
                  }
                  //if(DEBUGCLIENT) printf("[CLIENT] Il risultato di closeFile è %d\n", closeFile(file));
                  else if(closeFile(file) == -1){
                      if (flags == 1){
                          printf("Operazione : -r (leggi file) File : %s Esito : negativo\n",file);
                      }
                      perror("Errore chiusura file");
                  }
                  else{
                      if(DEBUGCLIENT) printf("[CLIENT] Entrato su closeFile con successo\n");
                      if (flags == 1){
                          printf("Operazione : -l (blocco file) File : %s Esito : positivo\n",file);
                      }
                      if(DEBUGCLIENT) printf("[CLIENT] lockFile su %s eseguita con successo\n", file);
                  }
                }
                token4 = strtok_r(NULL, ",", &save4);
            }
        }

        if(strcmp(curr->cmd, "u") == 0) {
          char *save5 = NULL;
          char *token5 = strtok_r(curr->arg, ",", &save5);

          if(DEBUGCLIENT) printf("[CLIENT] Token passato nell'opzione l: %s\n", token5);
          //printf("1: Token:%s, Save:%s\n", token4, save4);
          while(token5){
              char* file = token5;
              //printf("File in carica: %s\n", file);
              if(openFile(file, 0) == -1){
                  if (flags == 1){
                      printf("Operazione : -u (sblocco file) File : %s Esito : negativo\n",file);
                  }
                  perror("Errore apertura file");
              }

              else{
                  if(DEBUGCLIENT) printf("[CLIENT] Entrato su openFile con successo\n");
                  if(unlockFile(file) == -1){
                    //printf("Faccio la LockFile su %s\n", file);
                      if (flags == 1){
                          printf("Operazione : -u (sblocco file) File : %s Esito : negativo\n",file);
                      }
                      perror("Errore blocco file");
                  }
                  if(DEBUGCLIENT) printf("[CLIENT] Entrato su unlockFile con successo\n");
                  else if(closeFile(file) == -1){
                      if (flags == 1){
                          printf("Operazione : -u (sblocco file) File : %s Esito : negativo\n",file);
                      }
                      perror("Errore chiusura file");
                  }
                  else{
                      if(DEBUGCLIENT) printf("[CLIENT] Entrato su closeFile con successo\n");
                      if (flags == 1){
                          printf("Operazione : -u (sblocco file) File : %s Esito : positivo\n",file);
                      }
                  }
              }
              token5 = strtok_r(NULL, ",", &save5);
          }
        }

        if(strcmp(curr->cmd, "c") == 0){
            char *save6 = NULL;
            char *token6 = strtok_r(curr->arg, ",", &save6);

            //printf("2: Token:%s, Save:%s\n", token6, save6);
            while(token6){
                char* file = token6;
                //printf("2: Token:%s, Save:%s, File:%s\n", token6, save6, file);
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

        if(strcmp(curr->cmd, "d") == 0){
            char *save7 = NULL;
            darg = strtok_r(curr->arg, ",", &save7);

            if(checkr == 0 && checkR == 0){
                if (flags == 1) printf("Operazione : -d (salva file) Directory : %s Esito : negativo\n",darg);
            }
            else{
                checkRead = 1;
                if (flags == 1) printf("Operazione : -d (salva file) Directory : %s Esito : positivo\n",darg);
            }
        }

        if(strcmp(curr->cmd, "D") == 0){
            char *save8 = NULL;
            Darg = strtok_r(curr->arg, ",", &save8);

            if(checkw == 0 && checkW == 0){
                if (flags == 1) printf("Operazione : -D (scrivi file rimossi) Directory : %s Esito : negativo\n",Darg);
            }
            else{
                if (flags == 1) printf("Operazione : -D (scrivi file rimossi) Directory : %s Esito : positivo\n",Darg);
            }
        }

        curr = curr -> next;
        if(DEBUGCLIENT) printList(curr);
    }

    //Libero la memoria e chiudo la connessione
    freeList(&lis);
    closeConnection(farg);

    return 0;
}

void listDir (char * dirname, int n, char* my_directory) {

    DIR * dir;
    struct dirent* entry;

    if ((dir = opendir(dirname)) == NULL || num_files == n) {
        return;
    }

    if(DEBUGCLIENT) printf("[CLIENT] Directory %s e valore %d\n", dirname, n);

    while ((entry = readdir(dir))!=NULL && (num_files < n)) {

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

        struct stat info;
        if (stat(path,&info)==-1) {
        perror("stat");
        exit(EXIT_FAILURE);
        }

        //SE FILE E' UNA DIRECTORY
        if (S_ISDIR(info.st_mode)) {
            if (strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0) continue;
            listDir(path, n, my_directory);
        } else {
            char * resolvedPath = NULL;
            if ((resolvedPath = realpath(path,resolvedPath))==NULL) {
                perror("realpath");
            }else{
                if (openFile(resolvedPath, 1) == -1) perror("Errore apertura file");
                else {
                    num_files++;
                    //WRITE FILE
                    if (writeFile(resolvedPath, my_directory) == -1) perror("Errore scrittura file");
                    if (closeFile(resolvedPath) == -1) perror("Errore chiusura file");
                }
                if (resolvedPath!=NULL) free(resolvedPath);
            }
            usleep(1000*tms);

        }
    }

    SYSCALL_EXIT("close directory", notused, (closedir(dir)), "close dir", "");
}

long isNumberParser(const char* s) {
   char* e = NULL;
   long val = strtol(s, &e, 0);
   if (e != NULL && *e == (char)0) return val;
   return -1;
}
