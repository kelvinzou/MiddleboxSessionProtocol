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
#include <queue>
#include "uthash.h"
#include "flowhash.h"
/* Sample UDP server */
int hashport =1025;
int sequenceNumber = 0;

int sockfd;
struct sockaddr_in servaddr;

using namespace std;
typedef struct {
	char * request;
	int n;
	int port_num;
	volatile int flag;
	struct sockaddr_in * cliAddr;
}parameter;

typedef struct{
	int action;
	int sequenceNum;
	int src_IP;
	int dst_IP;
	__u16 srcPort;
	__u16 dstPort;

} header;

void sendBack(char * request, int n,  struct sockaddr_in * cliAddr){
	header * hdr = (header *) request;
	
	int action = hdr->action;
	int SeqNum = hdr->sequenceNum;

	printf("The action is %d and %d \n",  action, SeqNum);
	int i ;
	for (i=sizeof(header); i<n-4 ; i+=4){
		struct in_addr addr = *(struct in_addr*) (request + i);
		printf("middlebox address for receive from is %s\n",inet_ntoa(addr));
	}

	char response[n];
	header * replyHdr = (header *) response;
	memcpy(response+4, request+4, n-4);
	//memcpy(response+, request+24, (size_t) n-24);
	replyHdr->action = 2;
	//*(int *) (response+4) = sequenceNumber;
	* (int *)( response+ sizeof(header) ) = hashport++;
	printf("The socket fd is %d \n",  sockfd);
	int count = 1;
	while(1){
		count++;
		sendto(sockfd,response,n,0,(struct sockaddr *)cliAddr,sizeof(struct sockaddr_in ));

		unsigned long ip_dst = cliAddr->sin_addr.s_addr;
		unsigned short dstPort = cliAddr->sin_port;
		unsigned long ip_src =servaddr.sin_addr.s_addr; 
		unsigned long srcPort = servaddr.sin_port; 
		printf("Receive from client: dst ip and port is %u and %u\n", ip_dst, ntohs(dstPort));
		flow * retv = NULL;
		findItem( (int) ip_src,(int) ip_dst,(__u16)srcPort,(__u16) dstPort,&retv);

		if (retv!=NULL && retv->acked ==1){
			printf("Packet is acked!\n");
			break;
		} else if(retv!=NULL){
			printf("Not acked yet!\n");
		}
		if (count>=10){
			printf("Timeout!\n");
			break;
		}
		usleep(10000);
	}

}



int sendForward(char * request, int n, int * port_num, struct sockaddr_in * cliAddr){
	
	int SendSockfd ;
   	struct sockaddr_in SendServaddr, SendCliaddr;
   	int i ;
	for (i=sizeof(header); i<n-4 ; i+=4){
		struct in_addr addr = *(struct in_addr*) (request + i);
		printf("middlebox address for receive from is %s\n",inet_ntoa(addr));
	}

	char sendmsg [n-4];
	memcpy(sendmsg+4, request+4, 16);
	memcpy(sendmsg+sizeof(header), request+sizeof(header)+4, n-sizeof(header)-4);
	//it is a sync packet
	*( (int *)sendmsg) = 1;
	SendSockfd=socket(AF_INET,SOCK_DGRAM,0);
	bzero(&SendServaddr,sizeof(SendServaddr));
	printf("The SendSockfd is %d \n", SendSockfd);

	SendServaddr.sin_family = AF_INET;
	struct in_addr addr = *(struct in_addr*) (request + sizeof(header) +4);
	char * IPStr = inet_ntoa(addr);
	SendServaddr.sin_addr.s_addr=inet_addr(IPStr); 

	SendServaddr.sin_port=htons(*port_num);

	sendto(SendSockfd,sendmsg,n-4,0,(struct sockaddr *)&SendServaddr,sizeof(struct sockaddr_in ));

	char recvsendmsg [1400];
	
	int m = recvfrom(SendSockfd,recvsendmsg,1400,0,NULL,NULL);

	header * RecvHeaderPointer = (header *) recvsendmsg;
	int action = RecvHeaderPointer->action;
	int sequenceNumber = RecvHeaderPointer->sequenceNum;
	int port = * (int *)(recvsendmsg+sizeof(header));
	printf("Action is and sequence number is and prot number is %d and %d and %d\n", action, sequenceNumber, port);
	int count =0;
	while(1){
		count++;

		sendto(sockfd,recvsendmsg,m,0,(struct sockaddr *) cliAddr,sizeof(struct sockaddr_in ));
		
		unsigned long ip_dst = cliAddr->sin_addr.s_addr;
		unsigned short dstPort = cliAddr->sin_port;
		unsigned long ip_src =servaddr.sin_addr.s_addr;// servaddr.sin_addr.s_addr;
		unsigned long srcPort = servaddr.sin_port;//servaddr.sin_port;
		printf("Receive from client: dst ip and port is %u and %u\n", ip_dst, ntohs(dstPort));
		flow * retv = NULL;
		findItem( (int) ip_src,(int) ip_dst,(__u16)srcPort,(__u16) dstPort,&retv);

		if (retv!=NULL && retv->acked ==1){
			printf("Packet is acked!\n");
			int HeaderLength = sizeof(header);
			char AckMesg[HeaderLength];
			header * ackHeader = (header *)AckMesg;
			ackHeader->action = 3;
			ackHeader->sequenceNum = sequenceNumber;
			sendto(SendSockfd,AckMesg,HeaderLength,0,(struct sockaddr *)&SendServaddr,sizeof(struct sockaddr_in ));
			break;
		} else if(retv!=NULL){
			printf("Not acked yet!\n");
		}
		if (count>=10){
			printf("Timeout!\n");
			break;
		}
		usleep(10000);
	}
	//char ackPackets[];	
	
}


