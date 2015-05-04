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


static unsigned int incoming_change_begin(unsigned int hooknum, 
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
        } 
/*
****************************************************************************************************************
                    TCP
****************************************************************************************************************
*/        
        else if( iph->protocol ==IPPROTO_TCP){
            struct tcphdr *tcph ;
            unsigned int data_len;
            data_len = skb->len;

            tcph =   tcp_hdr (skb );

            record_t l, *p;
           
            memset(&l, 0, sizeof(record_t));

            p=NULL;
            l.key.src =iph->saddr;
            l.key.sport = ntohs(tcph->source) ;
            HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);
            if(p)
            {
                __be32 oldIP = iph->daddr;
                iph->daddr = p->dst;
                __be32 newIP = iph->daddr;
                //printk( KERN_ALERT "Destination: found %pI4 and value is %pI4  \n", &oldIP, &newIP);
                inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
                csum_replace4(&iph->check, oldIP, newIP);

        	    return NF_ACCEPT;
            } 
            else{
                memset(&l, 0, sizeof(record_t));
                l.key.src =iph->saddr;
                l.key.dport = ntohs(tcph->dest) ;
                HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);
                if (p){
                    __be32 oldIP = iph->daddr;
                    iph->daddr = p->dst;
                    __be32 newIP = iph->daddr;
                    //printk( KERN_ALERT "Destination: found %pI4 and value is %pI4  \n", &oldIP, &newIP);

                    inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
                    csum_replace4(&iph->check, oldIP, newIP);

        	    	return NF_ACCEPT;
               	}
            }
            return NF_ACCEPT;            
    		
        }
/*
****************************************************************************************************************
                   UDP
****************************************************************************************************************
*/         
        else  if( iph->protocol ==IPPROTO_UDP)
        {
            struct tcphdr *udph ;

            unsigned int data_len;
            data_len = skb->len;

            udph = udp_hdr (skb );

            record_t l, *p;
           
            memset(&l, 0, sizeof(record_t));
            p=NULL;
            l.key.src =iph->saddr;
            l.key.sport = ntohs(udph->source) ;
            HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);
            if(p)
            {
                __be32 oldIP = iph->daddr;
                iph->daddr = p->dst;
                __be32 newIP = iph->daddr; 

                csum_replace4(&iph->check, oldIP, newIP);
                return NF_ACCEPT;
            }
            else{
                memset(&l, 0, sizeof(record_t));
                l.key.src =iph->saddr;
                l.key.dport = ntohs(udph->dest) ;
                HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);
                if (p){                
                    __be32 oldIP = iph->daddr;
                    iph->daddr = p->dst;
                    __be32 newIP = iph->daddr;
  
                    csum_replace4(&iph->check, oldIP, newIP);
                    return NF_ACCEPT;
                }
            }
            return NF_ACCEPT;
        }
         return NF_ACCEPT;
    }
    
}

#endif

MODULE_AUTHOR("Kelvin Zou: <xuanz@cs.princeton.edu>");
MODULE_DESCRIPTION("add ip/udp header on top of ip header");
MODULE_LICENSE("GPL");
