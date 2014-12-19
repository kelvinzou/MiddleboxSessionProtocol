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
    if ( ntohs(tcph->dest)  == 5001 )
        printk("Input: The sequence nunmber and its sequence ack number are %u  and %u ", ntohl(seqNumber), ntohl(ackSeq));
    //tcph->check = 0;
    //tcph->check = ~csum_tcpudp_magic( iph->saddr, iph->daddr,tcp_len, IPPROTO_TCP, 0);

    /*
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
	        //ip_route_me_harder(skb, RTN_UNSPEC);
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
        }*/

    return skb;
    
}
 

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
            skb=tcp_header_rewrite(skb);
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
            /*
            if ( ntohs(tcph->dest)  == 5001 )
                printk("Output: The sequence nunmber and its sequence ack number are %u  and %u ", ntohl(seqNumber), ntohl(ackSeq));
            */
            //tcph->check = 0;
            //tcph->check = ~csum_tcpudp_magic( iph->saddr, iph->daddr,tcp_len, IPPROTO_TCP, 0);

            record_t l, *p ;
            memset(&l, 0, sizeof(record_t) ) ;
            p=NULL;
            l.key.dst =iph->daddr ;
            l.key.dport = ntohs(tcph->dest) ;
            read_lock(&my_rwlock) ;
            HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p) ;
            read_unlock(&my_rwlock) ;

            if(p){
	            __be32 oldIP = iph->daddr;
                iph->daddr = p->dst;
                __be32 newIP = iph->daddr;
                inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
                csum_replace4(&iph->check, oldIP, newIP);
                
                
                if(p->Migrate ==1){
                    if(p->Buffer ==1 ){
                        printk("Queue packets now!\n");
                        return NF_QUEUE;
                    }
                    else {
                        printk("No buffer needed, notify the user queue!\n");
                        tcph->urg =1;
                        write_lock(&my_rwlock);
                        p->Migrate =0;
                        write_unlock(&my_rwlock);
                        return NF_QUEUE;
                    }

                }  
                else {
                    read_lock(&release_lock);
                    read_unlock(&release_lock);
                    printk( "Output: found src dest are  %pI4 and %pI4 \n", & iph->saddr, & iph->daddr);
                    return NF_ACCEPT;
                    }               
                
                }
                return NF_ACCEPT;
            
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
                read_lock(&my_rwlock) ;
                HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p) ;
                read_unlock(&my_rwlock) ;
                if(p){
	                __be32 oldIP = iph->daddr;
                    iph->daddr = p->dst;
                    __be32 newIP = iph->daddr;
                    printk("UDP packet found!\n");
                    if (udph->check || skb->ip_summed == CHECKSUM_PARTIAL) {
                    	printk("Old checksum is %u", ntohs(udph->check) );
                        inet_proto_csum_replace4(&udph->check, skb, oldIP, newIP, 1);
                        printk("New checksum is %u", ntohs(udph->check) );
                    }
                csum_replace4(&iph->check, oldIP, newIP);
                return NF_ACCEPT;
                } 
        	return NF_ACCEPT;
        }
     return NF_ACCEPT;

    }
     return NF_ACCEPT;
}

#endif
