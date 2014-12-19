#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#define BUFLEN 2
#define NPACK 10
#define PORT 5001


void diep(char *s){
    perror(s);
    exit(1);
}

int main(int argc, char**argv) {
    struct sockaddr_in saddr, daddr; 
    int fd, i;
    char buf[BUFLEN];
    if ((fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    {  diep("socket");
    }
    bzero(&saddr,sizeof(saddr));    
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (inet_aton (argv[1], & daddr.sin_addr) ==0 ) {
        diep("binding");
    }

    socklen_t size = sizeof(struct sockaddr_in );

    for (i=0; i<NPACK; i++){
        printf("sending packets\n");
        if (sendto(fd, buf, BUFLEN, 0, (struct sockaddr *)  &daddr, size) ==-1 ){
            diep("RECVFROM error");
        } 
    }
    close (fd);
    return 0;
    
}
