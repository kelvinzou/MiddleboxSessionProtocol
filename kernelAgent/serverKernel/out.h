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
    __u16 dst_port, src_port;
    struct iphdr *iph;
    struct udphdr *udph;
    if (skb) {

        iph = (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);
        
        //do not change any non-UDP traffic
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_UDP&&iph->protocol!= IPPROTO_TCP) ) {
            return NF_ACCEPT;
        } 

        else if (iph->protocol ==IPPROTO_TCP)
        {
		    struct tcphdr *tcph ;

		    unsigned int data_len ;
		    data_len = skb->len ;

		    tcph = (struct tcphdr *) tcp_hdr ( skb ) ;
		    __u16 total_len = ntohs(iph->tot_len);
		    unsigned int iphdr_len ;
		    iphdr_len = ip_hdrlen(skb) ;
		    unsigned int tcphdr_len ;
		    tcphdr_len = tcp_hdrlen(skb) ;
		    unsigned int tcp_len ;
		    tcp_len = data_len - iphdr_len ;  
			__u32 seqNumber =  tcph->seq;
		    __u32 ackSeq = tcph->ack_seq;

		    record_t l, *p ;
		    memset(&l, 0, sizeof(record_t) ) ;
		    p=NULL;
		    //get_random_bytes ( &i, sizeof (int) );
		    l.key.dst =iph->daddr ; 
		    l.key.sport = ntohs(tcph->source) ;
		    read_lock(&my_rwlock) ;
		    HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p) ;
		    read_unlock(&my_rwlock) ;


		   	if(p){
		        //printk(  "Output: found source key %pI4 and value is %pI4  \n", &iph->saddr , &p->src ) ;
                //printk(  "Output: found dest key %pI4 and value is %pI4  \n", & iph->daddr , &p->dst ) ;
		        __be32 oldIP = iph->daddr;
		        iph->daddr = p->dst;
		        __be32 newIP = iph->daddr;
		        inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
			    csum_replace4(&iph->check, oldIP, newIP);
		        
		        //this is due to the rerouting for different interfaces
		        oldIP = iph->saddr;
		        iph->saddr = p->src;
		        newIP = iph->saddr;
		        inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
			    csum_replace4(&iph->check, oldIP, newIP);


		        if(p->Migrate ==1){
		        		printk("Buffering packets now and the length is %u and %d\n", total_len, data_len);

		        		ip_route_me_harder(skb, RTN_UNSPEC);
		        		return NF_QUEUE;
		        	} 
		        else{

			    	ip_route_me_harder(skb, RTN_UNSPEC);
            		return NF_ACCEPT;
		        }
		        
			   // printk( " Output: found src and dest is  %pI4 and %pI4 \n", &iph->saddr,  &iph->daddr);
		   	}
            return NF_ACCEPT;
 
        } 



        else if(iph->protocol ==IPPROTO_UDP){
			struct udphdr *udph;
		    udph =  udp_hdr (skb );
		    unsigned int iphdr_len ;
		    iphdr_len= sizeof (struct iphdr);
		    unsigned int  udphdr_len;
		    udphdr_len = sizeof (struct udphdr) ;

		    record_t l, *p ;
		    memset(&l, 0, sizeof(record_t) ) ;
		    p=NULL;
		    //get_random_bytes ( &i, sizeof (int) );
		    l.key.dst =iph->daddr ; 
		    l.key.sport = ntohs(udph->source) ;
		    read_lock(&my_rwlock) ;
		    HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p) ;
		    read_unlock(&my_rwlock) ;
		    if(p){
		        __be32 oldIP = iph->daddr;
		        iph->daddr = p->dst;
		        __be32 newIP = iph->daddr;
		        if (udph->check || skb->ip_summed == CHECKSUM_PARTIAL) {
		            inet_proto_csum_replace4(&udph->check, skb, oldIP, newIP, 1);
		        } else{
		        	udph->check = CSUM_MANGLED_0;
		        }
			    csum_replace4(&iph->check, oldIP, newIP);
		   	}
			return NF_ACCEPT;
	        
	     }
     return NF_ACCEPT;

    }
     return NF_ACCEPT;
}

#endif
