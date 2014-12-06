#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include "uthash.h"
static struct sock *nl_sk = NULL;

void deleteHash(record_t * item){
    write_lock(&my_rwlock);
    record_t * p=NULL;
    HASH_FIND(hh, records, &item->key, sizeof(record_key_t), p);
    HASH_DEL(records, p);
    if (p)  {
        printk(KERN_ALERT "Kernel eviction happens!\n");
        kfree(p);
    }
    write_unlock(&my_rwlock);
}

void addHash(record_t * item){
    record_t  *r;

    r = (record_t*)kmalloc( sizeof(record_t) , GFP_KERNEL);
    memcpy((char *)r, (char *)item, sizeof(record_t));
    write_lock(&my_rwlock);
    HASH_ADD(hh, records, key, sizeof(record_key_t), r);
    write_unlock(&my_rwlock);
     printk(KERN_ALERT "Kernel insertion happens!\n");
}

static void netlink_agent(struct sk_buff *skb)
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
    
    record_t item;

    if (strcmp((char*)nlmsg_data(nlh), "del")==0) {
        memset(&item, 0, sizeof(record_t));
        item.key.dst = in_aton( "128.112.93.108" );
        item.key.dport =5001;
        deleteHash(&item);

        memset(&item, 0, sizeof(record_t));
        item.key.src = in_aton( "128.112.93.106" );
        item.key.sport =5001;
        deleteHash(&item);
    } 

    else if (strcmp((char*)nlmsg_data(nlh), "add")==0) {
        memset(&item, 0, sizeof(record_t));
        item.key.dst = in_aton( "128.112.93.108" );
        item.key.dport =5001;
        item.dst = in_aton("128.112.93.106");
        addHash(& item);

        memset(&item, 0, sizeof(record_t));
        item.key.src = in_aton( "128.112.93.106" );
        item.key.sport =5001;
        item.src =  in_aton("128.112.93.108");;
        addHash(& item);
    }
    


    pid = nlh->nlmsg_pid; /*pid of sending process */


    skb_out = nlmsg_new(msg_size, 0);

    if (!skb_out)
    {
		printk(KERN_ALERT "Failed to allocate new skb\n");
		return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    
    

    printk(KERN_ALERT "finish one IPC!\n");

    strncpy(nlmsg_data(nlh), msg, msg_size);

    res = nlmsg_unicast(nl_sk, skb_out, pid);


    if (res < 0)
        printk(KERN_ALERT "Error while sending bak to user\n");
}