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

#include <netinet/in.h>
#include <linux/netfilter.h>        /* for NF_ACCEPT */
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <libnetfilter_queue/libnetfilter_queue.h>

#define NETLINK_USER 31

#define MAX_PAYLOAD 1024 /* maximum payload size*/

#define UDP_PORT 1025

#define THREAD_NUM 100

#define NETLINK_FLAG false

#define RETRANSMIT_TIMER 1000 //minimum is 1000 since poll only supports down to 1 ms

using namespace std;



/*Netfilter queue*/

/********************************************/
struct nfq_handle *h;
struct nfq_q_handle *qh;
struct nfnl_handle *nh;
int nf_queue_fd;
int recvCount;
char buf[65536] __attribute__((aligned));
volatile int nfq_flag = 0;

/********************************************/



/*this is netlink variables*/

struct sockaddr_nl netlink_src, netlink_dest;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int netlink_socket_fd;
struct msghdr netlink_msg;

pthread_mutex_t lock;
pthread_mutex_t buffer_lock;



/*this is for socket port */
int hashport =2000;
int sequenceNumber = 0;
int thread_iterator = 0;
int sockfd;
struct sockaddr_in servaddr;
struct timeval t1, t2;
struct sockaddr_in * replyAddr[2];


/*
The following variables are for interaction between two different connections both at the first and the last hop
*/
int old_syn_ack = 0;
int new_syn_ack = 0;

int old_syn = 0;
int new_syn = 0;

int first_syn =0;
int first_ack =0;

volatile int flag =1;

volatile int update_ack =0;

typedef struct parameter{
    char * request;
    int n;
    int port_num;
    volatile int flag;
    struct sockaddr_in * cliAddr;
}parameter;


typedef struct header {
    int action;
    int sequenceNum;
    int oldMboxLength;
    int newMboxLength;
} header;


/*This is for netilter queue call back*/

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
              struct nfq_data *nfa, void *data);

void nfq_init();




/*
This building block is for netlink

*/
int netlink_init();
int send_netlink(char * input);
/*
end of netlink building block
*/
void settingAck(char * AckMesg, int sequenceNumber);


//this notifies the update, it is UPDATE-SYN to next hop and SYN to back 
void relayMsg(char * request, int n, int * port_num, struct sockaddr_in * cliAddr);
void * handleACK(void * ptr);
void * handleUpdate(void * ptr);

void * sendback_packet(void * ptr);


