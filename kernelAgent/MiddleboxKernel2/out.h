#ifndef _LOCAL_OUT_H_
#define _LOCAL_OUT_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/if_packet.h>
#include <linux/crypto.h>
#include <linux/init.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/inet.h>
#include <linux/tcp.h>
#include <net/udp.h>
#include <net/tcp.h>
#include <net/inet_sock.h>
#include <net/flow.h>
#include <net/route.h>
#include <linux/inetdevice.h>

#include "uthash.h"

static unsigned int outgoing_change_begin (unsigned int hooknum,
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *))
{ 
    __u16 dst_port, src_port;
    struct iphdr *iph;
    struct udphdr *udph;
    if (skb) {

        iph = (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);
        
        //do not change any non-UDP traffic
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_UDP&&iph->protocol!= IPPROTO_TCP) ) {
            return NF_ACCEPT;
        } 
/*
****************************************************************************************************************
                    TCP
****************************************************************************************************************
*/  
        else if (iph->protocol ==IPPROTO_TCP)
        {
            struct tcphdr *tcph ;
            
            unsigned int data_len;
            data_len = skb->len;
            tcph =  tcp_hdr (skb );

            unsigned int  iphdr_len;
            iphdr_len =  ip_hdrlen(skb) ;
            unsigned int   tcphdr_len;
            tcphdr_len = tcp_hdrlen(skb) ;
            unsigned int tcp_len;
            tcp_len = data_len - iphdr_len;  
            if( ntohs(tcph->source ) == 80  )
            {
             printk( "Outgoing: src is %pI4 and dst is %pI4  \n", & iph->saddr, & iph->daddr);
            }
            record_t l, *p;
            memset(&l, 0, sizeof(record_t));
            p=NULL;
            l.key.src =iph->saddr;
            l.key.dport = ntohs(tcph->dest) ;
            HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);
            if (p){

                __be32 oldIP = iph->saddr;
                iph->saddr = p->src;
                __be32 newIP = iph->saddr;
	            printk( KERN_ALERT "Src: found %pI4 and value is %pI4  \n", &oldIP, &newIP);
                inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
                csum_replace4(&iph->check, oldIP, newIP);
            }
            else{
                memset(&l, 0, sizeof(record_t));
                p=NULL;
                l.key.src =iph->saddr;
                l.key.sport = ntohs(tcph->source) ;
                HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);
                if(p)
                {
                    __be32 oldIP = iph->saddr;
                    iph->saddr = p->src;
                    __be32 newIP = iph->saddr;
                     printk( KERN_ALERT "Source: found %pI4 and value is %pI4  \n", &oldIP, &newIP);
                    inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
                    csum_replace4(&iph->check, oldIP, newIP);
                }
            }
            return NF_ACCEPT;
        } 
/*
****************************************************************************************************************
                   UDP
****************************************************************************************************************
*/  
        else if (iph->protocol ==IPPROTO_UDP){
            struct udphdr *udph;
            udph =  udp_hdr (skb );
            unsigned int iphdr_len ;
            iphdr_len= sizeof (struct iphdr);
            unsigned int  udphdr_len;
            udphdr_len = sizeof (struct udphdr) ;
            
            record_t l, *p;
            memset(&l, 0, sizeof(record_t));
            p=NULL;
            l.key.src =iph->saddr;
            l.key.dport = ntohs(udph->dest) ;
            HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);
            if (p){

                __be32 oldIP = p->src;
                __be32 newIP = p->dst;
                inet_proto_csum_replace4(&udph->check, skb, oldIP, newIP, 1);

                oldIP = iph->saddr;
                iph->saddr = p->src;
                newIP = iph->saddr;
                inet_proto_csum_replace4(&udph->check, skb, oldIP, newIP, 1);
                csum_replace4(&iph->check, oldIP, newIP);
                         
            } else{
                memset(&l, 0, sizeof(record_t));
                l.key.src =iph->saddr;
                l.key.sport = ntohs(udph->source) ;
                HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);
                if(p)
                {
                    __be32 oldIP = p->src;
                    __be32 newIP = p->dst;
                    inet_proto_csum_replace4(&udph->check, skb, oldIP, newIP, 1);
                    oldIP = iph->saddr;
                    iph->saddr = p->src;
                    newIP = iph->saddr;
                    inet_proto_csum_replace4(&udph->check, skb, oldIP, newIP, 1);
                    csum_replace4(&iph->check, oldIP, newIP);
                }

            }
            return NF_ACCEPT;
        }
     return NF_ACCEPT;
    }
     return NF_ACCEPT;
}

#endif
