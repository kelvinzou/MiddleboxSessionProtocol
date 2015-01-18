#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include "uthash.h"
static struct sock *nl_sk = NULL;


static void netlink_agent(struct sk_buff *skb)
{ 
 	struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    int msg_size;
    char *msg = "Update Finished!";
    int res;

    printk(KERN_ALERT "Entering: %s\n", __FUNCTION__);

    nlh = (struct nlmsghdr *)skb->data;

    printk(KERN_ALERT "Netlink received msg payload: %s\n", (char *)nlmsg_data(nlh));
    
    record_t item;
    //here we just need SYNC packet, because SYN-ACK are rule is preinstalled first
    /*
    if (strcmp((char*)nlmsg_data(nlh), "SYN")==0){
        memset(&item, 0, sizeof(record_t));
        item.key.dst = in_aton( "10.0.3.2" );
        item.key.dport =5001;
        spin_lock(&slock);
        record_t * p=NULL;

        HASH_FIND(hh, records, &item.key, sizeof(record_key_t), p);

        if (p!=NULL) { 
            p->Migrate = 1;
            printk(KERN_ALERT "HASH migration modification happens!\n");
        }        
        spin_unlock(&slock);
    }
    */
    if (strcmp((char*)nlmsg_data(nlh), "RESET")==0){
        memset(&item, 0, sizeof(record_t));
        item.key.dst = in_aton( "10.0.3.2" );
        item.key.dport =5001;
        record_t * p=NULL;
        HASH_FIND(hh, records, &item.key, sizeof(record_key_t), p);
        
        spin_lock(&slock);
        if (p!=NULL) {
            p->Migrate = 0;
            p->dst =  in_aton("10.0.2.1");
            p->src =  in_aton("10.0.2.2");
            p->Seq = 0;
            p->Ack = 0;
            p->Dropped =0;
            p->NoRecvED = 0;
            p->Track =0;
        }
        spin_unlock(&slock);
    }
    
    if(strcmp((char*)nlmsg_data(nlh), "ACK")==0){
        memset(&item, 0, sizeof(record_t));
        item.key.dst = in_aton( "10.0.3.2" );
        item.key.dport =5001;
        record_t * p=NULL;

        HASH_FIND(hh, records, &item.key, sizeof(record_key_t), p);
        
        spin_lock(&slock);
        if (p!=NULL) {
            p->Migrate = 0;
            p->dst =  in_aton("10.0.4.1");
            p->src =  in_aton("10.0.4.2");
            //if the ack is being received from the old path
            if(p->NoRecvED ==0){
                p->Track =0;
            }
        }
        spin_unlock(&slock);
    }

    if(strcmp((char*)nlmsg_data(nlh), "SYN")==0){
        memset(&item, 0, sizeof(record_t));
        item.key.dst = in_aton( "10.0.3.2" );
        item.key.dport =5001;
        record_t * p=NULL;

        HASH_FIND(hh, records, &item.key, sizeof(record_key_t), p);
        
        spin_lock(&slock);
        if(p){
            p->NoRecvED = 1;
            p->pid =  nlh->nlmsg_pid ;
            p->Migrate = 1;
            p->Track = 1;
            printk(KERN_ALERT "The recorded max seq number is %u\n", p->Seq);
        }
        spin_unlock(&slock);
    }



    pid = nlh->nlmsg_pid; /*pid of sending process */
    msg_size = strlen(msg);
    skb_out = nlmsg_new(msg_size, 0);

    if (!skb_out)
    {
		printk(KERN_ALERT "Failed to allocate new skb\n");
		return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    
    

    printk(KERN_ALERT "finish one IPC! with the pid %d \n", pid);

    strncpy(nlmsg_data(nlh), msg, msg_size);

    res = nlmsg_unicast(nl_sk, skb_out, pid);


    if (res < 0)
        printk(KERN_ALERT "Error while sending bak to user\n");
}

MODULE_AUTHOR("Kelvin Zou: <xuanz@cs.princeton.edu>");
MODULE_DESCRIPTION("Interacting with user space function calls, atomic operations are done at kernel space");
MODULE_LICENSE("GPL");
