#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>


#define DRIVER_AUTHOR "Kelvin Zou"
#define DRIVER_DESC "Testing driver"

#define DEBUG 1

static struct nf_hook_ops nfho;

static unsigned int hook_func(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *)){
    struct iphdr *ip_header;
    
    struct ethhdr *eth_header;

    u32 saddr, daddr;
    u16 source, dest;

    /* Get all the headers */
    eth_header = (struct ethhdr *)skb_mac_header(skb);
    ip_header = (struct iphdr *)skb_network_header(skb);
    skb_set_transport_header(skb, ip_header->ihl * 4);
    tcp_header = (struct tcphdr *)skb_transport_header(skb);

    /* If the packet source or dest are not 80 then the packet is not for us :) */
    if(tcp_header->source != ntohs(80) && tcp_header->dest != ntohs(80))
        return NF_ACCEPT;

#if DEBUG > 0
printk(KERN_INFO "[HTTP] Got packet on %d from %d\n", htons(tcp_header->dest), htons(tcp_header->source));
#endif

    saddr = ip_header->saddr;
    daddr = ip_header->daddr;

    source = tcp_header->source;
    dest = tcp_header->dest;

    /* In link layer header change sender mac to our ethernet mac
        and destination mac to sender mac :) ping-pong */
    memcpy(eth_header->h_dest,eth_header->h_source,ETH_ALEN);
    memcpy(eth_header->h_source,skb->dev->dev_addr,ETH_ALEN);

    /* Set new link layer headers to socket buffer */
    skb->data = (unsigned char *)eth_header;
    skb->len += ETH_HLEN;

    /* Setting it as outgoing packet */
    skb->pkt_type = PACKET_OUTGOING;

    /* Swap the IP headers sender and destination addresses */
    memcpy(&ip_header->saddr, &daddr, sizeof(u32));
    memcpy(&ip_header->daddr, &saddr, sizeof(u32));


    /* If transmission suceeds then report it stolen
        if it fails then drop it */
    if(dev_queue_xmit(skb)==NET_XMIT_SUCCESS){
#if DEBUG > 0
printk(KERN_INFO "[HTTP] Successfully sent packet\n");
#endif
        return NF_STOLEN;
    } else {
#if DEBUG > 0
printk(KERN_INFO "[HTTP] Sending failed\n");
#endif
        return NF_DROP;
    }

}


static int __init init_main(void) {
    nfho.hook = hook_func;
    nfho.hooknum = 0;
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;
    nf_register_hook(&nfho);

#if DEBUG > 0
    printk(KERN_INFO "[HTTP] Successfully inserted protocol module into kernel.\n");
#endif
    return 0;
}

static void __exit cleanup_main(void) {
    nf_unregister_hook(&nfho);
#if DEBUG > 0
    printk(KERN_INFO "[HTTP] Successfully unloaded protocol module.\n");
#endif
}

module_init(init_main);
module_exit(cleanup_main);

MODULE_LICENSE("GPL v3");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);