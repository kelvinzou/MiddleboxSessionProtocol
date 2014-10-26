
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

#define INTI_TO 10000
int main(int argc, char**argv)
{
	int sockfd,n;
	struct sockaddr_in servaddr,cliaddr;

	if (argc < 2)
	{
		printf("usage:  udpcli <IP address>\n");
		exit(1);
	}

	sockfd=socket(AF_INET,SOCK_DGRAM,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(argv[1]);
	servaddr.sin_port=htons(63000);

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


	while (1)
	{
		
		char sendline[1400];
		char recvline[1400];
		memset(sendline, 0x41,1399);

		printf("Source address for receive from is %s",inet_ntoa(*(struct in_addr*) &cliaddr.sin_addr.s_addr));
		printf(" and %s\n",inet_ntoa(*(struct in_addr*) &servaddr.sin_addr.s_addr));

		

		sendto(sockfd,sendline,1400,0,(struct sockaddr *)&servaddr,sizeof(servaddr));
		
		
		active_fs = readfds;
		select(sockfd+1, &active_fs, NULL, NULL, &tv);
		if(!FD_ISSET(sockfd, &active_fs)){
			printf("Not ready for read yet, skip\n");
			printf("Timeout is now %d\n", timeout);
			usleep(timeout);
			timeout =  timeout *2;
		} else {
			n=recvfrom(sockfd,recvline,1399,0,NULL,NULL);
			recvline[n]=0;
			fputs(recvline,stdout);
			timeout = INTI_TO;
			gettimeofday(&t1, NULL);
		}
		

		gettimeofday(&t2, NULL);
		elapsedTime =(t2.tv_sec - t1.tv_sec);
		if (elapsedTime>30){
			break;
		}

	}
}
