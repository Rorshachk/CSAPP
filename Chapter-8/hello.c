#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(){
    fork();
    fork();
    printf("hello\n");
    fork();
}