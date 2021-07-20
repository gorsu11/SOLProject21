//
//  util.h
//  PSOL21
//
//  Created by Gianluca Orsucci on 05/07/21.
//


//TODO: importare qui tutte le librerie
#ifndef util_h
#define util_h

#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>

// utility syscall socket
#define SYSCALL_WRITE(c,e) \
    if (c==-1) { \
        perror(e); \
        int fine=-1; \
        write(pfd,&cfd,sizeof(cfd)); \
        write(pfd,&fine,sizeof(fine)); \
        return; \
    }
#define SYSCALL_READ(s,c,e) \
    s = c; \
    if (s==0 || s==-1) { \
        perror(e); \
        int fine=-1; \
        write(pfd,&cfd,sizeof(cfd)); \
        write(pfd,&fine,sizeof(fine)); \
        return; \
    }


#define CHECKNULL(r,c,e) CHECK_EQ_EXIT(e, (r=c), NULL,e,"")

// utility macro pthread
#define SYSCALL_PTHREAD(e,c,s) \
    if((e=c)!=0) { errno=e;perror(s);fflush(stdout);exit(EXIT_FAILURE); }

#define UNIX_PATH_MAX 104 /* man 7 unix */


//-------------- DEFINE DEI DEBUG CHE UTILIZZO PER SEMPLIFICARE E PER POTER STAMPARE ---------------//
//variabile globale che se impostata mi permette di effettuare debugging
#if !defined(DEBUGAPI)
#define DEBUGAPI 0
#endif

#if !defined(DEBUGSERVER)
#define DEBUGSERVER 0
#endif

#if !defined(DEBUGCLIENT)
#define DEBUGCLIENT 0
#endif
//-------------------------------------------------------------//


#if !defined(BUFSIZE)
#define BUFSIZE 256
#endif

#if !defined(EXTRA_LEN_PRINT_ERROR)
#define EXTRA_LEN_PRINT_ERROR   512
#endif

#define SYSCALL_EXIT(name, r, sc, str, ...)    \
    if ((r=sc) == -1) {                \
    perror(#name);                \
    int errno_copy = errno;            \
    print_error(str, __VA_ARGS__);        \
    exit(errno_copy);            \
    }

#define SYSCALL_PRINT(name, r, sc, str, ...)    \
    if ((r=sc) == -1) {                \
    perror(#name);                \
    int errno_copy = errno;            \
    print_error(str, __VA_ARGS__);        \
    errno = errno_copy;            \
    }

#define SYSCALL_RETURN(name, r, sc, str, ...)    \
    if ((r=sc) == -1) {                \
    perror(#name);                \
    int errno_copy = errno;            \
    print_error(str, __VA_ARGS__);        \
    errno = errno_copy;            \
    return r;                               \
    }

#define CHECK_EQ_EXIT(name, X, val, str, ...)    \
    if ((X)==val) {                \
        perror(#name);                \
    int errno_copy = errno;            \
    print_error(str, __VA_ARGS__);        \
    exit(errno_copy);            \
    }

#define CHECK_NEQ_EXIT(name, X, val, str, ...)    \
    if ((X)!=val) {                \
        perror(#name);                \
    int errno_copy = errno;            \
    print_error(str, __VA_ARGS__);        \
    exit(errno_copy);            \
    }

/**
 * \brief Procedura di utilita' per la stampa degli errori
 *
 */
static inline void print_error(const char * str, ...) {
    const char err[]="ERROR: ";
    va_list argp;
    char * p=(char *)malloc(strlen(str)+strlen(err)+EXTRA_LEN_PRINT_ERROR);
    if (!p) {
    perror("malloc");
        fprintf(stderr,"FATAL ERROR nella funzione 'print_error'\n");
        return;
    }
    strcpy(p,err);
    strcpy(p+strlen(err), str);
    va_start(argp, str);
    vfprintf(stderr, p, argp);
    va_end(argp);
    free(p);
}


/**
 * \brief Controlla se la stringa passata come primo argomento e' un numero.
 * \return  0 ok  1 non e' un numbero   2 overflow/underflow
 */
static inline int isNumber(const char* s, long* n) {
  if (s==NULL) return 1;
  if (strlen(s)==0) return 1;
  char* e = NULL;
  errno=0;
  long val = strtol(s, &e, 10);
  if (errno == ERANGE) return 2;    // overflow/underflow
  if (e != NULL && *e == (char)0) {
    *n = val;
    return 0;   // successo
  }
  return 1;   // non e' un numero
}


long isNumberParser(const char* s) {
   char* e = NULL;
   long val = strtol(s, &e, 0);
   if (e != NULL && *e == (char)0) return val;
   return -1;
}


#define LOCK(l)      if (pthread_mutex_lock(l)!=0)        { \
    fprintf(stderr, "ERRORE FATALE lock\n");            \
    pthread_exit((void*)EXIT_FAILURE);                \
  }
#define UNLOCK(l)    if (pthread_mutex_unlock(l)!=0)      { \
  fprintf(stderr, "ERRORE FATALE unlock\n");            \
  pthread_exit((void*)EXIT_FAILURE);                    \
  }
#define WAIT(c,l)    if (pthread_cond_wait(c,l)!=0)       { \
    fprintf(stderr, "ERRORE FATALE wait\n");            \
    pthread_exit((void*)EXIT_FAILURE);                    \
}
/* ATTENZIONE: t e' un tempo assoluto! */
#define TWAIT(c,l,t) {                            \
    int r=0;                                \
    if ((r=pthread_cond_timedwait(c,l,t))!=0 && r!=ETIMEDOUT) {        \
      fprintf(stderr, "ERRORE FATALE timed wait\n");            \
      pthread_exit((void*)EXIT_FAILURE);                    \
    }                                    \
  }
#define SIGNAL(c)    if (pthread_cond_signal(c)!=0)       {    \
    fprintf(stderr, "ERRORE FATALE signal\n");            \
    pthread_exit((void*)EXIT_FAILURE);                    \
  }
#define BCAST(c)     if (pthread_cond_broadcast(c)!=0)    {        \
    fprintf(stderr, "ERRORE FATALE broadcast\n");            \
    pthread_exit((void*)EXIT_FAILURE);                        \
  }
static inline int TRYLOCK(pthread_mutex_t* l) {
  int r=0;
  if ((r=pthread_mutex_trylock(l))!=0 && r!=EBUSY) {
    fprintf(stderr, "ERRORE FATALE unlock\n");
    pthread_exit((void*)EXIT_FAILURE);
  }
  return r;
}


#endif /* util_h */
