#ifndef serverFunction_h
#define serverFunction_h

#include "util.h"
#include "conn.h"
#include "log.h"

//--------------- GESTIONE NODI ----------------//
typedef struct node {
    int data;
    struct node  * next;
} node;
//----------------------------------------------//

//--------------- GESTIONE FILE ----------------//
typedef struct file {
    char path[PATH_MAX];            //nome
    char* data;                     //contenuto file
    node* client_open;              //lista client che lo hanno aperto

    int lock_flag;                  //variabile che mi tiene conto se un file è in lock da parte di un client
    node* testa_lock;               //lista che mi tiene conto dei client in testa per la lock
    node* coda_lock;                //lista che mi tiene conto dei client in coda per la lock

    int client_write;               //FILE DESCRIPTOR DEL CLIENT CHE HA ESEGUITO COME ULTIMA OPERAZIONE UNA openFile con flag O_CREATE
    struct file * next;
} file;
//----------------------------------------------//


//-------------------- FUNZIONI SOLO DICHIARATE --------------------//
//Funzioni per gestire le richieste
void insertNode (node ** list, int data);
int removeNode (node ** list);
int updatemax(fd_set set, int fdmax);

//Funzioni utilizzate dal server per gestire le richieste
int aggiungiFile(char* path, int flag, int cfd);
int rimuoviCliente(char* path, int cfd);
int rimuoviFile(char* path, int cfd);
int inserisciDati(char* path, char* data, int size, int cfd);
int appendDati(char* path, char* data, int size, int cfd);
int bloccaFile(char* path, int cfd);
int sbloccaFile(char* path, int cfd);
char* prendiFile (char* path, int cfd);
file* lastFile(file** temp);

//Funzioni di stampa
void printFile (file* cache);
void printClient (node * list);

//Funzioni per liberare la memoria
void freeList(node** head);
void freeCache (file* cache);
int fileOpen(node* list, int cfd);

void* Workers(void *argument);
void execute (char * request, int cfd,int pfd);

//Funzioni per gestire la lock e la unlock di un file
void push(node **testa, node **coda, int data);
int pop(node **testa, node **coda);
int find(node** testa, int data);
int free_L(node** testa, node** coda);

//----------------------------------------------------------------//

#endif /* serverFunction_h */
