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
#include <queue>
#include <string>
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
#include <linux/genetlink.h>



#define NETLINK_USER 31

#define MAX_PAYLOAD 1400 /* maximum payload size*/

#define UDP_PORT 1025

#define THREAD_NUM 100
#define NETLINK_LOSSFREE false 
#define NETLINK_FLAG true

#define RETRANSMIT_TIMER 100000 //minimum is 1000 since poll only supports down to 1 ms

using namespace std;

struct timeval t1, t2;
double  elapsedTime;
typedef struct header {
    int action;
    int MboxLength;
} header;

int set_packet( char * writeBuffer, char ** MBox, int MListLen){
	header * hdr_ptr = (header*) writeBuffer;	
    hdr_ptr->MboxLength = MListLen; 
    int ByteStreamCount = sizeof(header) +  4* (hdr_ptr->MboxLength + 1 ) ;
	
    struct in_addr addr;
    int i = 0;
    for (i=0; i< hdr_ptr->MboxLength ; i++ ){
		inet_aton(MBox[i], &addr);
		memcpy( writeBuffer + sizeof(header) +  4*i, &addr.s_addr, 4);
    } 
    for (i = 0; i<hdr_ptr->MboxLength ; i++){
        struct in_addr addr = *(struct in_addr*) ( writeBuffer + i*4 +   sizeof(header));
        printf("middlebox list is %s\n",inet_ntoa(addr));
    }
   //sendto(fd,writeBuffer,ByteStreamCount,0,(struct sockaddr *)servaddr,sizeof(struct sockaddr_in ));
    return ByteStreamCount;
}



//this is the main function, it starts from the 

