#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"

#define MAX_PATH_LENGTH 1024

typedef struct node {
    char * cmd;
    char * arg;
    struct node * next;
} node;

//RETURN 1 SSE LIST CONTIENE IL COMANDO CMD (LO RIMUOVE DALLA LISTA), 0 ALTRIMENTI -- SE LO TROVA INSERISCE L'ARGOMENTO IN ARG SE PRESENTE, NULL ALTRIMENTI

void addList (node ** list,char * cmd,char * arg) {
    node* new;
    CHECKNULL(new, malloc(sizeof(node)), "malloc");
    CHECKNULL(new->cmd, malloc(sizeof(cmd)), "malloc");

    strcpy(new->cmd,cmd);
    if (arg != NULL) {
        CHECKNULL(new->arg, malloc(MAX_PATH_LENGTH), "malloc");
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

void printList (node * list) {
    node * curr = list;
    while (curr!=NULL) {
        printf("CMD = %s ARG = %s \n",curr->cmd,curr->arg);
        curr = curr->next;
    }
}

int searchCommand (node ** list, char * cmd, char ** arg) {

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

void freeList (node ** list) {
    node * tmp;
    while (*list!=NULL) {
        tmp = *list;
        free((*list)->arg);
        free((*list)->cmd);
        (*list)=(*list)->next;
        free(tmp);
    }

}
