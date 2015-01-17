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
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_UDP&&iph->protocol!= IPPROTO_TCP) ) {
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
            __u32 seqNumber =  ntohl(tcph->seq) +data_len -iphdr_len -tcpoffset   ;
            __u32 ackSeq = ntohl(tcph->ack_seq);

            record_t l, *p ;

            memset(&l, 0, sizeof( record_t) );
            l.key.dst =iph->daddr ;
            l.key.dport = ntohs( tcph->dest );
            //spin_lock(&slock);
            HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p) ;
            //spin_unlock(&slock);

            if(p){
                if(p->Migrate ==1) {
                	/*
                	//introduce the reordering here
                	if(p->Dropped == 0){
                        	p->Dropped =1;
                        	return NF_DROP;
                    }*/
                    printk("\nThe seq number is %u and the ack number is %u\n", seqNumber,ackSeq );
                    if (p->Seq >= seqNumber){
                       printk("The packets are lower than seq so transmit\n");
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
                    else{
                        
                        printk("queue packets for higher seq number! and the seq and ack number is %u and %u\n",p->Seq, p->Ack);
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

                        return NF_QUEUE;
                    } 
                } else{
                //	printk("\nThe seq number is %u and the ack number is %u\n", seqNumber,ackSeq );


                    if(p->Seq ==0){
                    	p->Seq =  seqNumber;
                    	//this is to initialize the seq number
                    } 
                    else if(p->Seq < seqNumber || p->Seq < (seqNumber+0xffff0000)){
                        //the second half condition is to avoid seq number wrap up
                         p->Seq =  seqNumber;
                    }

                    __be32 oldIP = iph->daddr;
                    iph->daddr = p->dst;
                    __be32 newIP = iph->daddr;
                    
                    inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
                    csum_replace4(&iph->check, oldIP, newIP);
                    
                    //it has to change its source ip address if it goes through a different interface
                    
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

/*
*******************************************************************************************
*******************************************************************************************
        The following is for UDP handling, 
        used for check millions packets per seconds tests
******************************************************************************************
******************************************************************************************
*/
        else if( iph->protocol == IPPROTO_UDP){
                struct udphdr *udph;
                udph = udp_hdr (skb );

                unsigned int iphdr_len ;
                iphdr_len= sizeof (struct iphdr);

                //create new space at the head of socket buffer
                record_t l, *p ;
                memset(&l, 0, sizeof(record_t) ) ;
                p=NULL;
                //get_random_bytes ( &i, sizeof (int) );
                l.key.dst =iph->daddr ;
                l.key.dport = ntohs(udph->dest) ;
                HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p) ;
                if(p){
	                __be32 oldIP = iph->daddr;
                    iph->daddr = p->dst;
                    __be32 newIP = iph->daddr;
                    if (udph->check || skb->ip_summed == CHECKSUM_PARTIAL) {
                        inet_proto_csum_replace4(&udph->check, skb, oldIP, newIP, 1);
                    }
                csum_replace4(&iph->check, oldIP, newIP);
              //  ip_route_me_harder(skb, RTN_UNSPEC);
                return NF_ACCEPT;
                } 
        	return NF_ACCEPT;
        }
     return NF_ACCEPT;

    }
     return NF_ACCEPT;
}

#endif
