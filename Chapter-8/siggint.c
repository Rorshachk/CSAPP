#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

void sigint_handler(int sig){
    printf("Caught SIGINT!\n");
    exit(0);
}

void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}


int main(){
    if(signal(SIGINT, sigint_handler) == SIG_ERR)
      unix_error("signal error!");
    
    pause();

    return 0;
}