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

struct sk_buff * udp_header_rewrite(struct sk_buff *skb){ 

    struct iphdr *iph;
    struct udphdr *udph;
    iph =  ip_hdr (skb ); 
    udph =  udp_hdr (skb );
    unsigned int iphdr_len ;
    iphdr_len= sizeof (struct iphdr);
    unsigned int  udphdr_len;
    udphdr_len = sizeof (struct udphdr) ;
    //create new space at the head of socket buffer
    
    record_t l, *p;
    memset(&l, 0, sizeof(record_t));
    p=NULL;
    l.key.src =iph->saddr;
    l.key.dport = ntohs(udph->dest) ;
    HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);
    if (p){

        __be32 oldIP = iph->saddr;
        iph->saddr = p->src;
        __be32 newIP = iph->saddr;
        printk( KERN_ALERT "Source: udp found %pI4 and value is %pI4  \n", &oldIP, &newIP);
        if (udph->check || skb->ip_summed == CHECKSUM_PARTIAL) {
            printk("Old checksum is %u\n",ntohs(udph->check) );
            inet_proto_csum_replace4(&udph->check, skb, oldIP, newIP, 1);
            printk("New checksum is %u\n",ntohs(udph->check) );
        }
        csum_replace4(&iph->check, oldIP, newIP);
        return  skb ;
    }
    
    memset(&l, 0, sizeof(record_t));
    p=NULL;
    l.key.src =iph->saddr;
    l.key.sport = ntohs(udph->source) ;
    HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);
    if(p)
    {
        printk( KERN_ALERT "src: udp found %pI4 and value is %pI4  \n", &p->key.src , &p->src);
        __be32 oldIP = iph->saddr;
        iph->saddr = p->src;
        __be32 newIP = iph->saddr;
         if (udph->check || skb->ip_summed == CHECKSUM_PARTIAL) {
            printk("Old checksum is %u\n",ntohs(udph->check) );
            inet_proto_csum_replace4(&udph->check, skb, oldIP, newIP, 1);
            printk("New checksum is %u\n",ntohs(udph->check) );
        }
        csum_replace4(&iph->check, oldIP, newIP);
        return skb;
    }

    return  skb ;
}

struct sk_buff * tcp_header_rewrite(struct sk_buff *skb){ 

    struct iphdr *iph ;
    struct tcphdr *tcph ;

    unsigned int data_len;
    data_len = skb->len;

    iph = (struct iphdr *) ip_hdr (skb ); 
    tcph = (struct tcphdr *) tcp_hdr (skb );

    unsigned int  iphdr_len;
    iphdr_len =  ip_hdrlen(skb) ;
    unsigned int   tcphdr_len;
    tcphdr_len = tcp_hdrlen(skb) ;
    unsigned int tcp_len;
    tcp_len = data_len - iphdr_len;  

    //get_random_bytes ( &i, sizeof (int) );
    
    bool FLAG = true;
    
    if(FLAG){
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
	    printk( KERN_ALERT "Source: found %pI4 and value is %pI4  \n", &oldIP, &newIP);
        inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
        csum_replace4(&iph->check, oldIP, newIP);

        return  skb ;
    }
    
    memset(&l, 0, sizeof(record_t));
    p=NULL;
    l.key.src =iph->saddr;
    l.key.sport = ntohs(tcph->source) ;
    HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);
    if(p)
    {
        printk( KERN_ALERT "src: found %pI4 and value is %pI4  \n", &p->key.src , &p->src);
        __be32 oldIP = iph->saddr;
        iph->saddr = p->src;
        __be32 newIP = iph->saddr;
        inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
        csum_replace4(&iph->check, oldIP, newIP);
	
        return skb;

    }
    else {
        //printk( KERN_ALERT "No hash found, do nothing \n");
        return skb;
        }
    }
    return skb;
    
}
 

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
                    inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
                    csum_replace4(&iph->check, oldIP, newIP);
                }
            }
            return  NF_ACCEPT;
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
                
		          /* 
	            unsigned int data_len = skb->len;
                udph->check=0;
                skb->csum = csum_partial( (char *)udph , data_len-4*iph->ihl ,0);
                udph->check = csum_tcpudp_magic((iph->saddr), (iph->daddr), data_len -iph->ihl*4,IPPROTO_UDP,skb->csum);
                   */         
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
