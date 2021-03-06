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
#include <linux/spinlock.h>
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
#include "in.h"
#include "out.h"
#include "netlink_connection.h"
#include <stddef.h>  
static struct nf_hook_ops post_routing ;
static struct nf_hook_ops local_out;  
static struct nf_hook_ops pre_routing;

#define NETLINK_USER 31

struct timespec ts_start,ts_end,test_of_time;
//standard init and exit for a module 

static int __init pkt_mangle_init(void)
{   
    printk(KERN_ALERT "\npkt_mangle output module started ...\n");
    rwlock_init(&my_rwlock);
    //pre_routing
    pre_routing.pf = NFPROTO_IPV4;
    pre_routing.priority =  NF_IP_PRI_CONNTRACK_DEFRAG -1;
    pre_routing.hooknum = NF_IP_PRE_ROUTING;
    pre_routing.hook = incoming_begin;
    nf_register_hook(& pre_routing);


    //out put does to localout and mangle the hdr

    local_out.pf = NFPROTO_IPV4;
    local_out.priority = NF_IP_PRI_NAT_DST;
    local_out.hooknum = NF_IP_LOCAL_OUT;
    local_out.hook =  outgoing_begin;
    nf_register_hook(& local_out);


    //net link also initilizaed here
    printk(KERN_ALERT "Entering: %s\n", __FUNCTION__);
    struct netlink_kernel_cfg cfg = {
                .groups = 1,
                .input = netlink_agent,
        };
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);

    if (!nl_sk)
    {
        printk(KERN_ALERT "Error creating socket.\n");
        return -10;

    }

	struct timespec ts_start,ts_end,test_of_time;

    //create a hash table
    //getnstimeofday(&ts_start);
    record_t  *r;

    //add hash entry in the hash table    
    r = (record_t*)kmalloc( sizeof(record_t) , GFP_KERNEL);
	memset(r, 0, sizeof(record_t));
    // this is middlebox copy
	r->key.dst = in_aton("128.112.93.108");
	r->key.dport =5001;
    r->dst =  in_aton("128.112.93.106");
    write_lock(&my_rwlock);
	HASH_ADD(hh, records, key, sizeof(record_key_t), r);
	write_unlock(&my_rwlock);


    r = (record_t*)kmalloc( sizeof(record_t) , GFP_KERNEL);
    memset(r, 0, sizeof(record_t));
    r->key.src = in_aton("128.112.93.106");
    r->key.sport =5001;
    r->src =  in_aton("128.112.93.108");
	write_lock(&my_rwlock);
    HASH_ADD(hh, records, key, sizeof(record_key_t), r);
    write_unlock(&my_rwlock);

    
    //getnstimeofday(&ts_end);
    //test_of_time = timespec_sub(ts_end,ts_start);
    return 0;

}

static void __exit pkt_mangle_exit(void)
{
    //nf_unregister_hook(&post_routing);
   
    nf_unregister_hook(&local_out);
    nf_unregister_hook(&pre_routing);
   
    netlink_kernel_release(nl_sk);

    
    //this is hash table 
    struct timespec ts_start,ts_end,test_of_time;
    getnstimeofday(&ts_start);
     record_t  *p;
     record_t *tmp;
     int counter;
     counter = 0;
     HASH_ITER(hh, records, p, tmp) {
     HASH_DEL(records, p);
     counter ++;
      kfree(p);
    }
    getnstimeofday(&ts_end);
    test_of_time = timespec_sub(ts_end,ts_start);
    printk(KERN_ALERT "Deletion takes time %lu", test_of_time.tv_nsec);

    printk(KERN_ALERT "\n delete %d \n", counter);
    printk(KERN_ALERT "\npkt_mangle output module stopped ...\n");

} 




module_init(pkt_mangle_init);
module_exit(pkt_mangle_exit);

MODULE_AUTHOR("Kelvin Zou: <xuanz@cs.princeton.edu>");
MODULE_DESCRIPTION("change the src && dst and extend tcp option field");
MODULE_LICENSE("GPL");
