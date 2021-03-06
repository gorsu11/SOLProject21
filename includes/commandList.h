#ifndef commandlist_h
#define commandlist_h

#include "util.h"

typedef struct node {
    char * cmd;                 //comando passato
    char * arg;                 //argomento passato
    struct node * next;
} node;

void addList(node ** list,char * cmd,char * arg);
void printList(node * list);
int containCMD(node ** list, char * cmd, char ** arg);
void freeList(node ** list);

#endif /* commandlist_h */
