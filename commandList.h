#ifndef commandList_h
#define commandList_h

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"

typedef struct node {
    char * cmd;
    char * arg;
    struct node * next;
} node;


void addList(node ** list,char * cmd,char * arg);
void printList(node * list);
int searchCommand(node ** list, char * cmd, char ** arg);
void freeList(node ** list);


#endif /* commandList_h */
