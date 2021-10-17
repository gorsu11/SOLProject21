#include "../includes/serverFunction.h"

//-------------------- FUNZIONI DI UTILITY PER LA LOCK E UNLOCK --------------------//

//Funzione che aggiunge elemento in coda alle due liste
//Ritorna errore se l'elemento da inserire è NULL
void push(node **testa, node **coda, int data){
    node* new;
    CHECKNULL(new, malloc(sizeof(node)), "malloc push");
    if(new != NULL){
        new->data = data;
        new->next = NULL;

        if(testa == NULL){
            *testa = new;
            *coda = new;
        }
        else{
            (*coda)->next = new;
            *coda = new;
        }
    }
    else{
        fprintf(stderr, "Memoria esaurita");
        exit(EXIT_FAILURE);
    }
}

//Funzione che rimuove in testa l'elemento della lista
//Ritorna il valore dell'elemento se va a buon fine, altrimenti ritorna un errore
int pop(node **testa, node **coda){
    if(*testa != NULL){
        int value = (*testa)->data;
        node *temp = *testa;
        *testa = (*testa)->next;
        if(*testa == NULL){
            *coda = NULL;
        }
        free(temp);
        return value;
    }
    else{
        fprintf(stderr, "La lista è vuota\n");
        exit(EXIT_FAILURE);
    }
}

//Funzione che ricerca un elemento in una lista
//Ritorna 1 se lo trova, 0 altrimenti
int find(node** testa, int data){
    node* curr = *testa;
    while(curr != NULL){
        if(curr->data == data){
            return 1;
        }
        else{
            curr = curr->next;
        }
    }
    return 0;
}
//--------------------------------------------------------------------------//

//-------------------- FUNZIONI DI UTILITY PER LA CACHE --------------------//

//Stampa la lista dei file presenti sulla cache
void printFile (file* cache) {
    printf ("Lista File : \n");
    fflush(stdout);
    file * curr = cache;
    while (curr != NULL) {
        printf("%s ",curr->path);
        if (curr->data!=NULL) {
            printf("size = %ld, locked = %d\n", strlen(curr->data), curr->lock_flag);
        } else {
            printf("size = 0, locked = %d\n", curr->lock_flag);
        }

        curr = curr->next;
    }
}

//Funzione che libera la lista
void freeList(node ** head) {
    node* temp;
    node* curr = *head;
    while (curr != NULL) {
        temp = curr;
        curr = curr->next;
        free(temp);
    }
    *head = NULL;
}

//Funzione che cerca all'interno della lista un valore
//Ritorna 1 se presente, 0 altrimenti
int fileOpen(node* list, int cfd) {
    node* curr = list;
    while (curr != NULL) {
        if (curr->data == cfd){
            return 1;
        }
    }
    return 0;
}

//Stampa la lista dei client che hanno aperto un file
void printClient (node * list) {
    node * curr = list;
    printf("APERTO DA : ");
    fflush(stdout);
    while (curr!=NULL) {
        printf("%d ",curr->data);
        fflush(stdout);
        curr = curr->next;
    }
    printf("\n");
}

//Libera la memoria da tutti i file presenti nella cache
void freeCache (file* cache) {
    file * tmp;
    file * curr = cache;
    while (curr != NULL) {
        tmp=curr;
        freeList(&(curr->client_open));
        free(curr->data);
        curr = curr->next;
        free(tmp);
    }
    cache = NULL;
}
//--------------------------------------------------------------------------//
