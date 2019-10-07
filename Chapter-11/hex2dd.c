#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
int main(int argc, char const *argv[])
{
	struct in_addr s;
	char IPDotDec[20];
	if(argc == 1){
		printf("Please enter the address: ");
		scanf("0x%x", &s.s_addr);
	}
	else sscanf(argv[1], "0x%x", &s.s_addr);
	s.s_addr = htonl(s.s_addr);
	inet_ntop(AF_INET, (void*)&s, IPDotDec, 16);
	puts(IPDotDec);
	return 0;
}