//void handleRequest(char * request, int n, int * port_num,  struct sockaddr_in * cliAddr){
void * handleRequest(void * ptr){
	parameter * passingparameter = (parameter *) ptr;
	char * request = passingparameter->request;
	struct sockaddr_in * cliAddr = passingparameter->cliAddr;
	int n  =  passingparameter->n;
	int port_num = passingparameter->port_num;

	int action = *( (int *) request);
	int SeqNum = *(int *)(request + 4); 
	if ( SeqNum <= sequenceNumber)
	{
		//Ignore the messge since it is after the current sequence number
		return NULL;
	}	
	//sendBack(request, n, cliAddr);
   	if (n>sizeof(header) +8){
		sendForward(request, n, &port_num, cliAddr);
	} else{
		sendBack(request, n, cliAddr);
	}
	free(request);
	free(cliAddr);
	free(ptr);
	return NULL;
}


int main(int argc, char**argv)
{
   	struct timeval t1, t2;
    double elapsedTime;
    gettimeofday(&t1, NULL);

	sockfd=socket(AF_INET,SOCK_DGRAM,0);

	int intvar, destintVar;
	if(argc!=3) {printf("Argument list wrong, it should be ./serverUDP port_num \n");return 0;}

	if (sscanf (argv[1], "%i", &intvar)!=1) { printf ("error - not an integer\n"); exit(-1); }
	if (sscanf (argv[2], "%i", &destintVar)!=1) { printf ("error - not an integer\n"); exit(-1); }

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(intvar);
	bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	
	//select(sockfd+1, &readfds, NULL, NULL, &tv);

	int counter  = 0;
   	pthread_t thread;
   	char * mesg;
   	struct sockaddr_in * clientAddressPtr;
	int drop =0;
	while(1){
		socklen_t len = sizeof(struct sockaddr_in) ;
		mesg =(char *) malloc(1400);
		clientAddressPtr = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
		
		int n = recvfrom(sockfd,mesg,1400,0,(struct sockaddr *)clientAddressPtr,&len);
		
		unsigned long ip_dst = clientAddressPtr->sin_addr.s_addr;
		unsigned short dstPort = clientAddressPtr->sin_port;
		unsigned long ip_src =servaddr.sin_addr.s_addr;// servaddr.sin_addr.s_addr;
		unsigned long srcPort = servaddr.sin_port;//servaddr.sin_port;
		printf("IP and port is %u and %u\n", ip_dst , dstPort);
		flow * retv = NULL;
		int sequenceNum  = *(int *)(mesg + 4);
		findItem( (int) ip_src,(int) ip_dst,(__u16)srcPort,(__u16) dstPort,&retv);

		if (*(int*) mesg == 1){
			if (retv!=NULL && retv->sequenceNumber >= sequenceNum){
			//printf("Updates are out of date, simply ignore the packet!\n");
			free(mesg);
			free(clientAddressPtr);
		} else{
			printf("Add or update item with a sequence number %d!\n", sequenceNum);

			addItem((int) ip_src,(int) ip_dst,(__u16)srcPort,(__u16) dstPort ,sequenceNum);

			void * para = malloc(sizeof(parameter));
			parameter * passingparameter = (parameter *) para;
			passingparameter->request = mesg;
			passingparameter->cliAddr = clientAddressPtr;
			passingparameter->n = n;
			passingparameter->port_num = destintVar;
			pthread_create(&thread, NULL, handleRequest, para);
		}
		}  else if(*(int*) mesg == 3){
			if(retv!=NULL && retv->sequenceNumber == sequenceNum){
				printf("Acknowledge for a correct sequence number\n");
				retv->acked=1;
			}  else{
				printf("Cannot update for an out-of-order ack packet\n");
			}
		}
		usleep(1000);
		gettimeofday(&t2, NULL);
   	 	elapsedTime =(t2.tv_sec - t1.tv_sec);
   	 	if (elapsedTime>600){
   	 		break;
   	 	}
	}
}

