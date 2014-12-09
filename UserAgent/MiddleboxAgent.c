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

#include <linux/genetlink.h>

#include "hash.h"
#include "flowhash.h"

#define NETLINK_USER 31

#define MAX_PAYLOAD 1024 /* maximum payload size*/

#define UDP_PORT 1025

#define THREAD_NUM 100
using namespace std;

struct sockaddr_nl netlink_src, netlink_dest;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int netlink_socket_fd;
struct msghdr netlink_msg;


int hashport =2000;
int sequenceNumber = 0;
int thread_iterator = 0;
int sockfd;
struct sockaddr_in servaddr;
struct timeval t1, t2;

volatile int flag =1;

volatile int update_ack =0;

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

/*read function for future use, now hard configure the source code*/

void readConfig(){
    char str[100];
    FILE * file;
    file = fopen( "configuration" , "r");
    if (file) {
        if (fscanf(file, "%s", str)!=EOF)
            printf("%s\n",str);
    fclose(file);
    return;
    }
}

/*
This building block is for netlink

*/
int init_netlink(){
    netlink_socket_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (netlink_socket_fd < 0)
        return -1;
    //set source and destination of netlink 

    //set source, it is itself
    memset(&netlink_src, 0, sizeof(netlink_src));
    netlink_src.nl_family = AF_NETLINK;
    netlink_src.nl_pid = getpid(); /* self pid */

    bind(netlink_socket_fd, (struct sockaddr *)&netlink_src, sizeof(netlink_src));


    //set destination, as kernel, 
    memset(&netlink_dest, 0, sizeof(netlink_dest));
    memset(&netlink_dest, 0, sizeof(netlink_dest));
    netlink_dest.nl_family = AF_NETLINK;
    netlink_dest.nl_pid = 0; /* For Linux Kernel */
    netlink_dest.nl_groups = 0; /* unicast */

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    netlink_msg.msg_name = (void *)&netlink_dest;
    netlink_msg.msg_namelen = sizeof(netlink_dest);
    netlink_msg.msg_iov = &iov;
    netlink_msg.msg_iovlen = 1;
    return 0;
}

int send_netlink(char * input){
    strcpy( (char *)NLMSG_DATA(nlh),input);
    printf("input is %s\n", input);
    
    printf("Sending update message to kernel\n");
    sendmsg(netlink_socket_fd, &netlink_msg, 0);
}

/*
end of netlink building block
*/

/*
Interrupt handler

*/
void sig_handler(int signo)
{
  if (signo == SIGINT)
  {
    printf("received SIGINT\n");
    flag =0;
  }
}

/*
Those are UDP messages that are designed to handle the control part of the protocol

*/

void sendBack(char * request, int n,  struct sockaddr_in * cliAddr){

    double elapsedTime;
    
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
    int count = 0;
    while(1){
        count++;
        if(count%40==1){
            sendto(sockfd,response,n,0,(struct sockaddr *)cliAddr,sizeof(struct sockaddr_in ));
        }
        
        unsigned long ip_dst = cliAddr->sin_addr.s_addr;
        unsigned short dstPort = cliAddr->sin_port;
        unsigned long ip_src =servaddr.sin_addr.s_addr; 
        unsigned short srcPort = servaddr.sin_port; 
    //  printf("Receive from client: dst ip and port is %lu %u \n", ip_dst, dstPort);
        flow * retv = NULL;
        findItem( (int) ip_src,(int) ip_dst,(__u16)srcPort,(__u16) dstPort,&retv);
        
        if (retv!=NULL && retv->acked ==1){
            printf("Packet is acked!\n");
            break;
        } else if(retv!=NULL){
        //  printf("Not acked yet!\n");
        }
        if (count>=1000){
            printf("Timeout!\n");
            break;
        }
        usleep(3000);
    }
    gettimeofday(&t2, NULL);
    elapsedTime =(t2.tv_usec - t1.tv_usec) + (t2.tv_sec - t1.tv_sec)*1000000;
    printf("3. Elapse Time is %f\n",elapsedTime);

}



