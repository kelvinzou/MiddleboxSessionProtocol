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
#include <pthread.h>
#include <sys/time.h>
#include "uthash.h"
#include "flowhash.h"
/* Sample UDP server */
int hashport =1025;
int sequenceNumber = 0;

int sockfd;




void sendBack(char * request, int n,  struct sockaddr_in * cliAddr){
	int action = *( (int *) request);
	int SeqNum = * (int *) (request + 4); 
	sequenceNumber = SeqNum;
	printf("The action is %d and %d \n",  action, SeqNum);
	int i ;
	for (i=8; i<n-4 ; i+=4){
		struct in_addr addr = *(struct in_addr*) (request + i);
		printf("middlebox address for receive from is %s\n",inet_ntoa(addr));
	}

	char response[n];
	memcpy(response+12, request+8, (size_t) n-8);
	*( (int *) response) = 2;
	*(int *) (response+4) = sequenceNumber;
	* (int *)( response+ 8 ) = hashport++;
	printf("The socket fd is %d \n",  sockfd);
	sendto(sockfd,response,n,0,(struct sockaddr *)cliAddr,sizeof(struct sockaddr_in ));

}

int sendForward(char * request, int n, int * port_num, struct sockaddr_in * cliAddr){
	
	int SendSockfd ;
   	struct sockaddr_in SendServaddr, SendCliaddr;

	

	char sendmsg [n-4];
	memcpy(sendmsg+8, request+12, (size_t) n-12);

	//it is a sync packet
	*( (int *)sendmsg) = 1;
	printf("The action is %d \n", sequenceNumber);
	//set sequence number is here
	* (int *)(sendmsg+4) = sequenceNumber;
	printf("The action is %d \n", * (int *)(sendmsg+4) );
	//set sequence number is here
	//*( (int *) sendmsg+4) = sequenceNumber;
	SendSockfd=socket(AF_INET,SOCK_DGRAM,0);

	bzero(&SendServaddr,sizeof(SendServaddr));
	
	printf("The SendSockfd is %d \n", SendSockfd);

	SendServaddr.sin_family = AF_INET;
	struct in_addr addr = *(struct in_addr*) (request + 12);
	char * IPStr = inet_ntoa(addr);
	SendServaddr.sin_addr.s_addr=inet_addr(IPStr); 

	SendServaddr.sin_port=htons(*port_num);

	sendto(SendSockfd,sendmsg,n-4,0,(struct sockaddr *)&SendServaddr,sizeof(struct sockaddr_in ));

	char recvsendmsg [n-4];
	
	int m = recvfrom(SendSockfd,recvsendmsg,1400,0,NULL,NULL);
	
	sendto(sockfd,recvsendmsg,n-4,0,(struct sockaddr *) cliAddr,sizeof(struct sockaddr ));
	int responseSeq = *(int *)(recvsendmsg+4) ;
	printf("Do we ever get to this line? and SeqNum is  %d %d\n", m, responseSeq);

}

void handleRequest(char * request, int n, int * port_num,  struct sockaddr_in * cliAddr){
	int action = *( (int *) request);
	int SeqNum = *(int *)(request + 4); 
	if ( SeqNum < sequenceNumber)
	{
		//Ignore the messge since it is after the current sequence number
		return;
	}	
	sendBack(request, n, cliAddr);
   	if (n>16){
		sendForward(request, n, port_num, cliAddr);
	}
}


int main(int argc, char**argv)
{
	/*
//initialize the threads	
	int counter  = 0;
   	pthread_t threads[10000];
   	const char * msg = "child thread";
  	//here we keep a queue of idle threads in queue idle, 
  	//and everytime we grab a thread id from the idle pool and create a thread to run it

   	queue<int> idle;
   	int i =0;
   	for(i=0; i<1000; i++){
   		idle.push(i);
   	}
*/

//keep track of time, and exits with a certain interval, since we cannot kill it in mininet

   	//this marks an exit after 600 seconds
   	//used for timing
   	struct timeval t1, t2;
    double elapsedTime;
    gettimeofday(&t1, NULL);


    //this is not much useful, since it is for nonblocking sockets
    //save it in the future in case
    /*
    struct timeval tv;
    fd_set readfds, active_fs;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    FD_ZERO(&readfds);

	FD_SET(sockfd, &readfds);

	active_fs = readfds;
	select(sockfd+1, &active_fs, NULL, NULL, &tv);
	
	if(FD_ISSET(sockfd, &active_fs)){
		n = recvfrom(sockfd,mesg,1400,0,(struct sockaddr *)&cliaddr,&len);
	}
	*/
	
   	socklen_t len;
	struct sockaddr_in servaddr,cliaddr;
   
	sockfd=socket(AF_INET,SOCK_DGRAM,0);

	int intvar, destintVar;
	if(argc!=3) {printf("Argument list wrong, it should be ./serverUDP port_num \n");return 0;}

	if (sscanf (argv[1], "%i", &intvar)!=1) { printf ("error - not an integer"); exit(-1); }
	if (sscanf (argv[2], "%i", &destintVar)!=1) { printf ("error - not an integer"); exit(-1); }

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(intvar);
	bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	
	//select(sockfd+1, &readfds, NULL, NULL, &tv);
	int drop =0;
	while(1){
		len = sizeof(cliaddr);
		char mesg[1400];

		int n = recvfrom(sockfd,mesg,1400,0,(struct sockaddr *)&cliaddr,&len);
		handleRequest(mesg, n, & destintVar, &cliaddr);


		//char response[1400];
		/*
		active_fs = readfds;
		select(sockfd+1, &active_fs, NULL, NULL, &tv);
		
		if(FD_ISSET(sockfd, &active_fs)){
			n = recvfrom(sockfd,mesg,1400,0,(struct sockaddr *)&cliaddr,&len);
			
			printf("Source address for receive from is %s",inet_ntoa(*(struct in_addr*) &cliaddr.sin_addr.s_addr));
			printf(" and %s\n",inet_ntoa(*(struct in_addr*) &servaddr.sin_addr.s_addr));

			handleRequest(mesg, n, & destintVar, &cliaddr);
			break;
		}
		*/
		usleep(1000);
		gettimeofday(&t2, NULL);
   	 	elapsedTime =(t2.tv_sec - t1.tv_sec);
   	 	if (elapsedTime>600){
   	 		break;
   	 	}
	}
}

