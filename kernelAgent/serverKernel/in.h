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


struct sk_buff * udp_header_write_prerouting(struct sk_buff *skb){
    struct iphdr *iph ;
    struct udphdr *udph;
    
    //we only decapsulate the packet, right now just remodify the source


    unsigned int data_len = skb->len;

    iph = (struct iphdr *) ip_hdr (skb );

    udph = (struct udphdr *) udp_hdr (skb );

    record_t l, *p;
    memset(&l, 0, sizeof(record_t));
    
    p=NULL ;

    l.key.src = iph->saddr ;
    l.key.dport = ntohs(udph->dest) ;
    read_lock(&my_rwlock) ;
    HASH_FIND(hh, records, &l.key, sizeof( record_key_t ), p) ;
    read_unlock(&my_rwlock) ;
    if(p)
    {
        printk( KERN_ALERT "Input: found %pI4 and value is %pI4  \n", &p->key.src , &p->src ) ;
        iph->saddr = p->src ;
        return skb ;
    } else{
    	if ( ntohs(tcph->dest)  == 5001 )
			printk( KERN_ALERT "No hash found, do nothing \n") ;
    	return skb ;
    	}
    }

    return  skb ;
}

struct sk_buff * tcp_header_write_prerouting(struct sk_buff *skb){
    struct iphdr *iph ;
    struct tcphdr *tcph ;

    unsigned int data_len;
    data_len = skb->len;

    iph = (struct iphdr *) ip_hdr (skb ); 
    tcph = (struct tcphdr *) tcp_hdr (skb );
    
    bool FLAG = true;
    
    if(FLAG){
    record_t l, *p;
    memset(&l, 0, sizeof(record_t));
    p=NULL ;

    l.key.src = iph->saddr ;
    l.key.dport = ntohs(tcph->dest) ;
    read_lock(&my_rwlock) ;
    HASH_FIND(hh, records, &l.key, sizeof( record_key_t ), p) ;
    read_unlock(&my_rwlock) ;
    if(p)
    {
        printk( KERN_ALERT "Input: found %pI4 and value is %pI4  \n", &p->key.src , &p->src ) ;
        iph->saddr = p->src ;
        return skb ;
    } else{
    	if ( ntohs(tcph->dest)  == 5001 )
			printk( KERN_ALERT "No hash found, do nothing \n") ;
    	return skb ;
    	}
    }
	return skb;
}


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
        	tcp_header_write_prerouting(skb);
        }
        else  if( iph->protocol ==IPPROTO_UDP)
        {
        	udp_header_write_prerouting(skb);
        }
		return NF_ACCEPT;
    }
     return NF_ACCEPT;
}

#endif

MODULE_AUTHOR("Kelvin Zou: <xuanz@cs.princeton.edu>");
MODULE_DESCRIPTION("add ip/udp header on top of ip header");
MODULE_LICENSE("GPL");
