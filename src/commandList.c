#include "../includes/commandList.h"

//Funzione che aggiunge alla lita un comando con il suo argomento
void addList (node** list,char* cmd, char* arg) {
    node* new;
    CHECKNULL(new, malloc(sizeof(node)), "malloc");
    CHECKNULL(new->cmd, malloc(sizeof(cmd)), "malloc");

    strcpy(new->cmd,cmd);
    if (arg != NULL) {
        CHECKNULL(new->arg, malloc(PATH_MAX), "malloc");
        strcpy(new->arg,arg);
    }
    else new->arg = NULL;
    new->next = NULL;

    node* curr = *list;

    if (*list == NULL) {
        *list = new;
        return;
    }

    while (curr->next!=NULL) {
        curr = curr->next;
    }
    curr->next = new;
}

//Funzione che stampa gli argomenti della lista
void printList (node* list) {
    node* curr = list;
    while(curr!=NULL) {
        printf("CMD = %s ARG = %s \n",curr->cmd,curr->arg);
        curr = curr->next;
    }
}

//Funzione che cerca il comando all'interno della lista
//Ritorna 1 se il comando è stato trovato, 0 altrimenti
int containCMD (node ** list, char * cmd, char ** arg) {

    node * curr = *list;
    node * prec = NULL;
    int trovato = 0;

    while (curr!=NULL && trovato==0) {
        if (strcmp(curr->cmd,cmd)==0) trovato=1;
        else {
            prec = curr;
            curr = curr->next;
        }
    }
    if (trovato==1) {
        if (curr->arg!=NULL) {
            char arg [strlen(curr->arg)];
            strcpy(arg,curr->arg);
        } else *arg = NULL;
        if (prec == NULL) {
            node * tmp = *list;
            *list = (*list)->next;
            free(tmp->arg);
            free(tmp->cmd);
            free(tmp);
        }else{
            prec->next = curr->next;
            free(curr->arg);
            free(curr->cmd);
            free(curr);
        }
    }

    return trovato;

}

//Funzione che libera la lista
void freeList (node** list) {
    node* tmp;
    while (*list!=NULL) {
        tmp = *list;
        free((*list)->arg);
        free((*list)->cmd);
        (*list) = (*list)->next;
        free(tmp);
    }

}
