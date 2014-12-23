
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <linux/netfilter.h>        /* for NF_ACCEPT */
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <libnetfilter_queue/libnetfilter_queue.h>

#include <linux/genetlink.h>
#include <time.h> 

#define NETLINK_USER 31

#define MAX_PAYLOAD 1024 /* maximum payload size*/


/********************************************/
struct nfq_handle *h;
struct nfq_q_handle *qh;
struct nfnl_handle *nh;
int nf_queue_fd;
int recvCount;
char buf[65536] __attribute__((aligned));

/********************************************/















struct sockaddr_nl netlink_src, netlink_dest;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int netlink_socket_fd;
struct msghdr netlink_msg;

volatile int nfq_flag = 0;

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
            int retv = nfq_set_verdict(qh, id, NF_ACCEPT,0, NULL);
            //unlock somethere here
            printf("send to kernel to unlock\n");
           
            char * netlink_message = "UNLOCK";

            send_netlink(netlink_message);
           return retv;
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

void sendback_packet(){
    int packet_counter =0;
    int sent =0;
    while ((recvCount = recv(nf_queue_fd, buf, sizeof(buf), 0)) && recvCount >= 0) {
        packet_counter ++;
        printf("pkt received %d\n", packet_counter);
        nfq_handle_packet(h, buf, recvCount);
        if(nfq_flag==1 &&sent==0){
            printf("send ack!\n");
            char * netlink_message = "ACK";
            send_netlink(netlink_message);
            sent =1;

        }
    }
}



int main(int argc, char **argv)
{

    struct timeval t1, t2;
    init_netlink();
    int packet_counter =0;
    nfq_init();

    
    double elapsedTime;
    gettimeofday(&t1, NULL);
    if ((recvCount = recv(nf_queue_fd, buf, sizeof(buf), 0)) && recvCount >= 0) {
        printf("pkt received\n");
        usleep(10000);
        char * netlink_message = "ACK";
        send_netlink(netlink_message);
        nfq_handle_packet(h, buf, recvCount);
    }
    
    
    sendback_packet();

    printf("send syn-ack\n");
    printf("unbinding from queue 0\n");
    nfq_destroy_queue(qh);

    printf("closing library handle\n");
    nfq_close(h);

    exit(0);
}
