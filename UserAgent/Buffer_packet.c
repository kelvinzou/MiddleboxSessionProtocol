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


#include "queue.h"

#define TCP_FLAG false

 
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


int main(int argc, char *argv[]) {
    //init all the headers
    struct iphdr *ip;
    struct tcphdr *tcp;
    struct sockaddr_in source, dest;
    struct pseudo_header psh;

    int sock, on = 1, one=1;
    
    int sendSocket = socket (PF_INET, SOCK_RAW, IPPROTO_TCP) ;

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

    char recv_buffer[65536];
	while(1){
		//sleep(1000);
		
        struct sockaddr_in from;
        socklen_t fromlen;
        fromlen = sizeof from;
        recvfrom(sock, recv_buffer, sizeof(recv_buffer), 0,(struct sockaddr*) &from, &fromlen);
       

        //recvfrom(sock, recv_buffer, sizeof(recv_buffer), 0,NULL, NULL);
		struct iphdr *ip = (struct iphdr *) recv_buffer;

        memset(&source, 0, sizeof(source));
        source.sin_addr.s_addr = ip->saddr;
         
        memset(&dest, 0, sizeof(dest));
        dest.sin_addr.s_addr = ip->daddr;
        unsigned short  iphdrlen =ip->ihl*4;
        struct tcphdr *tcp = (struct tcphdr *) (recv_buffer + iphdrlen);
        if(ntohs(tcp->dest) ==5001){
            printf("\nThe source address for receive from is %s\n", inet_ntoa( *(struct in_addr* ) &from.sin_addr.s_addr));
            printf("TTL= %d\n", ip->ttl);
            printf("Window= %d\n", tcp->window);
            printf("ACK= %lu\n", tcp->ack);
            printf("IP field the total length is %d\n", ntohs(ip->tot_len)) ;
            printf("%s:%d\t",  inet_ntoa(source.sin_addr), ntohs(tcp->source)); 
            printf(" --> \t%s:%d \tSeq: %lu \tAck: %lu\n", inet_ntoa(dest.sin_addr), ntohs(tcp->dest), ntohl(tcp->seq), ntohl(tcp->ack_seq));

            printf("We send the traffic via sendSocket\n");
            
            //seems like we dont need to redo checksum for the TCP header, but we do need to redo it for the IP header.

            ip->daddr  =  inet_addr("128.112.93.106");
            ip->protocol = IPPROTO_TCP;
            ip->check = csum( (unsigned short *) recv_buffer, ntohs(ip->tot_len) );
            memset(&dest, 0, sizeof(dest));
            dest.sin_addr.s_addr = ip->daddr ;


            psh.source_address = ip->saddr;
            psh.dest_address = ip->daddr;
            psh.placeholder = 0;
            psh.protocol = IPPROTO_TCP;
            psh.tcp_length = ntohs(ip->tot_len)- sizeof(struct iphdr);
            int psize = sizeof(struct pseudo_header) + ntohs(ip->tot_len) - sizeof(struct iphdr);
            void * pseudogram  = malloc(psize);
            
            memcpy((char *)pseudogram, (char*) & psh, sizeof(struct pseudo_header));
            memcpy((char*)pseudogram+sizeof(struct pseudo_header), tcp, ntohs(ip->tot_len) - sizeof(struct iphdr) );
            tcp->check = csum((unsigned short*) pseudogram , psize);
            free(pseudogram);

            if (sendto (sendSocket, recv_buffer,ntohs(ip->tot_len) ,  0, (struct sockaddr *) &dest, sizeof (dest)) < 0)
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
       
        //printf("dest IP is %s and the port number is %d\n",inet_ntoa(dest.sin_addr), ntohs(tcp->dest));
	}

    return 0;
}
