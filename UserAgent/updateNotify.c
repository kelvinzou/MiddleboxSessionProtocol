
/* Author:
Kelvin Zou
Princeton University 

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

#define INTI_TO 10000
#define UDP_PORT 1025
#define SEQUENCENUM 100

struct timeval t1, t2;

typedef struct{
	int action;
	int sequenceNum;
	int src_IP;
	int dst_IP;
	__u16 srcPort;
	__u16 dstPort;

} header;

int  srcPort =1 , dstPort=1;

int sync_packet(int fd, char * writeBuffer, struct sockaddr_in * servaddr ){
	
	char * MBox[] ={"128.112.93.108", "128.112.93.107"} ;
	int ByteStreamCount = sizeof(header) + 4 + 4 *( sizeof(MBox) / sizeof(char * )) ;

	header * hdr_ptr = (header*) writeBuffer;

	hdr_ptr->action = 4 ;
	hdr_ptr->sequenceNum  = SEQUENCENUM ;
	struct in_addr addr;

	char * srcIP = "10.0.0.1";

	char * dstIP = "10.0.0.5";

	inet_aton(srcIP, &addr);
	hdr_ptr->src_IP =(int) addr.s_addr;
	inet_aton(dstIP, &addr);
	hdr_ptr->dst_IP =(int) addr.s_addr;
	
	hdr_ptr->srcPort = srcPort;
	hdr_ptr->dstPort = dstPort;
	
	int i = 0;
	for (i=0; i<sizeof(MBox) / sizeof(char * ); i++ ){
		inet_aton(MBox[i], &addr);
		memcpy(writeBuffer+sizeof(header)+ 4*i, &addr.s_addr, 4);
	} 
	gettimeofday(&t1, NULL);
	
	sendto(fd,writeBuffer,ByteStreamCount,0,(struct sockaddr *)servaddr,sizeof(struct sockaddr_in ));
	return 0;
}

void keepalive(int sockfd, struct sockaddr * servaddr){
	int HeaderLength = sizeof(header);
	char AckMesg[HeaderLength];
	header * ackHeader = (header *)AckMesg;
	ackHeader->action = 4;
	ackHeader->sequenceNum = SEQUENCENUM;
	printf("Keep alive!\n");
	sendto(sockfd,AckMesg,sizeof(header),0, servaddr,sizeof(struct sockaddr_in ));
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

// Here we use file descriptor to avoid I/O block at receive side.
	int timeout =3000;
	struct timeval tv;
	fd_set readfds, active_fs;
	tv.tv_sec = 0;
	tv.tv_usec = 3000;
	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);

	
	double elapsedTime;
	int flag =0;

	//blocking way, it measures the latency in a better way!
	char sendline[1400];
	char recvline[1400];
	memset(sendline, 0,1400);
	sync_packet(sockfd, sendline, &servaddr);
	printf("Before ACK \n");
	n=recvfrom(sockfd,recvline,1400,0,NULL,NULL);
			//fputs(recvline,stdout);

	int ack = *(int *) recvline;
	int seq = *(int *) (recvline +4);
	int port = *(int *) (recvline+20);
	printf("Is is sync ack? %d and %d and the port number is %d\n", ack, seq, port);
	if (ack==5 && seq == SEQUENCENUM && n ==24)
	{

		int HeaderLength = sizeof(header);
		char AckMesg[HeaderLength];
		header * ackHeader = (header *)AckMesg;
		ackHeader->action = 6;
		ackHeader->sequenceNum = SEQUENCENUM;
		gettimeofday(&t2, NULL);
		elapsedTime =(t2.tv_usec - t1.tv_usec) + (t2.tv_sec - t1.tv_sec)*1000000;
		printf("Time is %f\n",elapsedTime);
		sendto(sockfd,AckMesg,sizeof(header),0,(struct sockaddr *) &servaddr,sizeof(struct sockaddr_in ));
		/*
		Here we add keep alive messages to show the mobility can be handled for packets on the fly
		
		while(1){
			sleep(5);
			keepalive(sockfd, (struct sockaddr *)&servaddr);
		}
		*/
		return 0;
	}
	

}