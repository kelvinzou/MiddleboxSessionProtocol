#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cstdlib>

#define BUFLEN 4
#define NPACK 9900000
#define PORT 5001

void diep(char *s){
    perror(s);
    exit(1);
}

int main(int argc, char**argv) {
    if(argc<3){
        printf("Please type down [port number] [packet size]!\n");
        return 0;
    }
    int buffer_len = atoi(argv[2]);
    int port_number = atoi(argv[1]);
    struct sockaddr_in saddr, daddr; 
    int fd, i;
    char buf[buffer_len];
    if ((fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    {  diep("socket");
    }
    bzero(&saddr,sizeof(saddr));    
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port_number);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *) &saddr, sizeof (struct sockaddr_in ) )==-1  ){
        diep("BINDING");
    }
    socklen_t size = sizeof(struct sockaddr_in );
    
    while(1){
        
        for (i=0; i<NPACK; i++){
            if (recvfrom(fd, buf, buffer_len, 0, (struct sockaddr *)  &daddr, &size) ==-1 ){
                diep("RECVFROM error");
                } 
            }
        printf("receive packet\n");
    }
    
    close (fd);
    return 0;
    
}
