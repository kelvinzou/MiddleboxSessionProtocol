/*
    Raw TCP packets
    Kelvin Zou
    Part of the code is from  Silver Moon (m00n.silv3r@gmail.com)
*/
#include <stdio.h> //for printf
#include <string.h> //memset
#include <sys/socket.h>    //for socket ofcourse
#include <stdlib.h> //for exit(0);
#include <errno.h> //For errno - the error number
#include <netinet/tcp.h>   //Provides declarations for tcp header
#include <netinet/udp.h>   //Provides declarations for tcp header

#include <netinet/ip.h>    //Provides declarations for ip header
#include <arpa/inet.h>
#include <time.h> 
#include <unistd.h>

typedef unsigned short u16;
typedef unsigned long u32;
/*
    Generic checksum calculation function
*/
unsigned short csum(unsigned short *buf, int nwords)
{       //
        unsigned long sum;
        for(sum=0; nwords>0; nwords--)
                sum += *buf++;
        sum = (sum >> 16) + (sum &0xffff);
        sum += (sum >> 16);
        return (unsigned short)(~sum);
}


u16 udp_sum_calc(u16 len_udp, u16 * src_addr,u16 * dest_addr, u16 * buff)
{
    u16 prot_udp=17;
    u16 word16;
    u32 sum;    
    int i;
    //initialize sum to zero
    sum=0;
    
    // make 16 bit words out of every two adjacent 8 bit words and 
    // calculate the sum of all 16 vit words
    for (i=0;i<len_udp;i=i+2){
        word16 =((*(buff+i)<<8)&0xFF00 )+(*(buff+i+1) & 0xFF);
        sum = sum + (unsigned long)word16;
    }   
    // add the UDP pseudo header which contains the IP source and destinationn addresses
    for (i=0;i<4;i=i+2){
        word16 =((*(src_addr +i)<<8)&0xFF00)+(*(src_addr+i+1)&0xFF);
        sum=sum+word16; 
    }
    for (i=0;i<4;i=i+2){
        word16 =((*(dest_addr+i) <<8)&0xFF00)+(*(dest_addr+i+1)&0xFF);
        sum=sum+word16;     
    }
    // the protocol number and the length of the UDP packet
    sum = sum + prot_udp + len_udp;

    // keep only the last 16 bits of the 32 bit calculated sum and add the carries
    while (sum>>16)
        sum = (sum & 0xFFFF)+(sum >> 16);
        
    // Take the one's complement of sum
    sum = ~sum;

return ((u16) sum);
}


int main (int argc, char * argv[])
{
    //Create a raw socket
    if(argc != 5)
    {
    printf("- Invalid parameters!!!\n");
    printf("- Usage %s <source hostname/IP> <source port> <target hostname/IP> <target port>\n", argv[0]);
    exit(-1);
    }
    
     
    //Datagram to represent the packet
    char datagram[4096], *data ;
    struct sockaddr_in sin, din;
    //zero out the packet buffer
    memset (datagram, 0, 4096);
     
    //IP header
    struct iphdr *iph = (struct iphdr *) datagram;
     
    //TCP header
    struct udphdr *udph = (struct udphdr *) (datagram + sizeof (struct iphdr));

    //pseudoheader is for checksum
     
    data = datagram + sizeof(struct iphdr) + sizeof(struct udphdr);
    memset(data, 'a', 1023);
    memset(data+1023, '\n', 1);
    //some address resolution
    sin.sin_family = AF_INET;
    din.sin_family = AF_INET;
    // Port numbers
    sin.sin_port = htons(atoi(argv[2]));
    din.sin_port = htons(atoi(argv[4]));
    // IP addresses
    sin.sin_addr.s_addr = inet_addr(argv[1]);
    din.sin_addr.s_addr = inet_addr(argv[3]);
     
    //Fill in the IP Header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof (struct iphdr) + sizeof (struct udphdr) + strlen(data);
    iph->id = htonl (54321); //Id of this packet
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = IPPROTO_UDP;
    iph->check = 0;      //Set to 0 before calculating checksum
    iph->saddr = inet_addr (argv[1]);   
    iph->daddr = inet_addr (argv[3]); 
     
    //UDP header
    udph->source = htons (atoi(argv[2]));
    udph->dest = htons (atoi(argv[4]));
    //udph->check = 0; //leave checksum 0 now, filled later by pseudo header
    udph->len =  sizeof(struct udphdr) + strlen(data);;

    //Ip checksum
    iph->check = csum ((unsigned short *) datagram, sizeof(struct iphdr));
   // u16 sourceIP[2], destIP[2];

    //udph->check = udp_sum_calc( sizeof(struct udphdr)+strlen(data), (u16*) &iph->saddr, (u16*)&iph->daddr, (u16*) udph );


    int s = socket (PF_INET, SOCK_RAW, IPPROTO_UDP);
    
    if(s == -1)
    {
        //socket creation failed, may be because of non-root privileges
        perror("Failed to create socket");
        exit(1);
    }

    //IP_HDRINCL to tell the kernel that headers are included in the packet
    int one = 1;
    const int *val = &one;
     
    if (setsockopt (s, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)
    {
        perror("Error setting IP_HDRINCL");
        exit(0);
    }
     
    //loop if you want to flood :)
    while (1)
    {
        //Send the packet
        sleep(1);

        if (sendto (s, datagram, iph->tot_len ,  0, (struct sockaddr *) & sin, sizeof (sin)) < 0)
        {
            perror("sendto failed");
        }
        //Data send successfully
        else
        {
            printf ("Packet Send. Length : %d \n" , iph->tot_len);
        }
    }
     
    return 0;
}
 
//Complete