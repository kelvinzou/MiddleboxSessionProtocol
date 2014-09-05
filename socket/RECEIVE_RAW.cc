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

int main(int argc, char *argv[]) {

    int sock, bytes, on = 1;
    char buffer[1024];
    struct iphdr *ip;
    struct tcphdr *tcp;
    struct sockaddr_in to;
    struct pseudohdr pseudoheader;
    struct data_4_checksum tcp_chk_construct;
    socklen_t fromlen;
    fromlen = sizeof to;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s ", argv[0]);
        fprintf(stderr, "<dest-addr>\n");
        return 1;
    }

    sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock == -1) {
        perror("socket() failed");
        return 1;
    }else{
        printf("socket() ok\n");
    }

    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) == -1) {
        perror("setsockopt() failed");
        return 2;
    }else{
        printf("setsockopt() ok\n");
    }

    ip = (struct iphdr*) buffer;
    tcp = (struct tcphdr*) (buffer + sizeof(struct iphdr));

    int iphdrlen = sizeof(struct iphdr);
    int tcphdrlen = sizeof(struct tcphdr);
    int datalen = 0;
    printf("Typecasting ok\n");

    ip->frag_off = 0;
    ip->version = 4;
    ip->ihl = 5;
    ip->tot_len = htons(iphdrlen + tcphdrlen);
    ip->id = 0;
    ip->ttl = 40;
    ip->protocol = IPPROTO_TCP;
    ip->saddr = inet_addr("127.0.0.1");
    ip->daddr = inet_addr(argv[1]);
    ip->check = 0;

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
	while(1){
		//recvfrom(sock, buffer, sizeof(buffer), 0,(struct sockaddr*) &to, &fromlen);
		recvfrom(sock, buffer, sizeof(buffer), 0,NULL, NULL);
		struct iphdr *ip = (struct iphdr *) buffer;
		struct tcphdr *tcp = (struct tcphdr *) (buffer + sizeof (struct ip));
		 printf("TTL= %d\n", ip->ttl);
	    printf("Window= %d\n", tcp->window);
	    printf("ACK= %d\n", tcp->ack);
	    printf("%s:%d\t --> \t%s:%d \tSeq: %d \tAck: %d\n",
                    inet_ntoa(*(struct in_addr*) &ip->saddr), ntohs(tcp->source),
                    inet_ntoa(*(struct in_addr *) &ip->daddr), ntohs(tcp->dest),
                    ntohl(tcp->seq), ntohl(tcp->ack_seq));

		//printf("%d", buffer);
	}
    
    printf("TTL= %d\n", ip->ttl);
    printf("Window= %d\n", tcp->window);
    printf("ACK= %d\n", tcp->ack);
    printf("%s:%d\t --> \t%s:%d \tSeq: %d \tAck: %d\n",
                    inet_ntoa(*(struct in_addr*) &ip->saddr), ntohs(tcp->source),
                    inet_ntoa(*(struct in_addr *) &ip->daddr), ntohs(tcp->dest),
                    ntohl(tcp->seq), ntohl(tcp->ack_seq));

    return 0;
}
