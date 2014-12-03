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
    p=NULL;
    //get_random_bytes ( &i, sizeof (int) );
    l.key.src =iph->saddr;
    l.key.sport = ntohs(tcph->source) ;
    HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);
    if(p)
    {
        printk( KERN_ALERT "Source: found %pI4 and value is %pI4  \n", &p->key.src , &p->src);
        __be32 oldIP = iph->saddr;
        iph->saddr = p->src;
        __be32 newIP = iph->saddr;
        inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
        csum_replace4(&iph->check, oldIP, newIP);

        return skb;

    }
    //get_random_bytes ( &i, sizeof (int) );

    memset(&l, 0, sizeof(record_t));
    p=NULL;
    l.key.dst =iph->daddr;
    l.key.dport = ntohs(tcph->dest) ;
    HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);
    if (p){
        
        //the following is the header rewriting

         if (unlikely(skb_linearize(skb) != 0))
            return NULL;

        __be32 oldIP = iph->daddr;
        iph->daddr = p->dst;
        __be32 newIP = iph->daddr;
        printk( KERN_ALERT "Dest: found %pI4 and value is %pI4  \n", &oldIP, &newIP);

        csum_replace4(&iph->check, oldIP, newIP);

        inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);

    }


    }

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
        	okfn(skb);
            return  NF_STOLEN;
        }
        else  if( iph->protocol ==IPPROTO_UDP)
        {
        	//do nothing
        }
         return NF_ACCEPT;
    }
    
}

#endif

MODULE_AUTHOR("Kelvin Zou: <xuanz@cs.princeton.edu>");
MODULE_DESCRIPTION("add ip/udp header on top of ip header");
MODULE_LICENSE("GPL");