int main(int argc, char *argv[])
{

    netlink_init();
    nfq_init();


    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init failed\n");
        return 1;
    }
    if (pthread_mutex_init(&buffer_lock, NULL) != 0) {
        printf("\n mutex2 init failed\n");
        return 1;
    }


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
    pthread_t releaseQueue_thread;

    pthread_create(&releaseQueue_thread, NULL, sendback_packet, NULL);

    char * mesg;
    struct sockaddr_in * clientAddressPtr;
    int drop =0;
    while(flag==1){

        socklen_t len = sizeof(struct sockaddr_in) ;
        mesg =(char *) malloc(1400);
        clientAddressPtr = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
       
        int n = recvfrom(sockfd,mesg,1400,0,(struct sockaddr *)clientAddressPtr,&len);
        header * msgheader =  (header * ) mesg;

        int sequenceNum  = msgheader->sequenceNum;

        if (msgheader->action ==4){
            if(sequenceNumber >= sequenceNum){
                printf("Old update, ignore\n");
                free(mesg);
                free(clientAddressPtr);
            } else{
                gettimeofday(&t1, NULL);
                
                if (first_syn==0){
                    first_syn=1;
                    pthread_mutex_lock(&buffer_lock);
                    if(NETLINK_FLAG){
                    char * netlink_message = "SYN";
                    send_netlink(netlink_message);
                    }
                    printf("SYN\n");
                }
                

                if (msgheader->oldMboxLength ==0 &&msgheader->newMboxLength >1 ){
                    //it is the new path, middle point
                    //no need to wait for old path syn-ack
                    sequenceNumber = sequenceNum;

                    printf("New msg\n");
                    old_syn_ack =1;
                    void * para = malloc(sizeof(parameter));
                    parameter * passingparameter = (parameter *) para;
                    passingparameter->request = mesg;
                    passingparameter->cliAddr = clientAddressPtr;

                    passingparameter->n = n;
                    passingparameter->port_num = UDP_PORT;
                    pthread_create(&(thread[thread_iterator]), NULL, handleUpdate, para);
                    thread_iterator++;

                    if(thread_iterator>=THREAD_NUM){
                        thread_iterator=0;
                    }  
                } 

                else if (msgheader->oldMboxLength >1 &&msgheader->newMboxLength ==0 ){
                    //it is the old path, middle point
                    //no need to wait for new path syn-ack
                    sequenceNumber = sequenceNum;

                    printf("Old msg\n");
                    new_syn_ack =1;
                    void * para = malloc(sizeof(parameter));
                    parameter * passingparameter = (parameter *) para;
                    passingparameter->request = mesg;
                    passingparameter->cliAddr = clientAddressPtr;

                    passingparameter->n = n;
                    passingparameter->port_num = UDP_PORT;
                    pthread_create(&(thread[thread_iterator]), NULL, handleUpdate, para);
                    thread_iterator++;

                    if(thread_iterator>=THREAD_NUM){
                        thread_iterator=0;
                    }  
                    
                } 

                else if (msgheader->oldMboxLength >1 && msgheader->newMboxLength >1 ) {
                    // it is the beginning of split, at the first hop
                    sequenceNumber = sequenceNum;
                    
                    printf("This is for split\n");
                    int oldMesgLen = sizeof(header) + 4*msgheader->oldMboxLength +4 ;
                    int newMesgLen = sizeof(header) + 4*msgheader->newMboxLength +4 ;
                    
                    char * oldMesg = (char *) malloc(oldMesgLen);
                    char * newMesg = (char *) malloc(newMesgLen);

                    memcpy(oldMesg, mesg, sizeof(header));
                    memcpy(newMesg, mesg, sizeof(header));

                    header * oldMesgHeader = (header *) oldMesg;
                    header * newMesgHeader = (header *) newMesg;

                    oldMesgHeader->newMboxLength = 0;
                    newMesgHeader-> oldMboxLength = 0;

                    printf("The length for middlebox lists are %d and %d\n", oldMesgHeader->oldMboxLength, newMesgHeader->newMboxLength);

                    memcpy(oldMesg + sizeof(header), mesg + sizeof(header) , 4*oldMesgHeader->oldMboxLength );
                    memset(oldMesg + sizeof(header)+ 4 * oldMesgHeader->oldMboxLength, 0, 4);

                    memcpy(newMesg + sizeof(header), mesg + sizeof(header) + 4*msgheader->oldMboxLength, 4 * newMesgHeader->newMboxLength  );
                    memset(newMesg + sizeof(header) + 4* newMesgHeader->newMboxLength, 0, 4 );

                    int itr =0;
                    for (itr = 0; itr<oldMesgHeader->oldMboxLength ; itr++){
                        struct in_addr addr = *(struct in_addr*) ( oldMesg + itr*4 + sizeof(header));
                        printf("old middlebox list is %s\n",inet_ntoa(addr));
                    }

                    for (itr = 0; itr<newMesgHeader->newMboxLength ; itr++){
                        struct in_addr addr = *(struct in_addr*) ( newMesg + itr*4 + sizeof(header));
                        printf("new middlebox list is %s\n",inet_ntoa(addr));
                    }

                    void * oldPara = malloc(sizeof(parameter));
                    parameter * old_passing_parameter = (parameter *) oldPara;
                    old_passing_parameter->request = oldMesg;
                    old_passing_parameter->cliAddr = clientAddressPtr;
                    old_passing_parameter->n = oldMesgLen;
                    old_passing_parameter->port_num = UDP_PORT;

                    void * newPara = malloc(sizeof(parameter));
                    parameter * new_passing_parameter = (parameter *) newPara;
                    new_passing_parameter->request = newMesg;
                    new_passing_parameter->cliAddr = clientAddressPtr;
                    new_passing_parameter->n = oldMesgLen;
                    new_passing_parameter->port_num = UDP_PORT;

                    pthread_create(&(thread[thread_iterator]), NULL, handleUpdate, oldPara);
                    thread_iterator++;

                    pthread_create( &(thread[thread_iterator]), NULL, handleUpdate, newPara );
                    thread_iterator++;

                    if(thread_iterator>=THREAD_NUM){
                        thread_iterator=0;
                    }
                    free (mesg);

                }
                else if (msgheader->oldMboxLength == 1 || msgheader->newMboxLength == 1 ) {
                    printf("This is for merge\n");
                    
                    if(msgheader->oldMboxLength == 1){
                        old_syn = 1;
                        if(new_syn ==1){
                            sequenceNumber = sequenceNum;
                            replyAddr[0] = clientAddressPtr;
                            if(first_syn==0){
                                printf("Ready for SYN-ACK\n");
                                pthread_mutex_lock(&buffer_lock);
                                first_syn=1;
                            }
                            pthread_create(&(thread[thread_iterator]), NULL, handleACK, NULL);
                            thread_iterator++;
                        } else{
                            replyAddr[0] = clientAddressPtr;
                            printf("See only old syn and not new syn yet!\n");
                        }
                        

                    } else{
                        new_syn = 1;
                        if(old_syn ==1){
                            replyAddr[1] = clientAddressPtr;
                            sequenceNumber = sequenceNum;
                            if(first_syn==0){
                                pthread_mutex_lock(&buffer_lock);

                                printf("Ready for SYN-ACK\n");
                                first_syn=1;
                            }
                            pthread_create(&(thread[thread_iterator]), NULL, handleACK, NULL);
                            thread_iterator++;

                        } else{
                            replyAddr[1] = clientAddressPtr;
                            printf("See only new syn and not old syn yet!\n");
                        }

                    }

                }
            }
              
         }
          else if(msgheader->action  == 6){
            if( sequenceNumber == sequenceNum){
                if(first_ack ==0){
                    printf("ACK\n");
                    first_ack=1;
                    pthread_mutex_unlock(&buffer_lock);
                }


                pthread_mutex_lock(&lock);
                update_ack = 1; 
                pthread_mutex_unlock(&lock);
            } 
        }
        gettimeofday(&t2, NULL);
        elapsedTime =(t2.tv_sec - t1.tv_sec);
        if(elapsedTime>60)
        {
            break;
        }
    }
    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&buffer_lock);
    close(netlink_socket_fd);
    free(mesg);
    free(clientAddressPtr);
    nfq_destroy_queue(qh);

    printf("closing library handle\n");
    nfq_close(h);
	return 0;
}

