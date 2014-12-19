#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/time.h>

#define BUFLEN 4
#define NPACK 1000000
#define PORT 5001


void diep(char *s){
    perror(s);
    exit(1);
}

int main(int argc, char**argv) {
    struct timeval t1, t2;
    struct sockaddr_in saddr, daddr; 
    int fd, i;
    char buf[BUFLEN];
    if ((fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    {  diep("socket");
    }
    bzero(&saddr,sizeof(saddr));    
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT);
    saddr.sin_addr.s_addr = inet_addr(argv[1]);

    socklen_t size = sizeof(struct sockaddr_in );
    gettimeofday(&t1, NULL);   
    for (i=0; i<NPACK; i++){
        //printf("sending packets\n");
        if (sendto(fd, buf, BUFLEN, 0, (struct sockaddr *)  &saddr, size) ==-1 ){
            diep("send to error");
        } 
    }
     gettimeofday(&t2, NULL);
    double elapsedTime =(t2.tv_sec - t1.tv_sec)*1000000.0;
     elapsedTime +=(t2.tv_usec-t1.tv_usec);
    double packetrate = (NPACK*1.0)/(elapsedTime);
    printf("\npacket rate is %f\n", packetrate);
    close (fd);
    return 0;
    
}
