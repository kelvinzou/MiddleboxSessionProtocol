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

int main(int argc, char *argv[]) {
    //init all the headers
    struct iphdr *ip;
    struct tcphdr *tcp;
    struct sockaddr_in source,dest;

    int sock, on = 1;
    

    //build socket
    //sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
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

    return 0;
}
