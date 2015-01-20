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


static unsigned int incoming_begin(unsigned int hooknum, 
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *)) 
{
    struct iphdr  *iph;
    __u16 dst_port, src_port;

    if (skb) {
        iph = (struct iphdr *) ip_hdr (skb);

        //do not change any non-TCP traffic
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_UDP && iph->protocol !=IPPROTO_TCP) ) {
            return NF_ACCEPT;
        } 
        else if( iph->protocol ==IPPROTO_TCP){
		    struct tcphdr *tcph ;

		    unsigned int data_len;
		    data_len = skb->len;
		    tcph = tcp_hdr (skb );
		    __u16 iphdr_len ;
            iphdr_len =  ip_hdrlen(skb) ;
            unsigned int tcphdr_len ;
            tcphdr_len = tcp_hdrlen(skb) ;
            unsigned int tcp_len ;
            tcp_len = data_len - iphdr_len ;  
            
            
            iph->tot_len = htons( data_len - 40 );
            
            tcph->check = 0;
            tcph->check = ~csum_tcpudp_magic( iph->saddr, iph->daddr,tcp_len-40, IPPROTO_TCP, 0);
   	        ip_send_check(iph);
                    
			return NF_ACCEPT;
        }
     
		return NF_ACCEPT;
    }
     return NF_ACCEPT;
}

#endif

MODULE_AUTHOR("Kelvin Zou: <xuanz@cs.princeton.edu>");
MODULE_DESCRIPTION("add ip/udp header on top of ip header");
MODULE_LICENSE("GPL");
