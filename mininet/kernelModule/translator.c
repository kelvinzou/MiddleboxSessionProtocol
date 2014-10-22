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

#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>

#undef __KERNEL__
#include <linux/netfilter_ipv4.h>
#define __KERNEL__

#include "uthash.h"
#include "pre_routing.h"
#include "post_routing.h"
#include "local_out.h"
#include "netlink_connection.h"
#include <stddef.h>  
static struct nf_hook_ops post_routing ;
static struct nf_hook_ops local_out;  
static struct nf_hook_ops pre_routing;

#define NETLINK_USER 31


 record_t  *records = NULL;

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


    //net link also initilizaed here
    printk(KERN_ALERT "Entering: %s\n", __FUNCTION__);
    struct netlink_kernel_cfg cfg = {
                .groups = 1,
                .input = hello_nl_recv_msg,
        };
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);

    if (!nl_sk)
    {

        printk(KERN_ALERT "Error creating socket.\n");
        return -10;

    }



    //create a hash table
    record_t l, *p, *r;

    r = (record_t*)kmalloc( sizeof(record_t) , GFP_KERNEL);
    memset(r, 0, sizeof(record_t));
    r->key.a =5;
    r->key.b = 1;
    HASH_ADD(hh, records, key, sizeof(record_key_t), r);

    memset(&l, 0, sizeof(record_t));
    l.key.a = 3;
    l.key.b = 1;
    HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);

    if (p) printk( KERN_ALERT "found %c %d\n", p->key.a, p->key.b);



    return 0;

}

static void __exit pkt_mangle_exit(void)
{
    //nf_unregister_hook(&post_routing);
    nf_unregister_hook(&local_out);
   // nf_unregister_hook(&pre_routing);
    netlink_kernel_release(nl_sk);
    //this is hash table 
     record_t  *p,  *tmp;
     HASH_ITER(hh, records, p, tmp) {
     HASH_DEL(records, p);
      kfree(p);
    }
    printk(KERN_ALERT "\npkt_mangle output module stopped ...\n");

} 




module_init(pkt_mangle_init);
module_exit(pkt_mangle_exit);

MODULE_AUTHOR("Kelvin Zou: <xuanz@cs.princeton.edu>");
MODULE_DESCRIPTION("change the src && dst and extend tcp option field");
MODULE_LICENSE("GPL");
