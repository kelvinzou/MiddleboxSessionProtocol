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

#include <pthread.h>
#include <sys/time.h>

/* Sample UDP server */

 typedef struct
{
    int id;
    char* str;
} threadParameter;
/*
void * handleConnection(void * ptr) {

	char * parameter = (char *) ptr;
	printf("%s\n", parameter);
}
*/


void handleRequest(char * request, char * response, int n){
	printf("data is %s\0", request);
	memcpy(response, request, (size_t) n);
	*(response +n) = 0;
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
   	struct timeval t1, t2;
    double elapsedTime;
    gettimeofday(&t1, NULL);
//initialize the socket
    struct timeval tv;
    fd_set readfds, active_fs;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    FD_ZERO(&readfds);


   	int port_count = 1025;
   	int sockfd,n;
   	struct sockaddr_in servaddr,cliaddr;
   	socklen_t len;
   
	sockfd=socket(AF_INET,SOCK_DGRAM,0);
	FD_SET(sockfd, &readfds);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(63000);
	bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	
	//select(sockfd+1, &readfds, NULL, NULL, &tv);
	int drop =0;
	while(1){
		len = sizeof(cliaddr);
		char mesg[1400];
		char response[1400];
		active_fs = readfds;
		select(sockfd+1, &active_fs, NULL, NULL, &tv);
		
		if(FD_ISSET(sockfd, &active_fs)){
			n = recvfrom(sockfd,mesg,1399,0,(struct sockaddr *)&cliaddr,&len);
			printf("Source address for receive from is %s",inet_ntoa(*(struct in_addr*) &cliaddr.sin_addr.s_addr));
			printf(" and %s\n",inet_ntoa(*(struct in_addr*) &servaddr.sin_addr.s_addr));
			mesg[n]=0;;
			handleRequest(mesg, response, n);
			if (drop <8) {
				drop++;
			}
			else {
				sendto(sockfd,response,n,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
				drop =0;
			}
		}
		
		//usleep(10000);
		gettimeofday(&t2, NULL);
   	 	elapsedTime =(t2.tv_sec - t1.tv_sec);
   	 	if (elapsedTime>600){
   	 		break;
   	 	}
	}

}

