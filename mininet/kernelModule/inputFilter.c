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
#include <net/ip.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <net/route.h>
#include <linux/inet.h>

#undef __KERNEL__
#include <linux/netfilter_ipv4.h>
#define __KERNEL__
//#define MODULE

#define IP_HDR_LEN 20
#define UDP_HDR_LEN 8
#define TCP_HDR_LEN 20
#define TOT_HDR_LEN 28

//value is 192.168.0.1
static unsigned char *ip_address = "\xC0\xA8\x00\x01"; 
//127.0.0.1
static unsigned char *ip_address2 = "\x8F\x00\x00\x01"; 


static unsigned int pkt_mangle_begin(unsigned int hooknum,
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *));

static int header_rewrite(struct sk_buff *skb, char * ip_dest);

static struct nf_hook_ops pkt_mangle_ops __read_mostly = {
    .pf = NFPROTO_IPV4,
    .priority = 1,
    .hooknum = NF_IP_PRE_ROUTING,
    .hook = pkt_mangle_begin,
};
//standard init and exit for a module 

static int __init pkt_mangle_init(void)
{
    printk(KERN_ALERT "\npkt_mangle input module started ...\n");
    return nf_register_hook(&pkt_mangle_ops);
}

static void __exit pkt_mangle_exit(void)
{
    nf_unregister_hook(&pkt_mangle_ops);
    printk(KERN_ALERT "\npkt_mangle input module stopped ...\n");
} 




//body for packet registration and modification
static int header_rewrite(struct sk_buff *skb, char * ip_dest){
    __u16 dst_port, src_port;
    struct iphdr *iph;
    struct tcphdr *tcph;
    struct udphdr *udph ;
    unsigned char * headerPtr, ptr;
    //whole IP packet now becomes a body
    unsigned int data_len = skb->len;
    
    iph = (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);
    tcph = (struct tcphdr *) skb_header_pointer (skb, IP_HDR_LEN, 0, NULL);
    src_port = ntohs (tcph->source);
    dst_port = ntohs (tcph->dest);
    
    //create new space at the head of socket buffer
    printk(KERN_ALERT "Input: Push header in front of the old header\n");
    headerPtr= skb_push(skb,TOT_HDR_LEN);

    //copy IP and create UDP header, including copying the ports number.
    memcpy(headerPtr, headerPtr+TOT_HDR_LEN, IP_HDR_LEN);
    iph = (struct iphdr *)skb_header_pointer (skb, 0, 0, NULL);
    
    ptr = headerPtr + IP_HDR_LEN;
    udph = (struct udphdr *) ptr;
    udph->source = htons(src_port) ;
    udph->dest  = htons(dst_port) ;


    //recompute the length
    iph->tot_len = htons( data_len + TOT_HDR_LEN);
    udph->len = htons( data_len + UDP_HDR_LEN);

    //update  IP header
    iph->daddr = * (unsigned int *) ip_address;

    //update checksum
    iph->check = 0;
    ip_send_check (iph);
    
    
    //update UDP header

    udph->check = 0;
    int offset = skb_transport_offset(skb);
    int len = skb->len - offset;
    udph->check = ~csum_tcpudp_magic((iph->saddr), (iph->daddr), len, IPPROTO_UDP, 0);
    return 0;

    //return NULL;
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
            printk(KERN_ALERT "Input: ever pass UDP\n");
            udph = (struct udphdr *) skb_header_pointer (skb, IP_HDR_LEN, 0, NULL);
            src_port = ntohs (udph->source);
            dst_port = ntohs (udph->dest);
            //do not change any non-special traffic
            if (dst_port !=1234 && src_port !=1234){
                printk(KERN_ALERT "The port numbers are %d and %d \n", dst_port, src_port );
                return NF_ACCEPT;
            }
            // this is the only change part and hopefully it works fine!
            else // (iph->daddr ==*(unsigned int *) ip_address)
             {
                printk(KERN_ALERT "Input: ever pass input final check?\n");
                //header_rewrite(skb, ip_address2);
                return NF_ACCEPT;
            } //else return NF_ACCEPT;
            
        }
    }
     return NF_ACCEPT;
}


module_init(pkt_mangle_init);
module_exit(pkt_mangle_exit);

MODULE_AUTHOR("Kelvin Zou: <xuanz@cs.princeton.edu>");
MODULE_DESCRIPTION("add ip/udp header on top of ip header");
MODULE_LICENSE("GPL");