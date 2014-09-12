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

static unsigned int pkt_mangle_begin(unsigned int hooknum,
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *));

struct sk_buff *  header_rewrite(struct sk_buff *skb);


static unsigned int pkt_unmangle_begin(unsigned int hooknum,
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *));

static struct nf_hook_ops in_put_traffic;
static struct nf_hook_ops out_put_traffic;

//standard init and exit for a module 

static int __init pkt_mangle_init(void)
{   
    printk(KERN_ALERT "\npkt_mangle output module started ...\n");
    
    //int goes to prerouting and unmangle the traffic header
    in_put_traffic.pf = NFPROTO_IPV4,
    in_put_traffic.priority = 1,
    in_put_traffic.hooknum = NF_IP_POST_ROUTING,
    in_put_traffic.hook = pkt_unmangle_begin,
    nf_register_hook(& in_put_traffic);


    //out put does to localout and mangle the hdr

    out_put_traffic.pf = NFPROTO_IPV4,
    out_put_traffic.priority = 1,
    out_put_traffic.hooknum = NF_IP_LOCAL_OUT,
    out_put_traffic.hook = pkt_mangle_begin,
    nf_register_hook(& out_put_traffic);

    return 0;
}

static void __exit pkt_mangle_exit(void)
{
    nf_unregister_hook(&in_put_traffic);
    nf_unregister_hook(&out_put_traffic);

    printk(KERN_ALERT "\npkt_mangle output module stopped ...\n");
} 


static unsigned int pkt_unmangle_begin(unsigned int hooknum,
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
            //printk(KERN_ALERT "Input: ever pass UDP\n");
            udph = (struct udphdr *) skb_header_pointer (skb, sizeof(struct iphdr), 0, NULL);
            src_port = ntohs (udph->source);
            dst_port = ntohs (udph->dest);
            
            //do not change any non-special traffic
            if (dst_port !=1234){
                return NF_ACCEPT;
            }
            // this is the only change part and hopefully it works fine!
            else // (iph->daddr ==*(unsigned int *) ip_address)
             {

                 
                printk(KERN_ALERT "Input: ever pass input final check?\n");
                printk(KERN_ALERT "PASS FINAL CHECK The IP numbers are %pI4 and  %pI4\n", 
                &iph->saddr ,&iph->daddr );
                //header_rewrite(skb, ip_address2);
                return NF_ACCEPT;
            } //else return NF_ACCEPT;
            
        }
    }
     return NF_ACCEPT;
}

//body for packet registration and modification
struct sk_buff * header_rewrite(struct sk_buff *skb){
    struct iphdr *iph, *iphOld;
    struct udphdr *udph, *udphOld;
    //whole IP packet now becomes a body
    unsigned int data_len = skb->len;
    printk(KERN_ALERT "Old skb checksum is %lu\n", skb->csum);

    iph = (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);
    printk(KERN_ALERT "Output: Initial Source and Dest address is  %pI4 and  %pI4 \n", 
        &iph->saddr, &iph->daddr );
    udph = (struct udphdr *) skb_header_pointer (skb, sizeof(struct iphdr), 0, NULL);
    printk(KERN_ALERT "Output: Initial udp port number is %d and %d \n", 
        ntohs(udph->source), ntohs(udph ->dest));
    size_t iphdr_len = sizeof (struct iphdr);
    size_t udphdr_len = sizeof (struct udphdr) ;
    //create new space at the head of socket buffer
    printk(KERN_ALERT "Output: Push header in front of the old header\n");

    printk(KERN_ALERT "Output: Push header in front of the old header\n");

    printk(KERN_ALERT "headroom is %d large\n", skb_headroom(skb));
    if (skb_headroom(skb) < (iphdr_len+udphdr_len)) {
        printk(KERN_ALERT "Output: After skb_push lalala Push header in front of the old header\n");
        skb = skb_realloc_headroom(skb, iphdr_len+udphdr_len);
        if (!skb) {
                printk(KERN_ERR "vlan: failed to realloc headroom\n");
                return NULL;
        }
    } 
    skb_push(skb, iphdr_len+udphdr_len);


    udphOld = (struct udphdr *) skb_header_pointer (skb, sizeof(struct iphdr)+ iphdr_len+udphdr_len, 0, NULL);
    iphOld = (struct iphdr *) skb_header_pointer (skb, iphdr_len+udphdr_len, 0, NULL);

    udph =  (struct udphdr *) skb_header_pointer (skb, sizeof(struct iphdr), 0, NULL);
    
    udph->source = udphOld->source ;
    udph->dest  = udphOld->dest;
    udph->len = htons( data_len + sizeof(struct udphdr));
    //update UDP header csum

    skb->csum = csum_partial((char *)udph,
                 sizeof(struct udphdr), skb->csum);

    //This is for IP header
    //skb_push(skb, sizeof(struct iphdr));
    
    iph =  (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);
    //bunch of values are copied to new ip header
    iph->version = iphOld->version;
    iph->ihl = iphOld->ihl;
    iph->tos = iphOld->tos;
    iph->frag_off = iphOld->frag_off;
    iph->id =iphOld->id;
    iph->ttl = iphOld->ttl;
    //change the protocol
    iph->protocol = IPPROTO_UDP; 
    //change the length
    iph->tot_len = htons(skb->len);

    //IP addresses
    iph->saddr = iphOld->saddr;

    iph->daddr = in_aton("192.168.56.1");

    ip_send_check(iph);



    //update UPD checksum after update IP header
    udph->check = 0;
    udph->check = ~csum_tcpudp_magic((iph->saddr), (iph->daddr), data_len+sizeof(struct udphdr), IPPROTO_UDP,0);
    if (udph->check == 0)
        udph->check = -1;
  //*/
    printk(KERN_ALERT "Output: Packet length is %d\n\n", data_len);

    printk(KERN_ALERT "Output: Src and Dest address is %pI4 and  %pI4\n", 
        &iph->saddr ,&iph->daddr );
     
    printk(KERN_ALERT "Output: Final Packet length is %d\n", skb->len);

    printk(KERN_ALERT "Output: Final udp port number is %d and %d \n", 
        ntohs(udph->source), ntohs(udph ->dest));

    return  skb ;
}


static unsigned int pkt_mangle_begin (unsigned int hooknum,
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
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_UDP) ) {
            return NF_ACCEPT;
        } //handle UDP packdets
        else{
            udph = (struct udphdr *) skb_header_pointer (skb, sizeof(struct iphdr) , 0, NULL);
            src_port = ntohs (udph->source);
            dst_port = ntohs (udph->dest);
            //do not change any non-special traffic
            if (dst_port !=1234 && src_port !=1234){
                return NF_ACCEPT;
            }
            else 
             {
                if (dst_port==1234)
                {
                    printk(KERN_ALERT "Output: get to modification phase? \n");
                    struct sk_buff * retv = header_rewrite(skb);
                    return okfn(retv);
                }     
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