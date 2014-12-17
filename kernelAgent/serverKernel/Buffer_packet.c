/*
Kelvin Xuan Zou

Princeotn university

This is the user space agent of the middlebox protocol
*/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <time.h> 
#include <string.h>    //memset
#include <signal.h>
#include <poll.h>
#include <linux/genetlink.h>

//#include <queue>
#include "queue.h"

#define TCP_FLAG false
#define NETLINK_USER 31
#define MAX_PAYLOAD 1024 /* maximum payload size*/
#define MTU 65536

struct sockaddr_nl netlink_src, netlink_dest;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int netlink_socket_fd;
struct msghdr netlink_msg;


struct iphdr *ip;
struct tcphdr *tcp;
struct sockaddr_in source, dest;
int sock, sendSocket, on = 1, one=1;
volatile bool flag = true;
volatile bool releaseFlag = false;
/* 
    96 bit (12 bytes) pseudo header needed for tcp header checksum calculation 
*/

struct pseudo_header
{
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t tcp_length;
};

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
    Generic checksum calculation function
*/

unsigned short csum(unsigned short *ptr,int nbytes) 
{
    register long sum;
    unsigned short oddbyte;
    register short answer;
    sum=0;
    while(nbytes>1) {
        sum+=*ptr++;
        nbytes-=2;
    }
    if(nbytes==1) {
        oddbyte=0;
        *((u_char*)&oddbyte)=*(u_char*)ptr;
        sum+=oddbyte;
    }
    sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);
    answer=(short)~sum;
    return(answer);
}

void sendback(char * recv_buffer,  unsigned int ipaddr, struct iphdr * ip, struct tcphdr * tcp) {

    printf("We send the traffic via sendSocket\n");
    
    int total_len = ntohs(ip->tot_len);
    int tcpopt_len = tcp->doff*4 - 20;
    int tcpdatalen = total_len - (tcp->doff*4) - (ip->ihl*4);

    struct pseudo_header psh;
    
    ip->daddr  = ipaddr;// inet_addr("128.112.93.106");
    ip->protocol = IPPROTO_TCP;
    ip->check = csum( (unsigned short *) recv_buffer, ntohs(ip->tot_len) );
    
    memset(&dest, 0, sizeof(struct sockaddr_in));
    dest.sin_addr.s_addr = ip->daddr ;

    psh.source_address = ip->saddr;
    psh.dest_address = ip->daddr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    
    psh.tcp_length =htons(sizeof(struct tcphdr) + tcpopt_len + tcpdatalen);

    int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + tcpopt_len + tcpdatalen;

    void * pseudogram  = malloc(psize);
    tcp->check = 0;
    
    memcpy((char *)pseudogram, (char*) & psh, sizeof(struct pseudo_header));
    
    memcpy((char*) (pseudogram + sizeof(struct pseudo_header) ), (char * )tcp, sizeof(struct tcphdr) + tcpopt_len + tcpdatalen );
    
    tcp->check = (csum( (unsigned short*) pseudogram , psize) )  ;
    
    printf("The total size is %d\n",  ntohs(ip->tot_len));

    free(pseudogram);

    if (sendto (sock, recv_buffer,ntohs(ip->tot_len) ,  0, (struct sockaddr *) &dest, sizeof (dest)) < 0)
    {
        perror("sendto failed");
        printf ("Packet Send. Length : %d \n" , ntohs(ip->tot_len));
    }
    //Data send successfully
    else
    {
        printf ("Packet Send. Length : %d \n" , ntohs(ip->tot_len));
    }
}

void reinjectBuffer(queue * Q ){
    while (Q->size >0){
        char * packet = dequeue(Q);

        struct iphdr *ip = (struct iphdr *) packet;
        unsigned short  iphdrlen =ip->ihl*4;
        struct tcphdr *tcp = ( struct tcphdr *) ( packet + iphdrlen );
        sendback( packet, inet_addr("128.112.93.106"), ip, tcp );
        free (packet);
    }
}

void sig_handler(int signo)
{
  if (signo == SIGINT)
  {
    printf("received SIGINT\n");
    flag =false;
  }
}


int main(int argc, char *argv[]) {

    if (signal(SIGINT, sig_handler) == SIG_ERR)
    { 
        printf("\ncan't catch SIGINT\n");
    }
    init_netlink();

    //init all the headers
    queue BufferedQueue;

    queue_init(&BufferedQueue);


    sendSocket = socket (PF_INET, SOCK_RAW, IPPROTO_TCP) ;
     if (setsockopt (sendSocket, IPPROTO_IP, IP_HDRINCL, &one, sizeof (one)) < 0)
    {
        perror("Error setting IP_HDRINCL");
        exit(0);
    }

    //build socket
    if(TCP_FLAG){
        sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    } else{
        sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    }
    
    if (sock == -1) {
        perror("socket() failed");
        return 1;
    }else{
        printf("socket() ok\n");
    }

    //&on is to set the SO_REUSEADDR to true
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) == -1) {
        perror("setsockopt() failed");
        return 2;
    }else{
        printf("setsockopt() ok\n");
    }

    //select initilize, to avoid recvfrom blocking call
    struct timeval tv;
    fd_set readfds, active_fs;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
      /*  
        active_fs = readfds;
        select(sock+1, &active_fs, NULL, NULL, &tv);
        
        if(FD_ISSET(sock, &active_fs)){
*/
    char * recv_buffer;
    int select_print =1;

	while(flag){
        struct sockaddr_in from;
        socklen_t fromlen;

        fromlen = sizeof from;

        recv_buffer = (char *) malloc(MTU);
        recvfrom(sock, recv_buffer, MTU, 0,(struct sockaddr*) &from, &fromlen);
        printf("\nThe source address for receive from is %s\n", inet_ntoa( *(struct in_addr* ) &from.sin_addr.s_addr));
       

        //recvfrom(sock, recv_buffer, sizeof(recv_buffer), 0,NULL, NULL);
		struct iphdr *ip = (struct iphdr *) recv_buffer;

        memset(&source, 0, sizeof(source));
        source.sin_addr.s_addr = ip->saddr;
         
        memset(&dest, 0, sizeof(dest));
        dest.sin_addr.s_addr = ip->daddr;
        unsigned short  iphdrlen =ip->ihl*4;
        struct tcphdr *tcp = (struct tcphdr *) (recv_buffer + iphdrlen);

        
        if(ntohs(tcp->source) ==5001){
            //check the urgent pointer
            __u16  reserved_field =  * (__u16 *) (((char *) tcp) + 12);
            printf("The reserved_field is %x\n", reserved_field);

            __u16  result = reserved_field & 0x2000 ;
            if(result == 0x2000){
                printf("Urgent packet \n");
                flag = false;
                reserved_field = reserved_field & 0xdfff;
                //reset the urgent pointer
                printf("The reserved_field is %x\n", reserved_field);

                __u16 * value  = (__u16 *) (((char *) tcp) + 12);
                * value = reserved_field;
            }
            

            enqueue(&BufferedQueue, recv_buffer);
            printf("The item number is %d\n", BufferedQueue.size);
            if(!flag){
                break;
            }
            
        } else{
            free(recv_buffer);
        }
           
	} 
    if(BufferedQueue.size >=0){
            reinjectBuffer( & BufferedQueue);
            printf("Notify the kernel to release the lock!\n");
            char * netlink_message = "UNLOCK";
            send_netlink(netlink_message);
    }
     
    return 0;
}
