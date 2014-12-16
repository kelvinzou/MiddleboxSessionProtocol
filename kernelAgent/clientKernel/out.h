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

//#include "uthash.h"

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
                printk(KERN_ERR "failed to realloc headroom\n");
                return NULL;
        }
         if(skbOld->sk){
            skb_set_owner_w(skb, skbOld->sk);
        }
    }   
    return  skb ;
}

struct sk_buff * tcp_header_rewrite(struct sk_buff *skb){ 

    struct iphdr * iph ;
    struct tcphdr * tcph ;

    unsigned int data_len ;
    data_len = skb->len ;

    iph = (struct iphdr *) ip_hdr ( skb ); 
    tcph = (struct tcphdr *) tcp_hdr ( skb ) ;

    unsigned int iphdr_len ;
    iphdr_len = ip_hdrlen(skb) ;
    unsigned int tcphdr_len ;
    tcphdr_len = tcp_hdrlen(skb) ;
    unsigned int tcp_len ;
    tcp_len = data_len - iphdr_len ;  
    __u32 seqNumber =  tcph->seq;
    __u32 ackSeq = tcph->ack_seq;

    //tcph->check = 0;
    //tcph->check = ~csum_tcpudp_magic( iph->saddr, iph->daddr,tcp_len, IPPROTO_TCP, 0);
    bool FLAG = true ;
    
    if(FLAG){
    record_t l, *p ;
    memset(&l, 0, sizeof(record_t) ) ;
    p=NULL;
    //get_random_bytes ( &i, sizeof (int) );
    l.key.dst =iph->daddr ;
    l.key.dport = ntohs(tcph->dest) ;
    read_lock(&my_rwlock) ;
    HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p) ;
    read_unlock(&my_rwlock) ;
    if (p){

        //the following is the header rewriting
		if ( unlikely(skb_linearize(skb) != 0) )
			return NULL;
        
        printk( KERN_ALERT "Output: found destination key  %pI4 and value is %pI4  \n", &p->key.dst , &p->dst);

        __be32 oldIP = iph->daddr;
        iph->daddr = p->dst;
        __be32 newIP = iph->daddr;

        printk("Output: found the packet buffer and migrate bool flags are %u and %u\n", p->Buffer, p->Migrate);

        if(p->Migrate ==1){
        	if(p->Buffer ==1){
        		printk("So we are buffering packets now!\n");
        		iph->protocol = IPPROTO_RAW; 
	        	ip_send_check(iph) ;
	        	ip_route_me_harder(skb, RTN_UNSPEC);
	        	return skb;
        	} else {
        		printk("No buffer is needed, release and reset migrate flag\n");
        		//just mark one special packet, and this is the end of the buffering
        		// need to change both migrate and buffer flags to false.
        		__u16  * mark_end =  (__u16 *) (((char *) tcph) + 12);
        		//this basically set the urgent flag. 
        		
                tcph->urg =1;
                
        		iph->protocol = IPPROTO_RAW; 
	        	ip_send_check(iph) ;
	        	ip_route_me_harder(skb, RTN_UNSPEC);
	        	
	        	HashResetMigration(&l);

	        	write_lock(&release_lock);
	        	printk("Entering release lock and should not see any readlock msg unless after release\n");
	        	return skb;
        	}
        }
        else 
        { 
        	
        	inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
	        csum_replace4(&iph->check, oldIP, newIP);
	        ip_route_me_harder(skb, RTN_UNSPEC);
	        printk("before entering the readlock\n");
	        read_lock(&release_lock);
	        printk("readlock\n");

        	read_unlock(&release_lock);
            printk( "Output: found src dest are  %pI4 and %pI4  \n", & iph->saddr, & iph->daddr);
	        return  skb ;
        }
    }
    else {
    	if ( ntohs(tcph->dest)  == 5001 ) 
    	{
    		printk( "Output: found src dest are  %pI4 and %pI4  \n", & iph->saddr, & iph->daddr);
 	       	printk( "Output: No hash found, do nothing \n");
 	       	//redo the checksum all the time?
 	       	//tcph->check = ~tcp_v4_check(tcp_len, iph->saddr, iph->daddr,0);
    	}
        return skb;
        }
    }
    return skb;
    
}
 

static unsigned int outgoing_begin (unsigned int hooknum,
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *))
{ 
    __u16 dst_port, src_port;
    struct iphdr *iph;
    struct udphdr *udph;
    if (skb) {

        iph = (struct iphdr *) ip_hdr ( skb ); 
        
        //do not change any non-UDP traffic
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_UDP&&iph->protocol!= IPPROTO_TCP) ) {
            return NF_ACCEPT;
        } 
        //handle TCP packdets
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
     return NF_ACCEPT;

    }
     return NF_ACCEPT;
}

#endif
