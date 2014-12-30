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
#include "in.h"
#include "out.h"
#include "netlink_connection.h"
#include <stddef.h>  
static struct nf_hook_ops post_routing ;
static struct nf_hook_ops local_out;  
static struct nf_hook_ops local_in;  

static struct nf_hook_ops pre_routing;

#define NETLINK_USER 31

struct timespec ts_start,ts_end,test_of_time;
//standard init and exit for a module 

static unsigned int input_change(unsigned int hooknum, 
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *)) 
{
    struct iphdr  *iph;
    __u16 dst_port, src_port;
    struct sk_buff * retv;

    if (skb) {
        iph =ip_hdr (skb);

        //do not change any non-UDP traffic
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_UDP && iph->protocol !=IPPROTO_TCP) ) {
            return NF_ACCEPT;
        }        
        else if( iph->protocol ==IPPROTO_TCP) {
            struct tcphdr *tcph ;

            unsigned int data_len;
            data_len = skb->len;
            tcph =   tcp_hdr (skb );
            
            if(ntohs(tcph->dest) == 80 )
            {
                __be32 oldIP = iph->daddr;
                iph->daddr = in_aton("157.166.226.26");
                __be32  newIP = iph->daddr;
				inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
                csum_replace4(&iph->check, oldIP, newIP);
                /*
                __be16 oldPort = tcph->dest;
                tcph->dest = htons(3128);
                __be16 newPort = tcph->dest;
                inet_proto_csum_replace2(&tcph->check, skb, oldPort, newPort, 0);
                */
                 
                printk( "Incoming: found %pI4 and value is %pI4  \n", &oldIP, &newIP);
               // okfn(skb);
                return NF_ACCEPT;
            } 
          	 return NF_ACCEPT;
        }
         return NF_ACCEPT;
    }
    
}

static unsigned int output_change(unsigned int hooknum, 
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *)) 
{
    struct iphdr  *iph;
    __u16 dst_port, src_port;
    struct sk_buff * retv;

    if (skb) {
        iph =ip_hdr (skb);

        //do not change any non-UDP traffic
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_UDP && iph->protocol !=IPPROTO_TCP) ) {
            return NF_ACCEPT;
        }        
        else if( iph->protocol ==IPPROTO_TCP) {
            struct tcphdr *tcph ;

            unsigned int data_len;
            data_len = skb->len;
            tcph =   tcp_hdr (skb );
            if(ntohs(tcph->dest) ==80){
            printk("So we do relay the packets?\n");
            }            
            if ( ntohs(tcph->source) ==80 ) {
                __be32 oldIP = iph->daddr;
                iph->daddr = in_aton("10.0.3.1");
                __be32 newIP = iph->daddr;
                printk( KERN_ALERT "Outgoing: found %pI4 and value is %pI4  \n", &oldIP, &newIP);

                csum_replace4(&iph->check, oldIP, newIP);
                inet_proto_csum_replace4(&tcph->check, skb, oldIP, newIP, 1);
                ip_route_me_harder(skb, RTN_UNSPEC);
            }
        	return NF_ACCEPT;
        }
         return NF_ACCEPT;
    }
    
}
static int __init pkt_mangle_init(void)
{   
    printk(KERN_ALERT "\npkt_mangle output module started ...\n");
    
    //pre_routing
    pre_routing.pf = NFPROTO_IPV4;
    pre_routing.priority =  NF_IP_PRI_FIRST;
    pre_routing.hooknum = NF_IP_PRE_ROUTING;
    pre_routing.hook = incoming_change_begin;
    nf_register_hook(& pre_routing);

     /* 
	local_out.pf = NFPROTO_IPV4;
    local_out.priority  = NF_IP_PRI_FIRST;
    local_out.hooknum = NF_IP_POST_ROUTING;
    local_out.hook = output_change;
    //nf_register_hook(& local_out);

    local_in.pf = NFPROTO_IPV4;
    local_in.priority  = NF_IP_PRI_FIRST;
    local_in.hooknum = NF_IP_PRE_ROUTING;
    local_in.hook = input_change;
  //  nf_register_hook(& local_in);
    */
    
    
    //output does to localout and mangle the hdr
    post_routing.pf = NFPROTO_IPV4;
    post_routing.priority = NF_IP_PRI_LAST;
    post_routing.hooknum = NF_IP_POST_ROUTING;
    post_routing.hook =  outgoing_change_begin;
    nf_register_hook(&  post_routing);


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
	r->key.src = in_aton("10.0.3.1");
	r->key.dport =5001;
    r->src = in_aton("10.0.1.2");
    r->dst = in_aton("10.0.1.1");
    //r->dport = 5001;

    HASH_ADD(hh, records, key, sizeof(record_key_t), r);

    r = (record_t*)kmalloc( sizeof(record_t) , GFP_KERNEL);
    memset(r, 0, sizeof(record_t));

    r->key.src = in_aton("10.0.1.1");
    r->key.sport =5001;
    r->src = in_aton("10.0.3.2");
    r->dst = in_aton("10.0.3.1");
	
    HASH_ADD(hh, records, key, sizeof(record_key_t), r);

    return 0;

}


static void __exit pkt_mangle_exit(void)
{
   
   nf_unregister_hook(&post_routing);
   nf_unregister_hook(&pre_routing);
  //  nf_unregister_hook(&local_in);
    //nf_unregister_hook(&local_out);


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
