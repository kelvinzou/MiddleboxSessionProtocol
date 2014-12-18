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
    struct udphdr *udph;
    __u16 dst_port, src_port;

    if (skb) {
        iph = (struct iphdr *) ip_hdr ( skb ); 
        //do not change any non-TCP traffic
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_UDP && iph->protocol !=IPPROTO_TCP) ) {
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
        else if( iph->protocol ==IPPROTO_TCP){
            struct tcphdr *tcph ;

            unsigned int data_len;
            data_len = skb->len;

            tcph =   tcp_hdr (skb );
            
            __u32 seqNumber =  tcph->seq;
            __u32 ackSeq = tcph->ack_seq;
            if ( ntohs(tcph->source)  == 5001 )
                printk("Input: The sequence nunmber and its sequence ack number are %u  and %u\n ", ntohl(seqNumber), ntohl(ackSeq));

            bool FLAG = true;
            
            if(FLAG){
            record_t l, *p;
            memset(&l, 0, sizeof(record_t));
            p=NULL ;
            
            //get_random_bytes ( &i, sizeof (int) );
            
            l.key.src =iph->saddr ;
            l.key.sport = ntohs(tcph->source) ;
            read_lock(&my_rwlock) ;
            HASH_FIND(hh, records, &l.key, sizeof( record_key_t ), p) ;
            read_unlock(&my_rwlock) ;
            if(p)
            {
                printk( KERN_ALERT "Input: found source key %pI4 and value is %pI4  \n", &p->key.src , &p->src ) ;
                iph->saddr = p->src ;
            } else{
            	if ( ntohs(tcph->source)  == 5001 ){
	                printk( KERN_ALERT "Input: No hash found, do nothing %pI4 \n",&iph->saddr ) ;
            	   }
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
        else  if( iph->protocol ==IPPROTO_UDP)
        {
            struct udphdr *udph;
    
            unsigned int data_len = skb->len;


            udph = udp_hdr (skb );

            record_t l, *p;
            memset(&l, 0, sizeof(record_t));
            
            p=NULL ;

            l.key.src = iph->saddr ;
            l.key.sport = ntohs(udph->source) ;
            read_lock(&my_rwlock) ;
            HASH_FIND(hh, records, &l.key, sizeof( record_key_t ), p) ;
            read_unlock(&my_rwlock) ;
            if(p)
            {
                printk( KERN_ALERT "Input: found %pI4 and value is %pI4  \n", &p->key.src , &p->src ) ;
                iph->saddr = p->src ;
            } else{
                if ( ntohs(udph->source)  == 5001 )
                    printk( KERN_ALERT "No hash found, do nothing \n") ;
            }
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
