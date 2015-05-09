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


static unsigned int outgoing_begin (unsigned int hooknum,
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *))
{ 
    struct iphdr *iph;
    if (skb) {

        iph = ip_hdr ( skb ); 
        
        //do not change any non-UDP traffic
        if ( iph && iph->protocol && iph->protocol!= IPPROTO_TCP ) {
            return NF_ACCEPT;
        } 
/*
*******************************************************************************************
*******************************************************************************************
        The following is for TCP handling, 
        used for check throughput tests
******************************************************************************************
******************************************************************************************
*/
        else if (iph->protocol ==IPPROTO_TCP)
        {
            struct tcphdr * tcph ;

            unsigned int data_len ;
            data_len = skb->len ;
            
            tcph = (struct tcphdr *) tcp_hdr ( skb ) ;
            
            unsigned int iphdr_len ;
            iphdr_len = ip_hdrlen(skb) ;
            unsigned int tcphdr_len ;
            tcphdr_len = tcp_hdrlen(skb) ;
            unsigned int tcp_len ;
            tcp_len = data_len - iphdr_len ;  
           
            int tcpoffset = 4* tcph->doff;

            record_t l, *p ;

            memset(&l, 0, sizeof( record_t) );
            l.key.dst =iph->daddr ;
            l.key.dport = ntohs( tcph->dest );
            //l.key.sport = ntohs( tcph->source );
            HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p) ;

            if(p){
            //during the migration, we do a buffering of the packets
                if(p->Migrate ==1 ) {

		            printk( KERN_ALERT "This means we have not received SYNACK yet\n");
            	
                    __be32 oldIP = iph->daddr;
                    iph->daddr = p->new_dst;
                    __be32 newIP = iph->daddr;
                    inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
                    csum_replace4(&iph->check, oldIP, newIP);
                    
                    oldIP = iph->saddr;
                    iph->saddr = p->new_src;
                    newIP = iph->saddr;
                    inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
                    csum_replace4(&iph->check, oldIP, newIP);

                    ip_route_me_harder(skb, RTN_UNSPEC);
                    printk( KERN_ALERT "Queue Packets now!\n");
                    return NF_QUEUE;
                } 


                //otherwise we just send the traffic directly

                else{
                    //printk( KERN_ALERT "This means we found a match\n");
		            __be32 oldIP = iph->daddr;
                    iph->daddr = p->dst;
                    __be32 newIP = iph->daddr;
                    inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
                    csum_replace4(&iph->check, oldIP, newIP);
                    
                    oldIP = iph->saddr;
                    iph->saddr = p->src;
                    newIP = iph->saddr;
                    inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
                    csum_replace4(&iph->check, oldIP, newIP);

                    ip_route_me_harder(skb, RTN_UNSPEC);
                    return NF_ACCEPT;

                    }
            }
            return NF_ACCEPT;
        }   

     return NF_ACCEPT;

    }
     return NF_ACCEPT;
}

#endif
