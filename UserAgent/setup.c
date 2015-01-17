/*
Kelvin Xuan Zou

Princeotn university

This is the user space agent of the middlebox protocol
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h> 
#include <asm/types.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <asm/types.h>

#define UDP_PORT 1025

#define THREAD_NUM 100

int main(int argc, char**argv){
    if (argc<2){
    
        struct timeval t1, t2;
        double elapsedTime;
        int sockfd;
        sockfd=socket(AF_INET,SOCK_DGRAM,0);
        struct sockaddr_in servaddr;
        bzero(&servaddr,sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
        servaddr.sin_port=htons(UDP_PORT);
        bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

        
        char mesg[4];
        struct sockaddr_in clientAddressPtr;
        socklen_t len = sizeof(struct sockaddr_in) ;
       
        recvfrom(sockfd,mesg,4,0,(struct sockaddr *) & clientAddressPtr,&len);
        gettimeofday(&t1, NULL);
        
        sendto(sockfd, mesg, 4, 0 , (struct sockaddr *)&clientAddressPtr,sizeof(struct sockaddr_in ));
        
        recvfrom(sockfd,mesg,4,0,(struct sockaddr *) & clientAddressPtr,&len);
        gettimeofday(&t2, NULL);
        elapsedTime =(t2.tv_usec - t1.tv_usec) + (t2.tv_sec - t1.tv_sec)*1000000;
		printf("Time is %f\n",elapsedTime);     
    
    
    } else if(argc <3) {
       
    
        struct timeval t1, t2;
        double elapsedTime;
        gettimeofday(&t1, NULL);
        int sockfd;
        sockfd=socket(AF_INET,SOCK_DGRAM,0);
        struct sockaddr_in servaddr;
        bzero(&servaddr,sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
        servaddr.sin_port=htons(UDP_PORT);
        bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

        char mesg[4];
        struct sockaddr_in clientAddressPtr;
        socklen_t len = sizeof(struct sockaddr_in) ;
        
        int * counter = (int *) mesg;
        printf("counter is %d\n", *counter);
        
        struct sockaddr_in SendServaddr, SendCliaddr;
        
        struct in_addr addr;
        
        int SendSockfd ;
        
        SendSockfd=socket(AF_INET,SOCK_DGRAM,0);
        bzero(&SendServaddr,sizeof(SendServaddr));
        printf("The SendSockfd is %d \n", SendSockfd);

        SendServaddr.sin_family = AF_INET;
        
        
        inet_aton(argv[1], &addr);
        
        char * IPStr = inet_ntoa(addr);
        
        
        printf("the destination is %s\n", IPStr);
        SendServaddr.sin_addr.s_addr=inet_addr(IPStr); 

        SendServaddr.sin_port=htons(UDP_PORT);
        recvfrom(sockfd,mesg,4,0,(struct sockaddr *) & clientAddressPtr,&len);
        
        sendto(SendSockfd,mesg,4,0,(struct sockaddr *)&SendServaddr,sizeof(struct sockaddr_in ));
        recvfrom(SendSockfd,mesg,4,0,NULL,NULL);
        
        sendto(sockfd, mesg, 4, 0 , (struct sockaddr *)&clientAddressPtr,sizeof(struct sockaddr_in ));
        recvfrom(sockfd,mesg,4,0,(struct sockaddr *) & clientAddressPtr,&len);
        
        sendto(SendSockfd,mesg,4,0,(struct sockaddr *)&SendServaddr,sizeof(struct sockaddr_in ));
            
    } 
    else {
        int sockfd,n;
	    struct sockaddr_in servaddr;
        struct timeval t1, t2;
	    sockfd=socket(AF_INET,SOCK_DGRAM,0);

	    bzero(&servaddr,sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_addr.s_addr=inet_addr(argv[1]);
	    servaddr.sin_port=htons(UDP_PORT);

	    double elapsedTime;
	    int flag =0;

	    //blocking way, it measures the latency in a better way!
	    char sendline[4];
	    char recvline[4];
	    memset(sendline, 0,4);
	    gettimeofday(&t1, NULL);
	    sendto(sockfd,sendline,4,0,(struct sockaddr *)&servaddr,sizeof(struct sockaddr_in ));
        recvfrom(sockfd,recvline,4,0,NULL,NULL);
        sendto(sockfd,sendline,4,0,(struct sockaddr *)&servaddr,sizeof(struct sockaddr_in ));
        gettimeofday(&t2, NULL);
        elapsedTime =(t2.tv_usec - t1.tv_usec) + (t2.tv_sec - t1.tv_sec)*1000000;
		printf("Time is %f\n",elapsedTime);
    }
    
    
    
    
    
}
