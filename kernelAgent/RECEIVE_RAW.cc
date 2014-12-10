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

struct pseudohdr {
    u_int32_t src_addr;
    u_int32_t dst_addr;
    u_int8_t padding;
    u_int8_t proto;
    u_int16_t length;
};

struct data_4_checksum {
    struct pseudohdr pshd;
    struct tcphdr tcphdr;
    char payload[1024];
};

unsigned short comp_chksum(unsigned short *addr, int len) {
    long sum = 0;
    while (len > 1) {
        sum += *(addr++);
        len -= 2;
    }
    if (len > 0)
        sum += *addr;
    while (sum >> 16)
        sum = ((sum & 0xffff) + (sum >> 16));
    sum = ~sum;
    return ((u_short) sum);

}

unsigned short csum(unsigned short *buf, int nwords)
{       //
        unsigned long sum;
        for(sum=0; nwords>0; nwords--)
                sum += *buf++;
        sum = (sum >> 16) + (sum &0xffff);
        sum += (sum >> 16);
        return (unsigned short)(~sum);
}

int main(int argc, char *argv[]) {
    //init all the headers
    struct iphdr *ip;
    struct tcphdr *tcp;
    struct sockaddr_in to;
    struct pseudohdr pseudoheader;
    struct data_4_checksum tcp_chk_construct;
    struct sockaddr_in source,dest;

    int sock, bytes, on = 1;
    char buffer[1024];
    

    //build socket
    //sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
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
/*
    ip = (struct iphdr*) buffer;
    tcp = (struct tcphdr*) (buffer + sizeof(struct iphdr));

    int iphdrlen = sizeof(struct iphdr);
    int tcphdrlen = sizeof(struct tcphdr);
    int datalen = 0;



    // building IP header
    ip->frag_off = 0;
    ip->version = 4;
    ip->ihl = 5;
    ip->tot_len = htons(1024);
    ip->id = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_TCP;
    ip->saddr = inet_addr("127.0.0.1");
    ip->daddr = inet_addr("127.0.0.1");
    //done by knernel for TCP's checksum
    ip->check =  0;


    // build TCP header
    tcp->source     = htons(80);
    tcp->dest       = htons(1234);
    tcp->seq        = random();
    tcp->doff       = 5;
    tcp->ack        = 0;
    tcp->psh        = 0;
    tcp->rst        = 0;
    tcp->urg        = 0;
    tcp->syn        = 1;
    tcp->fin        = 0;
    tcp->window     = htons(65535);

    //consrtuct pseudoheader and then use it to construct check_sum
    pseudoheader.src_addr = ip->saddr;
    pseudoheader.dst_addr = ip->daddr;
    pseudoheader.padding = 0;
    pseudoheader.proto = ip->protocol;
    pseudoheader.length = htons(tcphdrlen + datalen);

    tcp_chk_construct.pshd = pseudoheader;
    tcp_chk_construct.tcphdr = *tcp;

    int checksum = comp_chksum((unsigned short*) &tcp_chk_construct,
            sizeof(struct pseudohdr) + tcphdrlen + datalen);
    tcp->check = checksum;


    printf("TCP Checksum: %i\n", checksum);
    printf("Destination : %i\n", ntohs(tcp->dest));
    printf("Source: %i\n", ntohs(tcp->source));

    to.sin_addr.s_addr = ip->daddr;
    to.sin_family = AF_INET;
    to.sin_port = tcp->dest;

    //bytes = sendto(sock, buffer, ntohs(ip->tot_len), 0, (struct sockaddr*) &to,sizeof(to));
    /*
    if (bytes == -1) {
        perror("sendto() failed");
        return 1;
    }
	*/
    char recv_buffer[65536];
	while(1){
		//sleep(1000);
		
        struct sockaddr_in from;
        socklen_t fromlen;
        fromlen = sizeof from;
        recvfrom(sock, recv_buffer, sizeof(recv_buffer), 0,(struct sockaddr*) &from, &fromlen);
        printf("\n\nThe source address for receive from is %s\n", inet_ntoa( *(struct in_addr* ) &from.sin_addr.s_addr));

        //recvfrom(sock, recv_buffer, sizeof(recv_buffer), 0,NULL, NULL);
		struct iphdr *ip = (struct iphdr *) recv_buffer;

        memset(&source, 0, sizeof(source));
        source.sin_addr.s_addr = ip->saddr;
         
        memset(&dest, 0, sizeof(dest));
        dest.sin_addr.s_addr = ip->daddr;

        if (ip->protocol == IPPROTO_TCP){
            unsigned short  iphdrlen =ip->ihl*4;
            struct tcphdr *tcp = (struct tcphdr *) (recv_buffer + iphdrlen);
            printf("TTL= %d\n", ip->ttl);
            printf("Window= %d\n", tcp->window);
            printf("ACK= %lu\n", tcp->ack);
            printf("IP field the total length is %d\n", ntohs(ip->tot_len)) ;
            printf("%s:%d\t",  inet_ntoa(source.sin_addr), ntohs(tcp->source)); 
            printf(" --> \t%s:%d \tSeq: %lu \tAck: %lu\n", inet_ntoa(dest.sin_addr), ntohs(tcp->dest), ntohl(tcp->seq), ntohl(tcp->ack_seq));
            //printf("dest IP is %s and the port number is %d\n",inet_ntoa(dest.sin_addr), ntohs(tcp->dest));
        }
		//printf("%d", buffer);
	}

    return 0;
}
