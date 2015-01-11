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
            __u32 seqNumber =  tcph->seq;
            __u32 ackSeq = tcph->ack_seq;
            int tcpoffset = tcph->doff;
            printk(" ip and tcp's length is %d\n",iphdr_len, tcpoffset );
            /*
            //this is for squid proxy
            if(ntohs(tcph->dest)==80 &&   ntohl (iph->daddr)  > ntohl( in_aton("157.166.0.0") ) && ntohl(iph->daddr)<ntohl( in_aton("157.167.0.0") ) ){
                savedIP =  iph->daddr;
                __be32 oldIP = iph->daddr;
                printk("we are going to rewrite the header?\n");
                iph->daddr = in_aton("10.0.3.2");
                __be32 newIP = iph->daddr;
                inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
                csum_replace4(&iph->check, oldIP, newIP);
                ip_route_me_harder(skb, RTN_UNSPEC);
                printk( KERN_ALERT "Destination: found %pI4 and value is %pI4  \n", &oldIP, &newIP);
                okfn(skb);
                
                return  NF_STOLEN;
            }
            */
            
            /*
            if ( ntohs(tcph->dest)  == 5001 )
                printk("Output: The sequence nunmber and its sequence ack number are %u  and %u ", ntohl(seqNumber), ntohl(ackSeq));
            */
            //tcph->check = 0;
            //tcph->check = ~csum_tcpudp_magic( iph->saddr, iph->daddr,tcp_len, IPPROTO_TCP, 0);
           	

            //	spin_lock(&slock);
            //TODO need to restore to the one without new mapping
            record_t l, *pvalue ;
            memset(&l, 0, sizeof(record_t) ) ;
            l.key.dst = iph->daddr ;
            l.key.dport = ntohs(tcph->dest) ;
            l.key.sport = ntohs(tcph->source);
            HASH_FIND(hh, records, &l.key, sizeof(record_key_t), pvalue) ;
            if(!pvalue){
                //this is the configured middlebox policy
                memset(&l, 0, sizeof(record_t) ) ;
                p=NULL;
                l.key.dst =iph->daddr ;
                l.key.dport = ntohs(tcph->dest) ;

                HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p) ;

                if(p){

                //    printk(  "Output: found source key %pI4 and value is %pI4  \n", &iph->saddr , &p->src ) ;
                //    printk(  "Output: found dest key %pI4 and value is %pI4  \n", & iph->daddr , &p->dst ) ;
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

                    if(p->Migrate ==1){
                        if(p->Buffer ==1 ){
                            printk("Queue packets now!\n");
                        //     spin_unlock(&slock);
                            ip_route_me_harder(skb, RTN_UNSPEC);
                            return NF_QUEUE;
                        }
                        else {
                            printk("No buffer needed, notify the user queue!\n");
                            tcph->urg =1;
                            p->Migrate =0;
                          //spin_unlock(&slock);
                            ip_route_me_harder(skb, RTN_UNSPEC);
                            return NF_QUEUE;
                        }

                    }  
                    else {
                    //    spin_unlock(&slock);
                        record_t  *r;
                        printk("Update the routing at the same time, map from configuration to flow mapping");
                        //add hash entry in the hash table    
                        r = (record_t*)kmalloc( sizeof(record_t) , GFP_KERNEL);
                        memset(r, 0, sizeof(record_t));
                        // this is middlebox copy
                        r->key.dst = iph->daddr ;
                        r->key.dport =ntohs(tcph->dest) ;
                        l->key.sport = ntohs(tcph->source);
                        //this is for testing raw socket
                        //r->dst =  in_aton("128.112.93.107");
                        //this is for testing MBP
                        //this is the old configure for the intial path
                        
                        r->dst = p->dst;
                        r->src = p->src;
                        HASH_ADD(hh, records, key, sizeof(record_key_t), r);

                        ip_route_me_harder(skb, RTN_UNSPEC);
                        return NF_ACCEPT;
                    }               
     
                }
            } else{
                __be32 oldIP = iph->daddr;
                iph->daddr = pvalue->dst;
                __be32 newIP = iph->daddr;
                inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
                csum_replace4(&iph->check, oldIP, newIP);
                
                //it has to change its source ip address if it goes through a different interface

                oldIP = iph->saddr;
                iph->saddr = pvalue->src;
                newIP = iph->saddr;

                inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
                csum_replace4(&iph->check, oldIP, newIP);

                if(p->Migrate ==1){
                    if(pvalue->Buffer ==1 ){
                        printk("Queue packets now!\n");
                    //     spin_unlock(&slock);
                        ip_route_me_harder(skb, RTN_UNSPEC);
                        return NF_QUEUE;
                    }
                    else {
                        printk("No buffer needed, notify the user queue!\n");
                        tcph->urg =1;
                        p->Migrate =0;
                      //spin_unlock(&slock);
                        ip_route_me_harder(skb, RTN_UNSPEC);
                        return NF_QUEUE;
                    }

                }  
                else {
                //    spin_unlock(&slock);
                    ip_route_me_harder(skb, RTN_UNSPEC);
                    return NF_ACCEPT;
                } 
            }

            
         //   spin_unlock(&slock); 
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