int sendForward(char * request, int n, int * port_num, struct sockaddr_in * cliAddr){
    struct timeval t1, t2;
    double elapsedTime;
    char recvsendmsg [1400];

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
    //Building a sync packet, and it sends to the next hop infinitely!
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
    gettimeofday(&t1, NULL);
    int m = recvfrom(SendSockfd,recvsendmsg,1400,0,NULL,NULL);
    gettimeofday(&t2, NULL);
    double elapsedTimeOld =(t2.tv_usec - t1.tv_usec) + (t2.tv_sec - t1.tv_sec)*1000000;
    //printf("1. Elapse Time is %f\n",elapsedTime);
    

    header * RecvHeaderPointer = (header *) recvsendmsg;
    
    int action = RecvHeaderPointer->action;
    int sequenceNumber = RecvHeaderPointer->sequenceNum;
    int port = * (int *)(recvsendmsg+sizeof(header));

    //printf("Action is and sequence number is and port number is %d and %d and %d\n", action, sequenceNumber, port);

    int count =0;

    gettimeofday(&t1, NULL);
    //receivd the message from the next hop, and reply the message back to the last hop, it is basically a repeated sending, until it is terminate. 
    while(1){
        count++;
        if(count%40==1){
        sendto(sockfd,recvsendmsg,m,0,(struct sockaddr *) cliAddr,sizeof(struct sockaddr_in ));
        }
        unsigned long ip_dst = cliAddr->sin_addr.s_addr;
        unsigned short dstPort = cliAddr->sin_port;
        unsigned long ip_src =servaddr.sin_addr.s_addr;// servaddr.sin_addr.s_addr;
        unsigned short srcPort = servaddr.sin_port;//servaddr.sin_port;
        //printf("Receive from client: dst ip and port is %u and %u, %u and %u \n", ip_dst, ntohs(dstPort),  ip_src, ntohs(srcPort));
        flow * retv = NULL;
        findItem( (int) ip_src,(int) ip_dst,(__u16)srcPort,(__u16) dstPort,&retv);

        if (retv!=NULL && retv->acked ==1){
            //printf("Packet is acked!\n");
            int HeaderLength = sizeof(header);
            char AckMesg[HeaderLength];
            header * ackHeader = (header *)AckMesg;
            ackHeader->action = 3;
            ackHeader->sequenceNum = sequenceNumber;
            sendto(SendSockfd,AckMesg,HeaderLength,0,(struct sockaddr *)&SendServaddr,sizeof(struct sockaddr_in ));
            break;
        } else if(retv!=NULL){
            //printf("Not acked yet!\n");
        }
        if (count>=1000){
            printf("Timeout!\n");
            break;
        }
        usleep(3000);
    }
    printf("1. Elapse Time is %f\n",elapsedTimeOld);
    gettimeofday(&t2, NULL);
    elapsedTime =(t2.tv_usec - t1.tv_usec) + (t2.tv_sec - t1.tv_sec)*1000000;
    printf("2. Elapse Time is %f\n",elapsedTime);
}

//this notifies the update, it is UPDATE-SYN to next hop and SYN to back 
void updateForward(char * request, int n, int * port_num, struct sockaddr_in * cliAddr){
    struct timeval t1, t2;
    double elapsedTime;
    char recvsendmsg [1400];

    //send to next hop
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
    //Building a sync packet, and it sends to the next hop infinitely!
    *( (int *)sendmsg) = 4;
    SendSockfd=socket(AF_INET,SOCK_DGRAM,0);
    bzero(&SendServaddr,sizeof(SendServaddr));
    printf("The SendSockfd is %d \n", SendSockfd);

    SendServaddr.sin_family = AF_INET;
    struct in_addr addr = *(struct in_addr*) (request + sizeof(header) +4);
    char * IPStr = inet_ntoa(addr);

    printf("the destination is %s\n", IPStr);
    SendServaddr.sin_addr.s_addr=inet_addr(IPStr); 

    SendServaddr.sin_port=htons(*port_num);

    sendto(SendSockfd,sendmsg,n-4,0,(struct sockaddr *)&SendServaddr,sizeof(struct sockaddr_in ));
    
    //receiving ACK from the receiver
    int m ;
    m = recvfrom(SendSockfd,recvsendmsg,1400,0,NULL,NULL);

    m = recvfrom(SendSockfd,recvsendmsg,1400,0,NULL,NULL);
    
    header * RecvHeaderPointer = (header *) recvsendmsg;
    
    int action = RecvHeaderPointer->action;
    int sequenceNumber = RecvHeaderPointer->sequenceNum;

    printf("Action is and sequence number is and seq number is %d and %d \n", action, sequenceNumber);

    gettimeofday(&t1, NULL);
    //receivd the message from the next hop, and reply the message back to the last hop, it is basically a repeated sending, until it is terminate. 
    //the repeated sending are from the last hop ack, so we simply relay the message
    
    struct timeval tv;

    fd_set readfds, active_fs;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;
    FD_ZERO(&readfds);
    FD_SET(SendSockfd, &readfds);

    while(1){
        active_fs = readfds;
        sendto(sockfd,recvsendmsg,m,0,(struct sockaddr *) cliAddr,sizeof(struct sockaddr_in ));
        int i =  select(SendSockfd+1, &active_fs, NULL, NULL, &tv);
        printf("%d select value", i);
        if( FD_ISSET(SendSockfd, &active_fs) ){  
            m = recvfrom(SendSockfd,recvsendmsg,1400,0,NULL,NULL);

            printf("Is is update sync ack? relaying packet again\n" );

            if(update_ack ==1){
                printf("Packet is been acked, so we can exit this loop now!\n");
                break;
            }
            else {
                printf("No Ack yet!\n");
            }
        } else{
            printf("Socket not set yet\n" );
            usleep(10000);
        }
    }
}



