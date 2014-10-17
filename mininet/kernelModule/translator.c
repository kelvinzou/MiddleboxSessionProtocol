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

#include "pre_routing.h"
#include "post_routing.h"
#include "local_out.h"

static struct nf_hook_ops post_routing ;
static struct nf_hook_ops local_out;  
static struct nf_hook_ops pre_routing;

//standard init and exit for a module 

static int __init pkt_mangle_init(void)
{   
    printk(KERN_ALERT "\npkt_mangle output module started ...\n");
    
    //int goes to prerouting and unmangle the traffic header
    post_routing.pf = NFPROTO_IPV4;
    post_routing.priority = NF_IP_PRI_MANGLE;
    post_routing.hooknum = NF_IP_POST_ROUTING;
    post_routing.hook = pkt_check_begin;
    //nf_register_hook(& post_routing);


    //pre_routing
    pre_routing.pf = NFPROTO_IPV4;
    pre_routing.priority = NF_IP_PRI_CONNTRACK_DEFRAG-1;
    pre_routing.hooknum = NF_IP_LOCAL_IN;
    pre_routing.hook = pre_routing_begin;
    //nf_register_hook(& pre_routing);


    //out put does to localout and mangle the hdr

    local_out.pf = NFPROTO_IPV4;
    local_out.priority =NF_IP_PRI_CONNTRACK_DEFRAG-1;
    local_out.hooknum = NF_IP_LOCAL_OUT;
    local_out.hook = pkt_mangle_begin;
    nf_register_hook(& local_out);

    return 0;
}

static void __exit pkt_mangle_exit(void)
{
    //nf_unregister_hook(&post_routing);
    nf_unregister_hook(&local_out);
   // nf_unregister_hook(&pre_routing);
    printk(KERN_ALERT "\npkt_mangle output module stopped ...\n");
} 

module_init(pkt_mangle_init);
module_exit(pkt_mangle_exit);

MODULE_AUTHOR("Kelvin Zou: <xuanz@cs.princeton.edu>");
MODULE_DESCRIPTION("change the src && dst and extend tcp option field");
MODULE_LICENSE("GPL");