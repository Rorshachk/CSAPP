#include "csapp.h"


int main(int argc, char const *argv[])
{
    struct in_addr s;
    char IPDotDec[20];
    if(argc == 1){
        printf("Please Enter the IP address:");
        scanf("%s", IPDotDec);
    }
    else sscanf(argv[1], "%s", IPDotDec);
    inet_pton(AF_INET, IPDotDec, (void *)&s);
    printf("0x%x\n", htonl(s.s_addr));
    return 0;
}