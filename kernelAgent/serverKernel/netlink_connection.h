#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include "uthash.h"
static struct sock *nl_sk = NULL;

void addHash(record_t * item){
    record_t  *r;

    r = (record_t*)kmalloc( sizeof(record_t) , GFP_KERNEL);
    memcpy((char *)r, (char *)item, sizeof(record_t));
    write_lock(&my_rwlock);
    HASH_ADD(hh, records, key, sizeof(record_key_t), r);
    write_unlock(&my_rwlock);
     printk(KERN_ALERT "Kernel insertion happens!\n");
}

static void hello_nl_recv_msg(struct sk_buff *skb)
{ 
 	struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    int msg_size;
    char *msg = "Update Finished!";
    int res;

    printk(KERN_ALERT "Entering: %s\n", __FUNCTION__);


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
    
    record_t l, *p;
    printk(KERN_ALERT "Kernel eviction starts!\n");
    if (strcmp((char*)nlmsg_data(nlh), "del")==0) {
    
    

    memset(&l, 0, sizeof(record_t));
    l.key.dst = in_aton( "128.112.93.107" );
    l.key.sport =5001;
    write_lock(&my_rwlock);
    HASH_FIND(hh, records, &l.key, sizeof(record_key_t), p);
    HASH_DEL(records, p);
    write_unlock(&my_rwlock);
    if (p)  {
	    printk(KERN_ALERT "Kernel eviction happens!\n");
	    kfree(p);
    }
    } else if (strcmp((char*)nlmsg_data(nlh), "add")==0){
         memset(&l, 0, sizeof(record_t));
        l.key.dst = in_aton( "128.112.93.107" );
        l.key.sport =5001;
        l.dst= in_aton("128.112.93.106");
        addHash(&l);
    }
    
    
    
    printk(KERN_ALERT "Unlocked\n");

    strncpy(nlmsg_data(nlh), msg, msg_size);

    res = nlmsg_unicast(nl_sk, skb_out, pid);


    if (res < 0)
        printk(KERN_ALERT "Error while sending bak to user\n");
}
