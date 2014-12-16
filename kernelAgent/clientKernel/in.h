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


struct sk_buff * header_rewrite_back(struct sk_buff *skb){
    struct iphdr *iph ;
    struct udphdr *udph;
    
    //we only decapsulate the packet, right now just remodify the source


    unsigned int data_len = skb->len;

    iph = (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);

    udph = (struct udphdr *) skb_header_pointer (skb, sizeof(struct iphdr), 0, NULL);
    printk(KERN_ALERT "PRE_ROUTING: Initial udp port number is %d and %d \n", 
        ntohs(udph->source), ntohs(udph ->dest));

    size_t iphdr_len = sizeof (struct iphdr);
    size_t udphdr_len = sizeof (struct udphdr) ;
    //create new space at the head of socket buffer
    printk(KERN_ALERT "PRE_ROUTING: SWAP back to old header\n");
    skb_pull(skb, iphdr_len+udphdr_len);

    iph = (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);
    udph = (struct udphdr *) skb_header_pointer (skb, sizeof(struct iphdr), 0, NULL);
     udph->check = 0;
    skb->csum = csum_partial((char *)udph, data_len - sizeof(struct iphdr),0);
     udph->check = csum_tcpudp_magic((iph->saddr), (iph->daddr), data_len- sizeof(struct iphdr), IPPROTO_UDP,skb->csum);

    printk(KERN_ALERT "PRE_ROUTING: Packet length is %d\n\n", data_len);

    printk(KERN_ALERT "PRE_ROUTING: Src and Dest address is %pI4 and  %pI4\n", 
        &iph->saddr ,&iph->daddr );
    printk(KERN_ALERT "PRE_ROUTING: Final Packet length is %d\n", skb->len);

    return  skb ;
}

struct sk_buff * tcp_header_write_prerouting(struct sk_buff *skb){
    struct iphdr *iph ;
    struct tcphdr *tcph ;

    unsigned int data_len;
    data_len = skb->len;

    iph = (struct iphdr *) ip_hdr (skb ); 
    tcph = (struct tcphdr *) tcp_hdr (skb );
    
    __u32 seqNumber =  tcph->seq;
    __u32 ackSeq = tcph->ack_seq;
    if ( ntohs(tcph->source)  == 5001 )
        printk("Input: The sequence nunmber and its sequence ack number are %u  and %u ", ntohl(seqNumber), ntohl(ackSeq));

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
        return skb ;
    } else{
    	if ( ntohs(tcph->source)  == 5001 ){
	        printk( KERN_ALERT "Input: No hash found, do nothing %pI4 \n",&iph->saddr ) ;
    	}
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
    struct udphdr *udph;
    __u16 dst_port, src_port;
    struct sk_buff * retv;

    if (skb) {
        iph = (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);

        //do not change any non-TCP traffic
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_UDP && iph->protocol !=IPPROTO_TCP) ) {
            return NF_ACCEPT;
        } else if( iph->protocol ==IPPROTO_TCP){
        	
        	tcp_header_write_prerouting(skb);

        }
        else  if( iph->protocol ==IPPROTO_UDP)
        {
            //do nothing
        }
	return NF_ACCEPT;
    }
     return NF_ACCEPT;
}

#endif

MODULE_AUTHOR("Kelvin Zou: <xuanz@cs.princeton.edu>");
MODULE_DESCRIPTION("add ip/udp header on top of ip header");
MODULE_LICENSE("GPL");
