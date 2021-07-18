#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "util.h"


int main(int argc, char* argv[]){
  char opt;

  while((opt = getopt(argc, argv, "hpf:w:W:r:R:d:D:t:c:l:u:")) != -1){
    swith(opt){
      case 'h':
      case 'p':
      case 'f':
      case 'w':
      case 'W':
      case 'r':
      case 'R':
      case 'd':
      case 'D':
      case 't':
      case 'l':
      case 'u':
      case 'c':
      case '?':
      case ':':
      default;
    }
  }
  
  return 0;
}
