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

struct sk_buff * header_rewrite(struct sk_buff *skb){ 

    struct iphdr *iph;
    struct udphdr *udph;
    iph = (struct iphdr *) ip_hdr (skb ); 
    udph = (struct udphdr *) udp_hdr (skb );
    unsigned int iphdr_len ;
    iphdr_len= sizeof (struct iphdr);
    unsigned int  udphdr_len;
    udphdr_len = sizeof (struct udphdr) ;
    //create new space at the head of socket buffer
    printk(KERN_ALERT "Output: Push header in front of the old header\n");
    if (skb_headroom(skb) < (iphdr_len+udphdr_len)) {
        printk(KERN_ALERT "Output: After skb_push lalala Push header in front of the old header\n");
        struct sk_buff * skbOld;
        skbOld = skb;
        skb = skb_realloc_headroom(skbOld, iphdr_len+udphdr_len);
        if (!skb) {
                printk(KERN_ERR "vlan: failed to realloc headroom\n");
                return NULL;
        }
         if(skbOld->sk){
            skb_set_owner_w(skb, skbOld->sk);
        }
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


    //tcph->check = 0;
    //tcph->check = ~csum_tcpudp_magic( iph->saddr, iph->daddr,tcp_len, IPPROTO_TCP, 0);
    bool FLAG = false;
    if(FLAG){
    int i;
    record_t l, *p;
    memset(&l, 0, sizeof(record_t));
    p=NULL;
    //get_random_bytes ( &i, sizeof (int) );
    i=1;
    l.key.a =i;
    l.key.b =i+5;
    HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);
    //if (p) printk( KERN_ALERT "found %d %d and value is %d \n", p->key.a, p->key.b, p->a);
    }

    if(iph->daddr == in_aton("192.168.56.102")){
        printk(KERN_ALERT "Output: Initial Src and Dest address is %pI4 and  %pI4\n",   &iph->saddr ,&iph->daddr );
   
    //    printk("Output: Initial checksum is %u and %u and %u checksum header and offset are %d and %d and %d \n", skb->csum, tcph->check,iph->check ,skb->csum_start, skb->transport_header, skb->csum_offset); 

        if (unlikely(skb_linearize(skb) != 0))
            return NULL;

        __be32 oldIP = iph->daddr;
        iph->daddr = in_aton("192.168.56.1");
        __be32 newIP = iph->daddr;

        //smart way of running checksum, it is being the same way in linux kernel netfilter_nat
        inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
        csum_replace4(&iph->check, oldIP, newIP);

        printk(KERN_ALERT "Output: New Src and Dest address is %pI4 and  %pI4\n",   &iph->saddr ,&iph->daddr );

    }
    return  skb ;
}
 

static unsigned int pkt_mangle_begin (unsigned int hooknum,
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
        } //handle UDP packdets
        else if (iph->protocol ==IPPROTO_TCP)
        {
            skb=tcp_header_rewrite(skb);
            //return NF_ACCEPT;
            if (skb ==NULL) {
                printk(KERN_ALERT "Output: Fail to skb_linearize\n");
                return NF_DROP;
            }
            okfn(skb);
            return  NF_STOLEN;
        } 
        else if (iph->protocol == IPPROTO_UDP)  {
            udph = (struct udphdr *) skb_header_pointer (skb, sizeof(struct iphdr) , 0, NULL);
            src_port = ntohs (udph->source);
            dst_port = ntohs (udph->dest);
            //do not change any non-special traffic
            if (dst_port !=1234 && src_port !=1234){
                return NF_ACCEPT;
            }
            else 
             {
                if (dst_port==1234)
                {
                    printk(KERN_ALERT "Output: get to modification phase? \n");
                    
                    skb = header_rewrite(skb);
                    //ip_route_me_harder(skb,RTN_LOCAL);
                    printk(KERN_ALERT "Finish writing? \n");
                    okfn(skb);

                    return  NF_STOLEN;
                }     
                return NF_ACCEPT;
            }        
        }
    }
     return NF_ACCEPT;
}

#endif