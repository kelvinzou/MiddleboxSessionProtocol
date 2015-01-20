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
static struct nf_hook_ops local_in;  

#define NETLINK_USER 31



static unsigned int post_routing_track(unsigned int hooknum, 
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *)) 
{
    struct iphdr  *iph;
    struct udphdr *udph;
    struct tcphdr * tcph;
    __u16 dst_port, src_port;
    struct sk_buff * retv;

    if (skb) {
        iph = (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);
        struct iphdr * iph2 =(struct iphdr *) skb_network_header(skb);
        printk("the two pointers are %d and %d\n", iph,iph2 );
        //do not change any non-TCP traffic
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_UDP && iph->protocol !=IPPROTO_TCP) ) {
            return NF_ACCEPT;
        } else if( iph->protocol ==IPPROTO_TCP){
        	tcph = (struct tcphdr *) tcp_hdr (skb );
        	if( ntohs(tcph->source)  == 5001){
        		printk("Do we modify the header? found %pI4 and value is %pI4  \n", &iph->saddr  , &iph->daddr);
        		__be32 oldIP = iph->daddr;
		        iph->daddr =  in_aton("128.112.93.106");
		        __be32 newIP = iph->daddr;
		        inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
	    		csum_replace4(&iph->check, oldIP, newIP);
        		printk( "POST: found %pI4 and value is %pI4  \n", &iph->saddr  , &iph->daddr) ;
        	}
        		 

        }
		return NF_ACCEPT;
    }
     return NF_ACCEPT;
}




struct timespec ts_start,ts_end,test_of_time;
//standard init and exit for a module 

static int __init pkt_mangle_init(void)
{   
    printk(KERN_ALERT "\npkt_mangle output module started ...\n");
    rwlock_init(&my_rwlock);
    rwlock_init(&release_lock);

    //pre_routing
    local_in.pf = NFPROTO_IPV4;
    local_in.priority = NF_IP_PRI_FIRST;
    local_in.hooknum = NF_IP_LOCAL_IN;
    local_in.hook = incoming_begin;
    nf_register_hook(&  local_in);

	post_routing.pf = NFPROTO_IPV4;
    post_routing.priority = NF_IP_PRI_NAT_DST+1;
    post_routing.hooknum = NF_IP_POST_ROUTING;
    post_routing.hook = post_routing_track;
    //nf_register_hook(&  post_routing);

    //out put does to localout and mangle the hdr

    local_out.pf = NFPROTO_IPV4;
    local_out.priority = NF_IP_PRI_LAST;
    local_out.hooknum = NF_IP_LOCAL_OUT;
    local_out.hook =  outgoing_begin;
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

	struct timespec ts_start,ts_end,test_of_time;

    //create a hash table
    //getnstimeofday(&ts_start);
    record_t l, *p, *r;


    //add hash entry in the hash table    
    r = (record_t*)kmalloc( sizeof(record_t) , GFP_KERNEL);
	memset(r, 0, sizeof(record_t));
    // this is middlebox copy
	r->key.dst = in_aton("10.0.6.2");
	r->key.sport =5001;
	//this is for old configure with old path
	
    r->dst =  in_aton("10.0.3.1");
    r->src =  in_aton("10.0.3.2");

    //r->dport = 5001;
    //this is for old configure with old path
    
    
    write_lock(&my_rwlock);
	HASH_ADD(hh, records, key, sizeof(record_key_t), r);
	write_unlock(&my_rwlock);
    

    return 0;

}

static void __exit pkt_mangle_exit(void)
{   
   	nf_unregister_hook(&local_out);
    nf_unregister_hook(&local_in);
   	//nf_unregister_hook(&post_routing);
    netlink_kernel_release(nl_sk);
    
    //this is hash table 
    struct timespec ts_start,ts_end,test_of_time;
    getnstimeofday(&ts_start);
     record_t  *p,  *tmp;
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
