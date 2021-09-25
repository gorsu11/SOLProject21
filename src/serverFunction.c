#include "../includes/serverFunction.h"

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



void rwLock_start(lockFile* file){
    file->waiting ++;
    if(DEBUGSERVER) printf("[SERVER] I clienti in attesa sono %d e writer_active è %d\n", file->waiting, file->writer_active);
    while (file->waiting > 1 || file->writer_active == true) {
        if(DEBUGSERVER) printf("[SERVER] Entra in WAIT\n");
        WAIT(&(file->cond_file), &(file->mutex_file));
    }
    file->writer_active = true;
}

void rwLock_end(lockFile* file){
    //printf("%d\n", file->waiting);
    file->waiting --;
    file->writer_active = false;
    //printf("%d\n", file->waiting);
    BCAST(&(file->cond_file));
}



int setLock(file* curr, int cfd){
    if (curr->lock_flag == -1){
        curr->lock_flag = cfd;
        return 0;
    }
    else if(curr->lock_flag != cfd){
        if(find(&(curr->testa_lock), cfd) == 0){
            push(&(curr->testa_lock), &(curr->coda_lock), cfd);
            return -1;
        }
        else{
            //se l'elemento è gia in coda
            return -2;
        }
    }
    return 0;
}

int setUnlock(file* curr, int cfd){
    if(curr->testa_lock == NULL){
        curr->lock_flag = -1;
        return 0;
    }
    else{
        curr->lock_flag = pop(&(curr->testa_lock), &(curr->coda_lock));
        writen((long)curr->coda_lock, 0, sizeof(int));
        return -1;
    }
}

//-------------------- FUNZIONI DI UTILITY PER LA CACHE --------------------//

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

int fileOpen(node* list, int cfd) {
    node* curr = list;
    while (curr != NULL) {
        if (curr->data == cfd){
            return 1;
        }
    }
    return 0;
}

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

int mkdir_p(const char *path) {
    if(DEBUGSERVER) printf("[SERVER] ******************************************************************\n");
    if(DEBUGSERVER) printf("[SERVER] Entra in mkdir_p con path %s\n", path);
    const size_t len = strlen(path);
    char _path[1024];
    char *p;

    errno = 0;

    if (len > sizeof(_path)-1) {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(_path, path);

    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';

            if (mkdir(_path, S_IRWXU) != 0) {
                if (errno != EEXIST)
                    return -1;
            }

            *p = '/';
        }
    }

    if (mkdir(_path, S_IRWXU) != 0) {
        if (errno != EEXIST)
            return -1;
    }
    if(DEBUGSERVER) printf("[SERVER] ******************************************************************\n");
    return 0;
}
