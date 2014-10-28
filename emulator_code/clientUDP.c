
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
#include <time.h> 
#include <asm/types.h>

#define INTI_TO 10000

#define SEQUENCENUM 100

int sync_packet(int fd, char * writeBuffer, struct sockaddr_in * servaddr ){
	
	int ByteStreamCount = 20;
	*( (int * )writeBuffer) = 1;
	* (int * )(writeBuffer + 4)  = SEQUENCENUM;
	struct in_addr addr;

	char * MBox1 = "127.0.0.1";
	char * MBox2 = "127.0.0.1";
	inet_aton(MBox1, &addr);
	memcpy(writeBuffer+8, &addr.s_addr, 4);
	
	inet_aton(MBox2, &addr);
	memcpy(writeBuffer+12, &addr.s_addr, 4);

	sendto(fd,writeBuffer,ByteStreamCount,0,(struct sockaddr *)servaddr,sizeof(struct sockaddr_in ));
	return 0;
}


int main(int argc, char**argv)
{
	int sockfd,n;
	struct sockaddr_in servaddr,cliaddr;

	if (argc < 3)
	{
		printf("usage:  udpcli <IP address>, udp <port number>\n");
		exit(1);
	}
	int  destintVar;

	if (sscanf (argv[2], "%i", &destintVar)!=1) { printf ("error - not an integer"); exit(-1); }


	sockfd=socket(AF_INET,SOCK_DGRAM,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(argv[1]);
	servaddr.sin_port=htons(destintVar);

// Here we use file descriptor to avoid I/O block at receive side.
	int timeout =10000;
	struct timeval tv;
	fd_set readfds, active_fs;
	tv.tv_sec = 0;
	tv.tv_usec = 10000;
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
		memset(sendline, 0,1399);

		printf("Source address for receive from is %s",inet_ntoa(*(struct in_addr*) &cliaddr.sin_addr.s_addr));
		printf(" and %s\n",inet_ntoa(*(struct in_addr*) &servaddr.sin_addr.s_addr));

		if(flag==1 || flag ==0) {
			sync_packet(sockfd, sendline, &servaddr);
		}
		
		//sendto(sockfd,sendline,1400,0,(struct sockaddr *)&servaddr,sizeof(servaddr));
		
		 
		active_fs = readfds;
		select(sockfd+1, &active_fs, NULL, NULL, &tv);
		if(!FD_ISSET(sockfd, &active_fs)){
			printf("Not ready for read yet, skip\n");
			printf("Timeout is now %d\n", timeout);
			usleep(timeout);
			timeout =  timeout *2;
			flag =1;
		} else {
			flag =2;
			n=recvfrom(sockfd,recvline,1399,0,NULL,NULL);
			//fputs(recvline,stdout);
			int ack = *(int *) recvline;
			int seq = *(int *) (recvline +4);
			printf("Is is sync ack? %d and %d \n", ack, seq);
			if (ack==2 && seq == SEQUENCENUM && n ==16)
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

	return;

}
