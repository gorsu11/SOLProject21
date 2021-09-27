#ifndef log_h
#define log_h

#include "util.h"

#define CONTROLLA(function)\
    if(function == -1) {\
        exit(errno);\
    }

void writeLogFd(FILE* log, int cfd);
char* getTime(void);

void valutaEsito(FILE* log, int value, char* text);

#endif /* log_h */
