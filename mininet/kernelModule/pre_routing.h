#ifndef _PRE_ROUTING_H_
#define _PRE_ROUTING_H_


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/crypto.h>
#include <linux/init.h>
#include <linux/crypto.h>
#include <linux/if_packet.h>
#include <linux/scatterlist.h>
#include <linux/inet.h>
#include <linux/tcp.h>
#include <net/udp.h>
#include <net/route.h>


struct sk_buff * header_rewrite_back(struct sk_buff *skb){
    struct iphdr *iph ;
    struct udphdr *udph;
    
    //we only decapsulate the packet, right now just remodify the source


    unsigned int data_len = skb->len;

    iph = (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);

    udph = (struct udphdr *) skb_header_pointer (skb, sizeof(struct iphdr), 0, NULL);
    printk(KERN_ALERT "PRE_ROUTING: Initial udp port number is %d and %d \n", 
        ntohs(udph->source), ntohs(udph ->dest));

    size_t iphdr_len = sizeof (struct iphdr);
    size_t udphdr_len = sizeof (struct udphdr) ;
    //create new space at the head of socket buffer
    printk(KERN_ALERT "PRE_ROUTING: SWAP back to old header\n");
    skb_pull(skb, iphdr_len+udphdr_len);

    iph = (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);
    udph = (struct udphdr *) skb_header_pointer (skb, sizeof(struct iphdr), 0, NULL);
     udph->check = 0;
    skb->csum = csum_partial((char *)udph, data_len - sizeof(struct iphdr),0);
     udph->check = csum_tcpudp_magic((iph->saddr), (iph->daddr), data_len- sizeof(struct iphdr), IPPROTO_UDP,skb->csum);

    printk(KERN_ALERT "PRE_ROUTING: Packet length is %d\n\n", data_len);

    printk(KERN_ALERT "PRE_ROUTING: Src and Dest address is %pI4 and  %pI4\n", 
        &iph->saddr ,&iph->daddr );
    printk(KERN_ALERT "PRE_ROUTING: Final Packet length is %d\n", skb->len);

    return  skb ;
}

struct sk_buff * tcp_header_write_prerouting(struct sk_buff *skb){
struct iphdr *iph ;
    struct tcphdr *tcph ;

    unsigned int data_len;
    data_len = skb->len;

    iph = (struct iphdr *) ip_hdr (skb ); 
    tcph = (struct tcphdr *) tcp_hdr (skb );

   // printk(KERN_ALERT "Output: Initial tcp port number is %u and %u and %u \n", ntohs(tcph->source), ntohs(tcph ->dest),   ntohs(portvalue)  ); 
   // printk(KERN_ALERT "Output: Src and Dest address is %pI4 and  %pI4\n",   &iph->saddr ,&iph->daddr );
    unsigned int  iphdr_len;
    iphdr_len =  ip_hdrlen(skb) ;
    unsigned int   tcphdr_len;
    tcphdr_len = tcp_hdrlen(skb) ;
    unsigned int tcp_len;
    tcp_len = data_len - iphdr_len;  

   // printk(KERN_ALERT "The ip hdr address is %d and tcp addr is %d and length is %d and %d\n", iph, tcph, skb->len, tcp_len);
  printk(KERN_ALERT "Input: Initial checksum is %u and %u and %u checksum header and offset are %d and %d and %d \n", skb->csum, tcph->check,iph->check ,skb->csum_start, skb->transport_header, skb->csum_offset); 
    

    __u16 tempCheck = tcph->check; 

    tcph->check = 0;
    
    //CHECKSUM_UNNECESSARY, HW already checked the packet for you. 
    //tcph->check = ~csum_tcpudp_magic( iph->saddr, iph->daddr,tcp_len, IPPROTO_TCP, 0);

     printk(KERN_ALERT "Input: New checksum is %u and %u and %u checksum header and offset are %d and %d and %d \n", skb->csum, tcph->check,iph->check ,skb->csum_start, skb->transport_header, skb->csum_offset); 


      if(iph->saddr == in_aton("192.168.56.101")){
        iph->saddr = in_aton("192.168.56.1");
    }
    /*
    switch(skb->ip_summed){
    	case CHECKSUM_NONE:
    	printk(KERN_ALERT "CHECKSUM_NONE   \n");
    	break;
    	case CHECKSUM_PARTIAL:
    	printk(KERN_ALERT "CHECKSUM_PARTIAL  \n");
    	break;
    	case CHECKSUM_UNNECESSARY:
    	printk(KERN_ALERT "CHECKSUM_UNNECESSARY  \n");
    	break;
    } 
	*/
	return skb;
}


static unsigned int pre_routing_begin(unsigned int hooknum, 
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *)) 
{
    struct iphdr  *iph;
    struct udphdr *udph;
    __u16 dst_port, src_port;
    struct sk_buff * retv;

    if (skb) {
        iph = (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);

        //do not change any non-UDP traffic
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_UDP && iph->protocol !=IPPROTO_TCP) ) {
            return NF_ACCEPT;
        } else if( iph->protocol ==IPPROTO_TCP){
        	
        	tcp_header_write_prerouting(skb);

        	printk(KERN_ALERT "PRE_ROUTING: ever pass input check?\n");
        }
        else  if( iph->protocol ==IPPROTO_UDP)
        {
            udph = (struct udphdr *) skb_header_pointer (skb, sizeof(struct iphdr), 0, NULL);
            src_port = ntohs (udph->source);
            dst_port = ntohs (udph->dest);
        }
    }
     return NF_ACCEPT;
}

#endif

MODULE_AUTHOR("Kelvin Zou: <xuanz@cs.princeton.edu>");
MODULE_DESCRIPTION("add ip/udp header on top of ip header");
MODULE_LICENSE("GPL");