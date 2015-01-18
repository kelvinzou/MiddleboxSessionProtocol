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

#include <netinet/in.h>
#include <linux/netfilter.h>        /* for NF_ACCEPT */
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <libnetfilter_queue/libnetfilter_queue.h>

#define NETLINK_USER 31

#define MAX_PAYLOAD 1024 /* maximum payload size*/

#define UDP_PORT 1025

#define NETLINK_FLAG true



/*Netfilter queue*/

/********************************************/
struct nfq_handle *h;
struct nfq_q_handle *qh;
struct nfnl_handle *nh;
int nf_queue_fd;
int recvCount;
char buf[65536] __attribute__((aligned));
volatile int nfq_flag = 0;

pthread_mutex_t buffer_lock;

/********************************************/

/*this is netlink variables*/

struct sockaddr_nl netlink_src, netlink_dest;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int netlink_socket_fd;
struct msghdr netlink_msg;

int first_syn;
int first_ack;

/*This is for netilter queue call back*/

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
              struct nfq_data *nfa, void *data);

void nfq_init();

/*
This building block is for netlink

*/
int netlink_init();
int send_netlink(char * input);

void * release_queue(void * ptr);


/*
end of netlink building block
*/




int main(int argc, char**argv){
    netlink_init();
    nfq_init();
    if (pthread_mutex_init(&buffer_lock, NULL) != 0) {
        printf("\n mutex init failed\n");
        return 1;
    }
    
    pthread_t releaseQueue_thread;

    
    


    /*
    this is the last hop during an update
    here we do not have any argument since just goes to the next hop
    */

    if (argc<2){
       
        struct timeval t1, t2;
        double elapsedTime;
        int sockfd;
        sockfd=socket(AF_INET,SOCK_DGRAM,0);
        struct sockaddr_in servaddr;
        bzero(&servaddr,sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
        servaddr.sin_port=htons(UDP_PORT);
        bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

        
        char mesg[4];
        struct sockaddr_in clientAddressPtr;
        socklen_t len = sizeof(struct sockaddr_in) ;
       
        recvfrom(sockfd,mesg,4,0,(struct sockaddr *) & clientAddressPtr,&len);
        gettimeofday(&t1, NULL);
    
        if (first_syn==0)
        {
            first_syn=1;
            pthread_mutex_lock(&buffer_lock);
            if(NETLINK_FLAG){
                char * netlink_message = "SYN";
                send_netlink(netlink_message);
            }
            printf("SYN\n");
        }
            
        sendto(sockfd, mesg, 4, 0 , (struct sockaddr *)&clientAddressPtr,sizeof(struct sockaddr_in ));
        
        recvfrom(sockfd,mesg,4,0,(struct sockaddr *) & clientAddressPtr,&len);
        
        if(first_ack ==0)
        {
            printf("ACK\n");
            first_ack=1;
            pthread_mutex_unlock(&buffer_lock);
            
            if(NETLINK_FLAG){
                char * netlink_message = "ACK";
                send_netlink(netlink_message);
            }
        }
        
        
        gettimeofday(&t2, NULL);
        elapsedTime =(t2.tv_usec - t1.tv_usec) + (t2.tv_sec - t1.tv_sec)*1000000;
		printf("Time is %f\n",elapsedTime);     
    
    
    } 
    
    /*
    Here we relay the messages
    We just need to know the next hop    
    */
    else if(argc <3) {
        struct timeval t1, t2;
        double elapsedTime;
        gettimeofday(&t1, NULL);
        //receive first        
        int sockfd;
        sockfd=socket(AF_INET,SOCK_DGRAM,0);
        struct sockaddr_in servaddr;
        bzero(&servaddr,sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
        servaddr.sin_port=htons(UDP_PORT);
        bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
        
        char mesg[4];
        struct sockaddr_in clientAddressPtr;
        socklen_t len = sizeof(struct sockaddr_in) ;
        
        int * counter = (int *) mesg;
        printf("counter is %d\n", *counter);
        
        //this is the sending hop during an update        
        struct sockaddr_in SendServaddr, SendCliaddr;
        struct in_addr addr;
        int SendSockfd ;
        SendSockfd=socket(AF_INET,SOCK_DGRAM,0);
        bzero(&SendServaddr,sizeof(SendServaddr));
        printf("The SendSockfd is %d \n", SendSockfd);
        SendServaddr.sin_family = AF_INET;
        inet_aton(argv[1], &addr);
        
        char * IPStr = inet_ntoa(addr);
        
        printf("the destination is %s\n", IPStr);
        SendServaddr.sin_addr.s_addr=inet_addr(IPStr); 

        SendServaddr.sin_port=htons(UDP_PORT);
        recvfrom(sockfd,mesg,4,0,(struct sockaddr *) & clientAddressPtr,&len);
        
        sendto(SendSockfd,mesg,4,0,(struct sockaddr *)&SendServaddr,sizeof(struct sockaddr_in ));
        recvfrom(SendSockfd,mesg,4,0,NULL,NULL);
        
        sendto(sockfd, mesg, 4, 0 , (struct sockaddr *)&clientAddressPtr,sizeof(struct sockaddr_in ));
        recvfrom(sockfd,mesg,4,0,(struct sockaddr *) & clientAddressPtr,&len);
        
        sendto(SendSockfd,mesg,4,0,(struct sockaddr *)&SendServaddr,sizeof(struct sockaddr_in ));
            
    } 
    
    
    
    
    /*
    starting point
    We only need to specify the next hop, 
    add a junk argument to tell that it is not the relay hops 
    */
    else {
        //this is the first hop, which will be used to deal with buffered packets in current setting.
        
        int sockfd,n;
	    struct sockaddr_in servaddr;
        struct timeval t1, t2;
	    sockfd=socket(AF_INET,SOCK_DGRAM,0);

	    bzero(&servaddr,sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_addr.s_addr=inet_addr(argv[1]);
	    servaddr.sin_port=htons(UDP_PORT);

	    double elapsedTime;
	    int flag =0;

	    //blocking way, it measures the latency in a better way!
	    char sendline[4];
	    char recvline[4];
	    memset(sendline, 0,4);
	    gettimeofday(&t1, NULL);
	    //send the first time, SYN
	    
	    if (first_syn==0)
        {
            first_syn=1;
            pthread_mutex_lock(&buffer_lock);
            if(NETLINK_FLAG){
                char * netlink_message = "SYN";
                send_netlink(netlink_message);
            }
            printf("SYN\n");
        }
        
	    sendto(sockfd,sendline,4,0,(struct sockaddr *)&servaddr,sizeof(struct sockaddr_in ));


	    pthread_create(&releaseQueue_thread, NULL, release_queue , NULL);
	    

        //receive the first time, SYNACK
        recvfrom(sockfd,recvline,4,0,NULL,NULL);
        
        //send ACK
        if(first_ack ==0)
        {
            printf("ACK\n");
            first_ack=1;
            pthread_mutex_unlock(&buffer_lock);
            
            if(NETLINK_FLAG){
                char * netlink_message = "ACK";
                send_netlink(netlink_message);
            }
        }
        
        //this should wait for the response


        sendto(sockfd,sendline,4,0,(struct sockaddr *)&servaddr,sizeof(struct sockaddr_in ));
        
        gettimeofday(&t2, NULL);
        elapsedTime =(t2.tv_usec - t1.tv_usec) + (t2.tv_sec - t1.tv_sec)*1000000;
		printf("Time is %f\n",elapsedTime);

        pthread_join(releaseQueue_thread, NULL);

    }
    
    
}

//This is the netfilter queue management thread

void * release_queue (void * ptr){
    int packet_counter =0;
    if(NETLINK_FLAG){
        pthread_mutex_lock(&buffer_lock);
        pthread_mutex_unlock(&buffer_lock);

        while ((recvCount = recv(nf_queue_fd, buf, sizeof(buf), 0))) 
        {
            packet_counter ++;
            printf("pkt received %d\n", packet_counter);
            if(recvCount <0){
                printf("Error with packet loss in the queue\n");
            } else{
                nfq_handle_packet(h, buf, recvCount);
            }
        }
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
    if(ph){
        printf("entering callback\n");
        id = ntohl(ph->packet_id);
        return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
    }
    
    
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

    if (nfq_set_mode(qh, NFQNL_COPY_META, 0xffff) < 0) {
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


