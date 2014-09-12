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

#undef __KERNEL__
#include <linux/netfilter_ipv4.h>
#define __KERNEL__


#define IP_HDR_LEN 20
#define UDP_HDR_LEN 8
#define TCP_HDR_LEN 20
#define TOT_HDR_LEN 28


static unsigned int pkt_mangle_begin(unsigned int hooknum,
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *));

static struct nf_hook_ops pkt_mangle_ops __read_mostly = {
    .pf = NFPROTO_IPV4,
    .priority = 1,
    .hooknum = NF_IP_PRE_ROUTING,
    .hook = pkt_mangle_begin,
};


//standard init and exit for a module 

static int __init pkt_mangle_init(void)
{
    printk(KERN_ALERT "\npkt_mangle output module started ...\n");
    return nf_register_hook(&pkt_mangle_ops);
}

static void __exit pkt_mangle_exit(void)
{
    nf_unregister_hook(&pkt_mangle_ops);
    printk(KERN_ALERT "\n pkt_mangle output module stopped ...\n");
} 


static unsigned int pkt_mangle_begin (unsigned int hooknum,
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *))
{ 
     struct iphdr  *iph;
    struct udphdr *udph;
    __u16 dst_port, src_port;

    if (skb) {
        iph = (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);

        //do not change any non-UDP traffic
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_UDP) ) {
            return NF_ACCEPT;
        } else{
            printk(KERN_ALERT "Extraput: ever pass UDP\n");
            udph = (struct udphdr *) skb_header_pointer (skb, IP_HDR_LEN, 0, NULL);
            src_port = ntohs (udph->source);
            dst_port = ntohs (udph->dest);
            printk(KERN_ALERT "The port numbers are %d and %d \n", src_port, dst_port );
            printk(KERN_ALERT "The ip addresses are%pI4 and  %pI4\n", 
                &iph->saddr ,&iph->daddr );
            //do not change any non-special traffic
            if (dst_port !=1234){
                return NF_ACCEPT;
            }
            // this is the only change part and hopefully it works fine!
            else // (iph->daddr ==*(unsigned int *) ip_address)
             {
                printk(KERN_ALERT "Extraput: ever pass input final check?\n");
                return NF_ACCEPT;
            } 
        }
    }
     return NF_ACCEPT;
}

module_init(pkt_mangle_init);
module_exit(pkt_mangle_exit);

MODULE_AUTHOR("Kelvin Zou: <xuanz@cs.princeton.edu>");
MODULE_DESCRIPTION("add ip/udp header on top of ip header");
MODULE_LICENSE("GPL");