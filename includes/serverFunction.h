#ifndef serverFunction_h
#define serverFunction_h

#include "util.h"
#include "conn.h"

//--------------- GESTIONE NODI ----------------//
typedef struct node {
    int data;
    struct node  * next;
} node;
//----------------------------------------------//

//--------------- GESTIONE CONCORRENZA ----------------//
typedef struct lockFile{
    pthread_mutex_t mutex_file;
    pthread_cond_t cond_file;
    unsigned int waiting;
    bool writer_active;
} lockFile;
//----------------------------------------------//

//--------------- GESTIONE FILE ----------------//
typedef struct file {
    char path[PATH_MAX];            //nome
    char * data;                    //contenuto file
    node * client_open;             //lista client che lo hanno aperto

    int lock_flag;                 //variabile che mi tiene conto se un file è in lock da parte di un client
    node* testa_lock;
    node* coda_lock;                //lista che mi tiene conto dei client in coda per la lock
    lockFile concorrency;           //per la gestione della concorrenza

    int client_write;               //FILE DESCRIPTOR DEL CLIENT CHE HA ESEGUITO COME ULTIMA OPERAZIONE UNA openFile con flag O_CREATE
    struct file * next;
} file;
//----------------------------------------------//

void insertNode (node ** list, int data);
int removeNode (node ** list);
int updatemax(fd_set set, int fdmax);

//-------------------- FUNZIONI SOLO DICHIARATE --------------------//
int aggiungiFile(char* path, int flag, int cfd, char* dirname);
int rimuoviCliente(char* path, int cfd);
int rimuoviFile(char* path, int cfd);
int inserisciDati(char* path, char* data, int size, int cfd, char* dirname);
int appendDati(char* path, char* data, int size, int cfd,  char* dirname);
int bloccaFile(char* path, int cfd);
int sbloccaFile(char* path, int cfd);
char* prendiFile (char* path, int cfd);

void printFile (file* cache);
void freeList(node** head);
int fileOpen(node* list, int cfd);

int resizeCache(int dim, char* dirname);
int mkdir_p(const char *path);
void printClient (node * list);

void execute (char * request, int cfd,int pfd);
void* Workers(void *argument);

int setLock(file* curr, int cfd);
int setUnlock(file* curr, int cfd);

void push(node **testa, node **coda, int data);
int pop(node **testa, node **coda);
int find(node** testa, int data);
int free_L(node** testa, node** coda);

void rwLock_start(lockFile* file);
void rwLock_end(lockFile* file);

#endif /* serverFunction_h */
