#ifndef parsingFile_h
#define parsingFile_h

#include "util.h"

#define SOCKNAME "socket_name"
#define FILE_LOG "log.txt"

typedef struct config_File{
    int num_thread;                         //numero thread
    size_t sizeBuff;                        //spazio di memoria
    unsigned int num_files;                 //numero massimo dei file possibili
    char *socket_name;                      //nome della socket
    char* fileLog;                          //nome del file di log
} config;

long isNumberParser(const char* s);
config* getConfig(const char* string);
config* default_configuration(void);
void stampa(config* configuration);
int freeConfig(config* configuration);

#endif /* parsingFile_h */
