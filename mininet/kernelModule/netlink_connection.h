#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include "uthash.h"
static struct sock *nl_sk = NULL;



static void hello_nl_recv_msg(struct sk_buff *skb)
{ 
 	struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    int msg_size;
    char *msg = "Hello from kernel";
    int res;

    printk(KERN_ALERT "Entering: %s\n", __FUNCTION__);




    struct timespec ts_start,ts_end,test_of_time;

    //create a hash table
    getnstimeofday(&ts_start);

    int i;
    record_t l, *p;
    memset(&l, 0, sizeof(record_t));
    p=NULL;
    //get_random_bytes ( &i, sizeof (int) );
    i=1;
	l.key.a =i;
	l.key.b =i+5;
	HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);
    getnstimeofday(&ts_end);

    test_of_time = timespec_sub(ts_end,ts_start);
    printk(KERN_ALERT "Finding takes time %lu", test_of_time.tv_nsec);
    printk(KERN_ALERT "Finding value is  time %d", i);
    if (p) printk( KERN_ALERT "found %d %d and value is %d \n", p->key.a, p->key.b, p->a);


    msg_size = strlen(msg);

    nlh = (struct nlmsghdr *)skb->data;
    printk(KERN_ALERT "Netlink received msg payload: %s\n", (char *)nlmsg_data(nlh));
    pid = nlh->nlmsg_pid; /*pid of sending process */

    skb_out = nlmsg_new(msg_size, 0);

    if (!skb_out)
    {

        printk(KERN_ALERT "Failed to allocate new skb\n");
        return;

    }
    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    strncpy(nlmsg_data(nlh), msg, msg_size);

    res = nlmsg_unicast(nl_sk, skb_out, pid);

	
    
    if (res < 0)
        printk(KERN_ALERT "Error while sending bak to user\n");
}