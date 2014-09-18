#ifndef _POST_ROUTING_H_
#define _POST_ROUTING_H_


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
#include <linux/scatterlist.h>
#include <linux/inet.h>
#include <linux/tcp.h>
#include <net/udp.h>
#include <net/route.h>


static unsigned int pkt_check_begin(unsigned int hooknum,
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *)) 
{
   struct iphdr  *iph;
    struct udphdr *udph;
    __u16 dst_port, src_port;

    if (skb) {
        iph = (struct iphdr *) skb_network_header (skb );

        //do not change any non-UDP traffic
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_UDP) ) {
            return NF_ACCEPT;
        } else{
            //printk(KERN_ALERT "Input: ever pass UDP\n");
            udph = (struct udphdr *) udp_hdr (skb );
            src_port = ntohs (udph->source);
            dst_port = ntohs (udph->dest);
            
            //do not change any non-special traffic
            if (dst_port !=1234){
                return NF_ACCEPT;
            }
            // this is the only change part and hopefully it works fine!
            else // (iph->daddr ==*(unsigned int *) ip_address)
             {
                printk(KERN_ALERT "POST_ROUTING: ever pass input final check?\n");
                printk(KERN_ALERT "POST_ROUTING: pass check IP numbers are %pI4 and  %pI4\n", 
                &iph->saddr ,&iph->daddr );
                //header_rewrite(skb, ip_address2);
                return NF_ACCEPT;
            } //else return NF_ACCEPT;
            
        }
    }
     return NF_ACCEPT;
}



#endif

MODULE_AUTHOR("Kelvin Zou: <xuanz@cs.princeton.edu>");
MODULE_DESCRIPTION("add ip/udp header on top of ip header");
MODULE_LICENSE("GPL");