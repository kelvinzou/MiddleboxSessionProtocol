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
#include <linux/inetdevice.h>
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>

#undef __KERNEL__
#include <linux/netfilter_ipv4.h>
#define __KERNEL__

#include "uthash.h"
#include "netlink_connection.h"

#include "in.h"
#include "out.h"
#include <stddef.h>  


static struct nf_hook_ops post_routing ;
static struct nf_hook_ops local_out;  
static struct nf_hook_ops pre_routing;

#define NETLINK_USER 31

struct timespec ts_start,ts_end,test_of_time;
//standard init and exit for a module 

static unsigned int local_buffer(unsigned int hooknum, 
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *)) 
{
    struct iphdr  *iph;
    struct udphdr *udph;
    struct tcphdr * tcph;
    __u16 dst_port, src_port;
    
    if (skb) {
        iph = (struct iphdr *) ip_hdr ( skb ); 
        //do not change any non-TCP traffic
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_UDP && iph->protocol !=IPPROTO_TCP) ) {
            return NF_ACCEPT;
        } else if( iph->protocol ==IPPROTO_TCP){
	    tcph = (struct tcphdr *) tcp_hdr ( skb ) ;
	    if( ntohs(tcph->dest)  == 5001 ){
		if (tcph->urg ==1){
		    tcph->urg =0;
		    printk("restore urgent pointer\n");
		}
	    }
        }
        else if( iph->protocol ==IPPROTO_UDP){
	    udph =  udp_hdr ( skb ) ;
	    if( ntohs(udph->dest)  == 5001){
		//do whatever
	    }
        } 
	return NF_ACCEPT;
    }
    return NF_ACCEPT;
} // reformated - raf


static int __init pkt_mangle_init(void)
{   
    time_counter=0;
	spin_lock_init(&slock);
    
    printk(KERN_ALERT "\npkt_mangle output module started ...\n");
    //pre_routing
    pre_routing.pf = NFPROTO_IPV4;
    pre_routing.priority =  NF_IP_PRI_FIRST;
    pre_routing.hooknum = NF_IP_LOCAL_IN;
    pre_routing.hook = incoming_begin;
    nf_register_hook(& pre_routing);

    post_routing.pf = NFPROTO_IPV4;
    post_routing.priority = NF_IP_PRI_LAST;
    post_routing.hooknum = NF_IP_POST_ROUTING;
    post_routing.hook = local_buffer;
   // nf_register_hook(&  post_routing);

    //out put does to localout and mangle the hdr

    local_out.pf = NFPROTO_IPV4;
    local_out.priority =  NF_IP_PRI_FIRST;
    local_out.hooknum = NF_IP_LOCAL_OUT;
    local_out.hook = outgoing_begin;  
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
    // this is to initilize the rules in the hash table
    record_t  *r;
    
    int counter = 1025;
    long timeValue;
    
    int portNum = 5000;
    for (portNum = 5000; portNum<5500; portNum++){

    	r = (record_t*)kmalloc( sizeof(record_t) , GFP_KERNEL);
    	memset(r, 0, sizeof(record_t));
    	r->key.dst = in_aton("10.0.3.2");
    	r->key.dport =portNum;
    	r->dst =  in_aton("10.0.2.1");
    	r->src =  in_aton("10.0.2.2");
    
   	 HASH_ADD(hh, records, key, sizeof(record_key_t), r);

    
    	r = (record_t*)kmalloc( sizeof(record_t) , GFP_KERNEL);
    	memset(r, 0, sizeof(record_t));
    	r->key.src = in_aton("10.0.2.1");
    	r->key.sport =portNum;
    	r->src =  in_aton("10.0.3.2");
    	r->dst =  in_aton("10.0.2.2");

    //r->dport = 5001;
    	HASH_ADD(hh, records, key, sizeof(record_key_t), r);

    	r = (record_t*)kmalloc( sizeof(record_t) , GFP_KERNEL);
    	memset(r, 0, sizeof(record_t));
    	r->key.src = in_aton("10.0.4.1");
    	r->key.sport =5001;
    	r->src =  in_aton("10.0.3.2");
    	r->dst =  in_aton("10.0.2.2");
    
    //r->dport = 5001;
    	HASH_ADD(hh, records, key, sizeof(record_key_t), r);
    
   }
    
    //getnstimeofday(&ts_end);
    //test_of_time = timespec_sub(ts_end,ts_start);
    return 0;

}

static void __exit pkt_mangle_exit(void)
{
    nf_unregister_hook(&local_out);
    nf_unregister_hook(&pre_routing);
   // nf_unregister_hook(&post_routing);

    netlink_kernel_release(nl_sk);

    
    //this is hash table 
    struct timespec ts_start,ts_end,test_of_time;
    getnstimeofday(&ts_start);
     record_t  *p;
     record_t *tmp;
     int counter;
     counter = 0;
     
     HASH_ITER(hh, records, p, tmp) {
     printk("The tracked max seq number is %u and %u\n", p->Seq, p->Ack);
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