void updateBack(char * request, int n,  struct sockaddr_in * cliAddr){
    double elapsedTime;
    
    header * hdr = (header *) request;
    
    int action = hdr->action;
    int SeqNum = hdr->sequenceNum;

    printf("The action is %d and %d \n",  action, SeqNum);
    int i ;
    for (i=sizeof(header); i<n-4 ; i+=4){
        struct in_addr addr = *(struct in_addr*) (request + i);
        printf("middlebox address for receive from is %s\n",inet_ntoa(addr));
    }

    char response[n-4];
    header * replyHdr = (header *) response;
    memcpy(response+4, request+4, n-4); 
    //this is UPDATE-SYN-AC
    replyHdr->action =5;
    * (int *)( response+ sizeof(header) ) = 0;

    printf("The socket fd is %d \n",  sockfd);

    int count = 0;
    while(1){
        count++;
        if(count%40==1){
            sendto(sockfd,response,n-4,0,(struct sockaddr *)cliAddr,sizeof(struct sockaddr_in ));
        }
        
        if (update_ack ==1){
            printf("Packet is acked!\n");
            break;
        } else  {
            printf("Not acked yet!\n");
        }
        if (count>=1000){
            printf("Timeout!\n");
            break;
        }
        usleep(300);
    }
    gettimeofday(&t2, NULL);
    elapsedTime =(t2.tv_usec - t1.tv_usec) + (t2.tv_sec - t1.tv_sec)*1000000;
    printf("3. Elapse Time is %f\n",elapsedTime);

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




void * handleUpdate(void * ptr){
    printf("Debug, get in HandleUpdate?\n");
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
    if (n>sizeof(header) +8){
       updateForward(request, n, &port_num, cliAddr);
    } else{
       updateBack(request, n, cliAddr);
    }
    printf("Debug, get in HandleUpdate?\n");

    free(request);
    free(cliAddr);
    free(ptr);
    return NULL;

}


int main(int argc, char *argv[])
{
    //interrupt handler
    //if (signal(SIGINT, sig_handler) == SIG_ERR)
    { 
        printf("\ncan't catch SIGINT\n");
    }
   // readConfig();

    //configure the netlink
   init_netlink();

/*
   //set up the message
    char input [MAX_PAYLOAD-1];
    strcpy(input, "add");
    //send_netlink(input);

    //receive message
    printf("Waiting for message from kernel\n");
   // recvmsg(netlink_socket_fd, &netlink_msg, 0);
    printf("Received message payload: %s\n", (char * ) NLMSG_DATA(nlh));
*/
//the following is for UDP packet handling
    double elapsedTime;
    gettimeofday(&t1, NULL);

    sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port=htons(UDP_PORT);
    bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

    int counter  = 0;
    pthread_t thread[THREAD_NUM];
    char * mesg;
    struct sockaddr_in * clientAddressPtr;
    int drop =0;

    while(flag==1){

        socklen_t len = sizeof(struct sockaddr_in) ;
        mesg =(char *) malloc(1400);
        clientAddressPtr = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
        
        int n = recvfrom(sockfd,mesg,1400,0,(struct sockaddr *)clientAddressPtr,&len);
        header * msgheader =  (header * ) mesg;
        //check the item in the hash table and then check the sequence number
        unsigned long ip_dst = msgheader->dst_IP;
        unsigned short dstPort = msgheader-> dstPort;
        unsigned long ip_src = msgheader -> src_IP;// servaddr.sin_addr.s_addr;
        unsigned short srcPort = msgheader-> srcPort;//servaddr.sin_port;
        
        flow * retv = NULL;
        int sequenceNum  = msgheader->sequenceNum;
        findItem( (int) ip_src,(int) ip_dst,(__u16)srcPort,(__u16) dstPort,&retv);
        

        //The first part is to check whether the it is a sync request
        if (msgheader->action == 1){
            if (retv!=NULL && retv->sequenceNumber >= sequenceNum){
                printf("SYN are out of date, simply ignore the packet!\n");
                free(mesg);
                free(clientAddressPtr);
            } else{
                gettimeofday(&t1, NULL);
                printf("IP and port is %lu and %u \n", ip_dst , dstPort);

                printf("Add an item with a sequence number %d!\n", sequenceNum);

                addItem((int) ip_src,(int) ip_dst,(__u16)srcPort,(__u16) dstPort ,sequenceNum);

                void * para = malloc(sizeof(parameter));
                parameter * passingparameter = (parameter *) para;
                passingparameter->request = mesg;
                passingparameter->cliAddr = clientAddressPtr;
                passingparameter->n = n;
                passingparameter->port_num = UDP_PORT;
                pthread_create(&(thread[thread_iterator]), NULL, handleRequest, para);
                thread_iterator++;
                //right now I dont recycle threads in thread pool yet, should be fixed soon!
                if(thread_iterator>=THREAD_NUM){
                    thread_iterator=0;
                }   
            }
        } 
        // the second part is to check whether it is a ack? 
        else if(msgheader->action  == 3){
            if(retv!=NULL && retv->sequenceNumber == sequenceNum){
                printf("Acknowledge for a correct sequence number\n");
                retv->acked=1;
            }  else if (retv==NULL){
                printf("Cannot ACK NULL entry\n");
            } else{
                printf("Cannot ACK for an out-of-order ack packet%d\n",sequenceNum );
            }
            free(mesg);
            free(clientAddressPtr);
        }
        // UPDATE-SYN
         else if (msgheader->action ==4){
            if (retv!=NULL && retv->sequenceNumber >= sequenceNum){
                printf("Updates are out of date, simply ignore the packet!\n");
                free(mesg);
                free(clientAddressPtr);
            } else{
                printf("IP and port is %lu and %u \n", ip_dst , dstPort);
                printf("Update item with a sequence number %d!\n", sequenceNum);
                addItem((int) ip_src,(int) ip_dst,(__u16)srcPort,(__u16) dstPort ,sequenceNum);

                void * para = malloc(sizeof(parameter));
                parameter * passingparameter = (parameter *) para;
                passingparameter->request = mesg;
                passingparameter->cliAddr = clientAddressPtr;
                passingparameter->n = n;
                passingparameter->port_num = UDP_PORT;
                pthread_create(&(thread[thread_iterator]), NULL, handleUpdate, para);
                thread_iterator++;

                //right now I dont recycle threads in thread pool yet, should be fixed soon!
                if(thread_iterator>=THREAD_NUM){
                    thread_iterator=0;
                }   

            }  
              
         }
          else if(msgheader->action  == 6){
            if(retv!=NULL && retv->sequenceNumber == sequenceNum){
                printf("Acknowledge for a correct sequence number\n");
                update_ack = 1;
            }  else if (retv==NULL){
                printf("Cannot ACK NULL entry\n");
            } else{
                printf("Cannot ACK for an out-of-order ack packet%d\n",sequenceNum );
            }
            free(mesg);
            free(clientAddressPtr);
        }
        gettimeofday(&t2, NULL);
        elapsedTime =(t2.tv_sec - t1.tv_sec);
        if (elapsedTime>600){
            break;
        }
    }
    close(netlink_socket_fd);
    free(mesg);
    free(clientAddressPtr);
	return 0;
}




