#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include "uthash.h"
static struct sock *nl_sk = NULL;


void netlink_notify(int pid, char * msg){
    struct sk_buff *skb_out;
    int msg_size = strlen(msg);
    skb_out = nlmsg_new(msg_size, 0);

    if (!skb_out)
    {
        printk(KERN_ALERT "Failed to allocate new skb\n");
        return;
    }
    struct nlmsghdr *nlh;
    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */

    printk(KERN_ALERT "Sending back with the pid %d \n", pid);

    strncpy(nlmsg_data(nlh), msg, msg_size);

    int res = nlmsg_unicast(nl_sk, skb_out, pid);
    if (res < 0) printk(KERN_ALERT "Error while sending bak to user\n");
}

static void netlink_agent(struct sk_buff *skb)
{ 
 	struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    int msg_size;
    int res;
    char *msg = "Update Finished!";
    printk(KERN_ALERT "Entering: %s\n", __FUNCTION__);

    nlh = (struct nlmsghdr *)skb->data;

    printk(KERN_ALERT "Netlink received msg payload: %s\n", (char *)nlmsg_data(nlh));
    
    record_t item;
     pid = nlh->nlmsg_pid;
    if (strcmp((char*)nlmsg_data(nlh), "RESET")==0){
        memset(&item, 0, sizeof(record_t));
        item.key.dst = in_aton("52.5.27.99");
        item.key.dport =5001;
        record_t * p=NULL;
        HASH_FIND(hh, records, &item.key, sizeof(record_key_t), p);

       if (p!=NULL) {
            p->Migrate = 0;
            p->dst =  in_aton("52.6.55.195");
            //p->src =  in_aton("52.8.21.243");
        }
        char * msg = "RESET";
        netlink_notify(pid, msg);
    }
    
    if(strcmp((char*)nlmsg_data(nlh), "ACK")==0){
        memset(&item, 0, sizeof(record_t));
        item.key.dst = in_aton( "52.5.27.99" );
        item.key.dport =5001;
        record_t * p=NULL;
        printk(KERN_ALERT "ACK is received!\n" );
        HASH_FIND(hh, records, &item.key, sizeof(record_key_t), p);
        
        if (p!=NULL) {  
             p->dst =  in_aton("52.24.102.104");
           printk(KERN_ALERT "Resume transferring!\n" );          
            p->Migrate = 0;
        }
    }

    if(strcmp((char*)nlmsg_data(nlh), "SYN")==0){
        memset(&item, 0, sizeof(record_t));
        item.key.dst = in_aton("52.5.27.99");
        item.key.dport =5001;
        record_t * p=NULL;

        HASH_FIND(hh, records, &item.key, sizeof(record_key_t), p);
        
        if(p){
             p->dst =  in_aton("52.24.102.104");
          // p->src =  in_aton("52.8.21.243");
 
            p->Migrate = 1;
            printk(KERN_ALERT "SYN is received!\n" );
        }

    }
    skb_out = nlmsg_new(msg_size, 0);
    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    


    strncpy(nlmsg_data(nlh), msg, msg_size);

    res = nlmsg_unicast(nl_sk, skb_out, pid);


    return;
}

MODULE_AUTHOR("Kelvin Zou: <xuanz@cs.princeton.edu>");
MODULE_DESCRIPTION("Interacting with user space function calls, atomic operations are done at kernel space");
MODULE_LICENSE("GPL");
