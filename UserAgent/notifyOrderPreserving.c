
/*
Kelvin Xuan Zou

Princeotn university

This is the user space agent of the middlebox protocol
*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h> 
#include <asm/types.h>
#include <poll.h>
#define INTI_TO 10000
#define UDP_PORT 1025
#define SEQUENCENUM 100

struct timeval t1, t2;

typedef struct header{
	int action;
	int sequenceNum;
	int oldMboxLength;
	int newMboxLength;
} header;

/*
What to put in the instruction, it includes 
Action
Sequence Number
Old Mbox length
New Mbox length
OldMiddlebox List
NewMiddlebox List
*/

int sync_packet(int fd, char * writeBuffer, struct sockaddr_in * servaddr ){
	
	//char * MBox[] ={"128.112.93.108", "128.112.93.107"} ;
	char * oldMBox[] ={"128.112.93.108", "128.112.93.106", "128.112.93.107"} ;
	char * newMBox[] ={"128.112.93.108", "128.112.93.109","128.112.93.107"} ;
	//char * oldMBox[] ={"10.0.0.3", "10.0.0.4",  "10.0.0.5"} ;
	//char * newMBox[] = {"10.0.0.3", "10.0.0.6",  "10.0.0.5"};

	header * hdr_ptr = (header*) writeBuffer;	
	hdr_ptr->oldMboxLength =  ( sizeof (oldMBox) / sizeof(char *) );
	hdr_ptr->newMboxLength = ( sizeof (newMBox) / sizeof(char *) );

	int ByteStreamCount = sizeof(header) +  4* (hdr_ptr->newMboxLength + hdr_ptr->oldMboxLength + 1 ) ;

	//this is UPDATE-SYN
	hdr_ptr->action = 4 ;
	hdr_ptr->sequenceNum  = SEQUENCENUM ;
	
	struct in_addr addr;

	int i = 0;

	for (i=0; i< hdr_ptr->oldMboxLength; i++ ){
		inet_aton(oldMBox[i], &addr);
		memcpy(writeBuffer+ sizeof(header) + 4*i, &addr.s_addr, 4);
	} 
	
	for (i=0; i< hdr_ptr->newMboxLength ; i++ ){
		inet_aton(newMBox[i], &addr);
		memcpy( writeBuffer + sizeof(header) +  hdr_ptr->oldMboxLength*4  + 4*i, &addr.s_addr, 4);
	} 
	int itr =0;
	for (itr = 0; itr<hdr_ptr->oldMboxLength ; itr++){
        struct in_addr addr = *(struct in_addr*) ( writeBuffer + itr*4 + sizeof(header));
        printf("old middlebox list is %s\n",inet_ntoa(addr));
    }

    for (itr = 0; itr<hdr_ptr->newMboxLength ; itr++){
        struct in_addr addr = *(struct in_addr*) ( writeBuffer + itr*4 + hdr_ptr->oldMboxLength *4 + sizeof(header));
        printf("new middlebox list is %s\n",inet_ntoa(addr));
    }

    printf("\n");
	sendto(fd,writeBuffer,ByteStreamCount,0,(struct sockaddr *)servaddr,sizeof(struct sockaddr_in ));
	return 0;
}


int main(int argc, char**argv)
{
	int sockfd,n;
	struct sockaddr_in servaddr;

	if (argc < 2)
	{
		printf("usage:  <IP address>\n");
		exit(1);
	}
	

	sockfd=socket(AF_INET,SOCK_DGRAM,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(argv[1]);
	servaddr.sin_port=htons(UDP_PORT);

	double elapsedTime;
	int flag =0;

	//blocking way, it measures the latency in a better way!
	char sendline[1400];
	char recvline[1400];
	memset(sendline, 0,1400);
	

   
	sync_packet(sockfd, sendline, &servaddr);
	gettimeofday(&t1, NULL);
	struct pollfd poll_fd[1] ;
	poll_fd[0].fd = sockfd;
    poll_fd[0].events = POLLIN|POLLPRI;
	printf("Before ACK \n");

    while(1)
    {
    	int i = poll(poll_fd, 1, 1);
    	if(i==1){
    		//this means you have seen syn-ack
    		n=recvfrom(sockfd,recvline,1400,0,NULL,NULL);
    		break;
    	} else{
    		usleep(1000);
			sync_packet(sockfd, sendline, &servaddr);
    	}
    }
			//fputs(recvline,stdout);

	int ack = *(int *) recvline;
	int seq = *(int *) (recvline +4);
	int end = *(int *) (recvline+20);
	printf("Is is sync ack? %d and %d and the end number is %d\n", ack, seq, end);
	if (ack==5 && seq == SEQUENCENUM)
	{

		int HeaderLength = sizeof(header)+4;
		char AckMesg[HeaderLength];
		* (int*) ((char *)AckMesg + sizeof(header) ) =0;
		header * ackHeader = (header *)AckMesg;
		ackHeader->action = 6;
		ackHeader->sequenceNum = SEQUENCENUM;
		

		gettimeofday(&t2, NULL);
		elapsedTime =(t2.tv_usec - t1.tv_usec) + (t2.tv_sec - t1.tv_sec)*1000000;
		printf("Time is %f\n",elapsedTime);
		
		//this is to send back the final ack and so that the other side of the update can send packets
		sendto(sockfd,AckMesg,HeaderLength,0,(struct sockaddr *) &servaddr,sizeof(struct sockaddr_in ));
		
		while(1){
			n=recvfrom(sockfd,recvline,1400,0,NULL,NULL);
			sendto(sockfd,AckMesg,HeaderLength,0,(struct sockaddr *) &servaddr,sizeof(struct sockaddr_in ));
		}
		
		return 0;
	}
	

}
