
/* Sample UDP client */

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

#define SEQUENCENUM 100


typedef struct{
	int action;
	int sequenceNum;
	int src_IP;
	int dst_IP;
	__u16 srcPort;
	__u16 dstPort;

} header;

int  ConnectionPort, srcPort, dstPort;

int sync_packet(int fd, char * writeBuffer, struct sockaddr_in * servaddr ){
	int ByteStreamCount = sizeof(header) + 4 + 4*3 ;

	header * hdr_ptr = (header*) writeBuffer;



	hdr_ptr->action = 1 ;
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



	char * MBox1 = "10.0.0.2";
	char * MBox2 = "10.0.0.3";
	char * MBox3 = "10.0.0.4";
	inet_aton(MBox1, &addr);
	memcpy(writeBuffer+sizeof(header), &addr.s_addr, 4);
	
	inet_aton(MBox2, &addr);
	memcpy(writeBuffer+sizeof(header)+4, &addr.s_addr, 4);

	inet_aton(MBox3, &addr);
	memcpy(writeBuffer+sizeof(header)+8, &addr.s_addr, 4);

	sendto(fd,writeBuffer,ByteStreamCount,0,(struct sockaddr *)servaddr,sizeof(struct sockaddr_in ));
	return 0;
}


int main(int argc, char**argv)
{
	int sockfd,n;
	struct sockaddr_in servaddr;

	if (argc < 5)
	{
		printf("usage:  udpcli <IP address>, udp <port number>, srcPort, dstPort\n");
		exit(1);
	}
	
	if (sscanf (argv[2], "%i", &ConnectionPort)!=1) { printf ("error - not an integer"); exit(-1); }

	if (sscanf (argv[3], "%i", &srcPort)!=1) { printf ("error - not an integer"); exit(-1); }
	if (sscanf (argv[4], "%i", &dstPort)!=1) { printf ("error - not an integer"); exit(-1); }
	
	sockfd=socket(AF_INET,SOCK_DGRAM,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(argv[1]);
	servaddr.sin_port=htons(ConnectionPort);

// Here we use file descriptor to avoid I/O block at receive side.
	int timeout =1000;
	struct timeval tv;
	fd_set readfds, active_fs;
	tv.tv_sec = 0;
	tv.tv_usec = 1000;
	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);



	struct timeval t1, t2;
	double elapsedTime;
	gettimeofday(&t1, NULL);


	int flag =0;

	while (1)
	{
		
		char sendline[1400];
		char recvline[1400];
		memset(sendline, 0,1400);

		printf(" Destination is %s\n",inet_ntoa(*(struct in_addr*) &servaddr.sin_addr.s_addr));

		if(flag==1 || flag ==0) {
			sync_packet(sockfd, sendline, &servaddr);
		}
		
		//sendto(sockfd,sendline,1400,0,(struct sockaddr *)&servaddr,sizeof(servaddr));
		
		 
		active_fs = readfds;
		select(sockfd+1, &active_fs, NULL, NULL, &tv);
		if(!FD_ISSET(sockfd, &active_fs)){
			printf("Not ready for read yet, skip\n");
			printf("Timeout is now %d\n", timeout);
			//usleep(timeout);
			timeout =  timeout *2;
			if(timeout<40000)
				usleep(timeout);
			else usleep(40000);
			flag =1;
		} else {
			flag =2;
			n=recvfrom(sockfd,recvline,1400,0,NULL,NULL);
			//fputs(recvline,stdout);
			int ack = *(int *) recvline;
			int seq = *(int *) (recvline +4);
			int port = *(int *) (recvline+20);
			printf("Is is sync ack? %d and %d and the port number is %d\n", ack, seq, port);
			if (ack==2 && seq == SEQUENCENUM && n ==28)
			{
				goto confirmed;
			}
			timeout = INTI_TO;
			gettimeofday(&t1, NULL);
		}
		

		gettimeofday(&t2, NULL);
		elapsedTime =(t2.tv_sec - t1.tv_sec);
		if (elapsedTime>30){
			break;
		}

	}
confirmed:
	{
		int HeaderLength = sizeof(header);
		char AckMesg[HeaderLength];
		header * ackHeader = (header *)AckMesg;
		ackHeader->action = 3;
		ackHeader->sequenceNum = SEQUENCENUM;
		sendto(sockfd,AckMesg,sizeof(header),0,(struct sockaddr *) &servaddr,sizeof(struct sockaddr_in ));
		return 0;
	}
	

}
