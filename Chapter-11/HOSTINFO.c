#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char const *argv[])
{
    struct addrinfo *p, *listp, hints;
    char buf[2048];
    int rc, flags;

    if(argc != 2){
        return 0;
    }

    memset(&hints, 0, sizeof(&hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if((rc = getaddrinfo(argv[1], NULL, &hints, &listp)) != 0){
        puts("Connection error");
         return 0;
    }

    flags = NI_NUMERICHOST;
    for(p = listp; p; p = p->ai_next){
        getnameinfo(p->ai_addr, p->ai_addrlen, buf, 2048, NULL, 0, flags);
        puts(buf);
        /*
        struct sockaddr_in *sockup;
        sockup = (struct sockaddr_in *)p->ai_addr;
        inet_ntop(AF_INET, &(sockup->sin_addr), buf, 2048);
        puts(buf);*/
    }

    freeaddrinfo(listp);
    return 0;
}