int main(int argc, char**argv)
{
	if (argc < 2) {
	    printf("Provide with the type of NF instance you are running, 1 means client, 4 means server, 2 means new NF, and 3 means the old NF\n");
	    exit(1);
	}
	
	if (atoi(argv[1])==1 ){
		printf("Left Anchor!\n");
		
		char * NewMBox[] = { "10.0.0.2",  "10.0.0.4"};

		char sendline[MAX_PAYLOAD], recvline[MAX_PAYLOAD];
		memset(sendline, 0, MAX_PAYLOAD);
		memset(recvline, 0, MAX_PAYLOAD);

		int headersize = set_packet(sendline, NewMBox,( sizeof (NewMBox) / sizeof(char *) ));
		header * hdr_ptr = (header*) sendline;	
		//syn packet
		hdr_ptr->action =1;

		struct sockaddr_in servaddr;
		int sockfd=socket(AF_INET,SOCK_DGRAM,0);
		bzero(&servaddr,sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port=htons(UDP_PORT);

		
		struct in_addr addr = *(struct in_addr*)( sendline +sizeof(header)) ;
		char * IPStr = inet_ntoa(addr);
        printf("middlebox address for receive from is %s\n", IPStr);
		servaddr.sin_addr.s_addr = inet_addr(IPStr);
		gettimeofday(&t1, NULL);
		sendto(sockfd, sendline, headersize, 0, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in));
		recvfrom(sockfd,recvline,MAX_PAYLOAD,0,NULL,NULL);
		
		hdr_ptr = (header*) recvline;

		//receive SYN-ACK packet
		if(hdr_ptr->action == 2){
			gettimeofday(&t2, NULL);
	    	elapsedTime =(t2.tv_usec - t1.tv_usec) + (t2.tv_sec - t1.tv_sec)*1000000;
	    	printf("Loss_free update SYN-ACK Time is %f\n",elapsedTime);

			printf("new path is established and start buffering packets\n");
		}


		/***************************************************
		this marks the end of setup new path, 
		and now we are transitioning to the old path
		*****************************************************/
		struct sockaddr_in servaddr_oldpath;
		int sockfd_oldpath=socket(AF_INET,SOCK_DGRAM,0);
		bzero(&servaddr_oldpath,sizeof(servaddr_oldpath));
		servaddr_oldpath.sin_family = AF_INET;
		servaddr_oldpath.sin_port=htons(UDP_PORT);


		char * OldMBox[] = { "10.0.0.3",  "10.0.0.4"};

		headersize = set_packet(sendline, OldMBox,( sizeof (OldMBox) / sizeof(char *) ));
		hdr_ptr = (header*) sendline;	
		//Fin packet
		hdr_ptr->action =4;
		
		addr = *(struct in_addr*)( sendline +sizeof(header)) ;
		IPStr = inet_ntoa(addr);
        printf("middlebox address for receive from is %s\n", IPStr);
		servaddr_oldpath.sin_addr.s_addr = inet_addr(IPStr);

		sendto(sockfd_oldpath, sendline, headersize, 0, (struct sockaddr *)&servaddr_oldpath, sizeof(struct sockaddr_in));
		recvfrom(sockfd_oldpath,recvline,MAX_PAYLOAD,0,NULL,NULL);

		hdr_ptr = (header*) recvline;
		//receive FIN-ACK packet
		if(hdr_ptr->action == 5){
			printf("Old path is closed\n");
		}

		recvfrom(sockfd_oldpath,recvline,MAX_PAYLOAD,0,NULL,NULL);

		hdr_ptr = (header*) recvline;
		//receive FIN-ACK packet
		if(hdr_ptr->action == 9){
			gettimeofday(&t2, NULL);
		    elapsedTime =(t2.tv_usec - t1.tv_usec) + (t2.tv_sec - t1.tv_sec)*1000000;
		    printf("NF state-enabled update SYN-ACK Time is %f\n",elapsedTime);
			
			printf("NF state migration is complete and resume data transfer\n");
		}

		return 0;


	}


	else if (atoi(argv[1])==4 ){
		printf("Right Anchor!\n");

		struct sockaddr_in servaddr;
		int sockfd=socket(AF_INET,SOCK_DGRAM,0);
	    bzero(&servaddr,sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	    servaddr.sin_port=htons(UDP_PORT);
	    bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

		struct sockaddr_in clientAddressPtr;
		socklen_t len = sizeof(struct sockaddr_in) ;
        char sendline[MAX_PAYLOAD], recvline[MAX_PAYLOAD];
		memset(sendline, 0, MAX_PAYLOAD);
		memset(recvline, 0, MAX_PAYLOAD);

        recvfrom(sockfd,recvline, MAX_PAYLOAD,0,(struct sockaddr *) &clientAddressPtr,&len);
        header * hdr_ptr = (header*) recvline;	
        if(hdr_ptr->action == 1 && hdr_ptr->MboxLength==1){
			printf("See SYN\n");
		}

		hdr_ptr = (header*) sendline;	

		hdr_ptr->action = 2;
		sendto(sockfd, sendline, 8, 0, (struct sockaddr *) &clientAddressPtr, sizeof(struct sockaddr_in) );

		//wait for FIN from the old path
        recvfrom(sockfd,recvline, MAX_PAYLOAD,0,(struct sockaddr *) &clientAddressPtr,&len);
        hdr_ptr = (header*) recvline;	
        if(hdr_ptr->action == 4 && hdr_ptr->MboxLength==1){
			printf("See FIN\n");
		}

		hdr_ptr = (header*) sendline;	

		hdr_ptr->action = 5;
		sendto(sockfd, sendline, 8, 0, (struct sockaddr *) &clientAddressPtr, sizeof(struct sockaddr_in) );

		
	}

	else if (atoi(argv[1])==2 ){
		printf("New path Mid point!\n");
		
		struct sockaddr_in servaddr, SendServaddr;
		int sockfd=socket(AF_INET,SOCK_DGRAM,0);
	    bzero(&servaddr,sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	    servaddr.sin_port=htons(UDP_PORT);
	    bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));


		struct sockaddr_in clientAddressPtr;
		socklen_t len = sizeof(struct sockaddr_in) ;
        char sendline[MAX_PAYLOAD], recvline[MAX_PAYLOAD];
		memset(sendline, 0, MAX_PAYLOAD);
		memset(recvline, 0, MAX_PAYLOAD);

        int n = recvfrom(sockfd,recvline, MAX_PAYLOAD,0,(struct sockaddr *) &clientAddressPtr,&len);
        
		header * hdr_ptr = (header*) recvline;
        if(hdr_ptr->action == 1 && hdr_ptr->MboxLength>1){
			printf("Relay SYN\n");
			//servaddr.sin_addr.s_addr = inet_addr(IPStr);
			memcpy(sendline, recvline, sizeof(header) );
			memcpy(sendline + sizeof(header), recvline + sizeof(header)+4, n -sizeof(header)-4);

			header * hdr_ptr = (header*) sendline;	
			struct in_addr addr = *(struct in_addr*)( sendline +sizeof(header) ) ;
			char * IPStr = inet_ntoa(addr);
        	printf("middlebox address for the next hop is %s\n", IPStr);

        	//create a new socket for the next hop!
			int RelaySockfd ;
			RelaySockfd=socket(AF_INET,SOCK_DGRAM,0);
		    bzero(&SendServaddr,sizeof(SendServaddr));
		    printf("The RelaySockfd is %d \n", RelaySockfd);

		    SendServaddr.sin_family = AF_INET;
		    SendServaddr.sin_addr.s_addr=inet_addr(IPStr); 
		    SendServaddr.sin_port=htons(UDP_PORT);
			sendto(RelaySockfd, sendline, n-4, 0, (struct sockaddr *)&SendServaddr, sizeof(struct sockaddr_in));
			
			//receive ACK, and relay it
			recvfrom(RelaySockfd,recvline,8,0,NULL,NULL);
			sendto(sockfd, recvline, 8, 0, (struct sockaddr *)&clientAddressPtr, sizeof(struct sockaddr_in));


			//receive from old Middlebox the state migration request
			struct sockaddr_in OldNFAddr;

			recvfrom(sockfd,recvline, MAX_PAYLOAD,0,(struct sockaddr *) &OldNFAddr,&len);
			hdr_ptr = (header*) recvline;
			if(hdr_ptr->action ==7){
				printf("Middlebox State Migration started\n");
				hdr_ptr = (header*) sendline;
				hdr_ptr->action=8;
				sendto(sockfd, sendline, 8, 0, (struct sockaddr *)&OldNFAddr, sizeof(struct sockaddr_in));
				

			}


		}


	}


	else if (atoi(argv[1])==3 ){
		printf("Old path mid point!\n");

		struct sockaddr_in servaddr, SendServaddr;
		int sockfd=socket(AF_INET,SOCK_DGRAM,0);
	    bzero(&servaddr,sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	    servaddr.sin_port=htons(UDP_PORT);
	    bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));


		struct sockaddr_in clientAddressPtr;
		socklen_t len = sizeof(struct sockaddr_in) ;
        char sendline[MAX_PAYLOAD], recvline[MAX_PAYLOAD];
		memset(sendline, 0, MAX_PAYLOAD);
		memset(recvline, 0, MAX_PAYLOAD);

        int n = recvfrom(sockfd,recvline, MAX_PAYLOAD,0,(struct sockaddr *) &clientAddressPtr,&len);
        
		header * hdr_ptr = (header*) recvline;
        if(hdr_ptr->action == 4 && hdr_ptr->MboxLength>1){
			printf("Relay FIN\n");
			//servaddr.sin_addr.s_addr = inet_addr(IPStr);
			memcpy(sendline, recvline, sizeof(header) );
			memcpy(sendline + sizeof(header), recvline + sizeof(header)+4, n -sizeof(header)-4);

			header * hdr_ptr = (header*) sendline;	
			struct in_addr addr = *(struct in_addr*)( sendline +sizeof(header) ) ;
			char * IPStr = inet_ntoa(addr);
        	printf("middlebox address for the next hop is %s\n", IPStr);

        	//create a new socket for the next hop!
			int RelaySockfd ;
			RelaySockfd=socket(AF_INET,SOCK_DGRAM,0);
		    bzero(&SendServaddr,sizeof(SendServaddr));
		    printf("The RelaySockfd is %d \n", RelaySockfd);

		    SendServaddr.sin_family = AF_INET;
		    SendServaddr.sin_addr.s_addr=inet_addr(IPStr); 
		    SendServaddr.sin_port=htons(UDP_PORT);
			sendto(RelaySockfd, sendline, n-4, 0, (struct sockaddr *)&SendServaddr, sizeof(struct sockaddr_in));
			
			//receive ACK, and relay it
			recvfrom(RelaySockfd,recvline,8,0,NULL,NULL);
			sendto(sockfd, recvline, 8, 0, (struct sockaddr *)&clientAddressPtr, sizeof(struct sockaddr_in));


			//do state migration from here and on!
			printf("Middlebox State Migration start request\n");
			hdr_ptr = (header*) sendline;
			hdr_ptr->action=7;
			/**********************************************
			This is another place you need to configure where the new Middlebox instance IP is 
			************************************************/
			SendServaddr.sin_addr.s_addr=inet_addr("10.0.0.2"); 
			sendto(RelaySockfd, sendline, 8, 0, (struct sockaddr *)&SendServaddr, sizeof(struct sockaddr_in));
			recvfrom(RelaySockfd,recvline,8,0,NULL,NULL);
			hdr_ptr = (header*) recvline;
			if(hdr_ptr->action==8){
				printf("Middlebox State Migration completed\n");
			}
			//sending back to the Host, 
			hdr_ptr = (header*) sendline;
			hdr_ptr->action=9;
			sendto(sockfd, sendline, 8, 0, (struct sockaddr *)&clientAddressPtr, sizeof(struct sockaddr_in));


		}
	}


	return 0;
}