//This is the netfilter queue management thread

void * sendback_packet(void * ptr){
    int packet_counter =0;
  //  if(NETLINK_FLAG){
        //while ((recvCount = recv(nf_queue_fd, buf, sizeof(buf), 0)) && recvCount >= 0) 
        
    while(1){


        while(first_syn==1){
        pthread_mutex_lock(&buffer_lock);
        usleep(1000000);
        packet_counter ++;
        printf("pkt received %d\n", packet_counter);
        //nfq_handle_packet(h, buf, recvCount);
        pthread_mutex_unlock(&buffer_lock);

        }
        usleep(1000000);
        
    }
    
}




static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
              struct nfq_data *nfa, void *data)
{
    //u_int32_t id = print_pkt(nfa);
    unsigned char * full_packet_ptr;
    int id = 0;
    struct nfqnl_msg_packet_hdr *ph;
    ph = nfq_get_msg_packet_hdr(nfa);
    
    if (ph) {
       // printf("ever get in this line?\n");
        id = ntohl(ph->packet_id);

        int size = nfq_get_payload(nfa, &full_packet_ptr);
        struct iphdr * ip = (struct iphdr * ) full_packet_ptr;
        unsigned short  iphdrlen =ip->ihl*4;
        unsigned short length =ntohs(ip->tot_len);
        struct tcphdr *tcp = (struct tcphdr *) (full_packet_ptr + iphdrlen);
       if (tcp->urg==1)
       {
           printf("set as an urg packet \n");
           nfq_flag = 1;
           tcp->urg=0;
           return nfq_set_verdict(qh, id, NF_ACCEPT, length, full_packet_ptr);
       } 
        return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
    }
    
   // printf("entering callback\n");
    return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
}

void nfq_init(){
    printf("opening library handle\n");
    h = nfq_open();

    if (!h) {
        fprintf(stderr, "error during nfq_open()\n");
        exit(1);
    }

    printf("unbinding existing nf_queue handler for AF_INET (if any)\n");

    if (nfq_unbind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "error during nfq_unbind_pf()\n");
        exit(1);
    }

    printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");

    if (nfq_bind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "error during nfq_bind_pf()\n");
        exit(1);
    }

    printf("binding this socket to queue '0'\n");
    
    //this is to create and use a queue and register call back function
    qh = nfq_create_queue(h,  0, &cb, NULL);

    if (!qh) {
        fprintf(stderr, "error during nfq_create_queue()\n");
        exit(1);
    }

    printf("setting copy_packet mode\n");

    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
        fprintf(stderr, "can't set packet_copy mode\n");
        exit(1);
    }
    if(nfq_set_queue_maxlen(qh, 2048) <0){
        fprintf(stderr, "can't set queue length \n");
        exit(1);
    }
    nh = nfq_nfnlh(h);
    nf_queue_fd = nfnl_fd(nh);
}


int netlink_init(){
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


void settingAck(char * AckMesg, int sequenceNumber){

   //TODO 
    * (int*) ((char *)AckMesg + sizeof(header) ) =0;
    header * ackHeader = (header *)AckMesg;
    ackHeader->action = 6;
    ackHeader->sequenceNum = sequenceNumber;

    ackHeader->oldMboxLength =0;
    ackHeader->newMboxLength =0;

}


void relayMsg(char * request, int n, int * port_num, struct sockaddr_in * cliAddr){
    char recvsendmsg [1400];
    //send to next hop
    int SendSockfd ;
    struct pollfd poll_fd[1] ;
    struct sockaddr_in SendServaddr, SendCliaddr;
    int i ;
    
    for (i=sizeof(header); i<n-4 ; i+=4){
        struct in_addr addr = *(struct in_addr*) (request + i);
        printf("middlebox address for receive from is %s\n",inet_ntoa(addr));
    }

    char sendmsg [n-4];
    memcpy(sendmsg, request, sizeof(header) );
    memcpy(sendmsg + sizeof(header), request + sizeof(header)+4,  n - sizeof(header) - 4);
    free(request);
    header * sendmsgHeader = (header *) sendmsg;

    //decrease the middlebox list middlebox count
    if(sendmsgHeader->oldMboxLength >0)
    {
        sendmsgHeader->oldMboxLength = sendmsgHeader->oldMboxLength -1;
    } else{
        sendmsgHeader->newMboxLength = sendmsgHeader->newMboxLength -1;
    }

    //Building a sync packet, and it sends to the next hop infinitely!
    
    SendSockfd=socket(AF_INET,SOCK_DGRAM,0);
    bzero(&SendServaddr,sizeof(SendServaddr));
    printf("The SendSockfd is %d \n", SendSockfd);

    SendServaddr.sin_family = AF_INET;
    struct in_addr addr = *(struct in_addr*) (sendmsg + sizeof(header));
    char * IPStr = inet_ntoa(addr);

    printf("the destination is %s\n", IPStr);
    SendServaddr.sin_addr.s_addr=inet_addr(IPStr); 

    SendServaddr.sin_port=htons(*port_num);

    sendto(SendSockfd,sendmsg,n-4,0,(struct sockaddr *)&SendServaddr,sizeof(struct sockaddr_in ));
    
    //receiving SYN-ACK from the receiver
    int m ;
    
    header * RecvHeaderPointer = (header *) recvsendmsg;

    struct timeval tv;

    poll_fd[0].fd = SendSockfd;
    poll_fd[0].events = POLLIN|POLLPRI;
   
    i =0;
    while(1){
        //printf("Before entering session!\n");
        i = poll(poll_fd, 1, RETRANSMIT_TIMER/1000);
        //printf("after select session!\n");

        if(i==1){   
            //see a syn-ack and then stop retransmitting the syn packet 
            printf("Receive SYN-ACK, we can exit the first SYN loop now\n");
            
            break;
        }
        else{
            sendto(SendSockfd,sendmsg,n-4,0,(struct sockaddr *)&SendServaddr,sizeof(struct sockaddr_in ));
            usleep(RETRANSMIT_TIMER);
        }
    }
    while(1){
        m = recvfrom(SendSockfd,recvsendmsg,1400,0,NULL,NULL);
        int action = RecvHeaderPointer->action;
        int sequenceNumber = RecvHeaderPointer->sequenceNum;
        printf("Action is and sequence number is %d and %d and ack value is %d \n", action, sequenceNumber, update_ack);
        pthread_mutex_lock(&lock);
        if(update_ack !=1){
            if(sendmsgHeader->oldMboxLength >0 ){
                //it is the old path message and we have to make sure the new path is also set up
                old_syn_ack = 1;
                if( new_syn_ack==1 ){
                    sendto(sockfd,recvsendmsg,m,0,(struct sockaddr *) cliAddr,sizeof(struct sockaddr_in ));
                    printf("Old SYN-ACK\n");
                    if(NETLINK_FLAG){
                        char * netlink_message = "SYNACK";
                      //  send_netlink(netlink_message);
                    }
                } else{
                    printf("waiting for the new path's syn-ack being received\n");
                }
                
            } 
            else {
                new_syn_ack =1;
                if(old_syn_ack ==1){
                    sendto(sockfd,recvsendmsg,m,0,(struct sockaddr *) cliAddr,sizeof(struct sockaddr_in ));
                    printf("New SYN-ACK\n");
                    if(NETLINK_FLAG){
                        char * netlink_message = "SYNACK";
                       // send_netlink(netlink_message);
                    }
                } else{
                    printf("waiting for the old path's syn-ack being received\n");
                }
            }
        } else {
            printf("Packet is been acked, so we can exit syn-ack loop now!\n");
            break;
        }
        pthread_mutex_unlock(&lock);
    }
    pthread_mutex_unlock(&lock);
    while(1){
        //free(cliAddr);
        //the point here is to block everything and retransmit the ACK if we see an SYN-ACK
        int HeaderLength = sizeof(header)+4;
        char AckMesg[HeaderLength];
        settingAck(AckMesg, sequenceNumber);
        sendto(SendSockfd,AckMesg,HeaderLength,0,(struct sockaddr *)&SendServaddr,sizeof(struct sockaddr_in ));
        m = recvfrom(SendSockfd,recvsendmsg,1400,0,NULL,NULL);
        printf("We see retransmission of the ack packets!\n");

    }

}


void * handleACK(void * ptr){
    char syn_ack_old [sizeof(header) + 4 ];
    header * syn_ack_hdr = (header *) syn_ack_old;
    syn_ack_hdr->action =5;
    syn_ack_hdr->sequenceNum = sequenceNumber;

    syn_ack_hdr->newMboxLength = 0;
    syn_ack_hdr->oldMboxLength = 1;
    * ((int *) (syn_ack_old + sizeof(header))) = 0;
    while(1){
        sendto(sockfd, syn_ack_old, sizeof(header) + 4 , 0, (struct sockaddr *) replyAddr[0], sizeof(struct sockaddr_in ) );
        sendto(sockfd, syn_ack_old, sizeof(header) + 4 , 0, (struct sockaddr *) replyAddr[1], sizeof(struct sockaddr_in ) );

        usleep(RETRANSMIT_TIMER);
        pthread_mutex_lock(&lock);

        if(update_ack==1){
            break;
        }
        pthread_mutex_unlock(&lock);

    }
    pthread_mutex_unlock(&lock);

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
    free(ptr);

    if (n>sizeof(header) +8){
       relayMsg(request, n, &port_num, cliAddr);
    } 
    printf("debug, finish up the old thread!\n");

    return NULL;

}


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