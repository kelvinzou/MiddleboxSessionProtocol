#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include "uthash.h"
static struct sock *nl_sk = NULL;

void HashMigrate(record_t * item){
    
    record_t * p=NULL;
    
    HASH_FIND(hh, records, &item->key, sizeof(record_key_t), p);
    if (p!=NULL) {
        p->Migrate = 1;
        p->src =  in_aton("10.0.1.2");
        p->dst =  in_aton("10.0.1.1");
        printk(KERN_ALERT "HASH migration modification happens!\n");
    }
}


void HashReleaseBuffer(record_t * item){
    record_t * p=NULL;
    HASH_FIND(hh, records, &item->key, sizeof(record_key_t), p);
    if (p!=NULL) {
        p->Migrate = 1;
        printk(KERN_ALERT "HASH buffer modification happens!\n");
    }
}

void HashResetMigration(record_t * item){
    record_t * p=NULL;
    
    HASH_FIND(hh, records, &item->key, sizeof(record_key_t), p);
    if (p!=NULL) {
        p->Migrate =0;
        printk(KERN_ALERT "HASH buffer reset happens!\n");
        p->dst =  in_aton("10.0.3.1");
        p->src =  in_aton("10.0.3.2");
    }
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
    
    

    record_t item;
    //here we just need SYNC packet, because SYN-ACK are rule is preinstalled first
    if (strcmp((char*)nlmsg_data(nlh), "SYN")==0){
        memset(&item, 0, sizeof(record_t));
        item.key.dst = in_aton( "10.0.2.2" );
        item.key.sport =5001;
            
        HashMigrate(&item);
    }
    if (strcmp((char*)nlmsg_data(nlh), "LOCK")==0){
        write_lock(&release_lock);
    }
    if (strcmp((char*)nlmsg_data(nlh), "UNLOCK")==0){
        write_unlock(&release_lock);
    }
    if(strcmp((char*)nlmsg_data(nlh), "ACK")==0){
        memset(&item, 0, sizeof(record_t));
        item.key.dst =  in_aton( "10.0.2.2" );
        item.key.sport =5001;

        HashReleaseBuffer(&item);
    }
    if(strcmp((char*)nlmsg_data(nlh), "RESET")==0){
        memset(&item, 0, sizeof(record_t));
        item.key.dst =  in_aton( "10.0.2.2" );
        item.key.sport =5001;

        HashResetMigration(&item);
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
    
    printk(KERN_ALERT "Unlocked\n");

    strncpy(nlmsg_data(nlh), msg, msg_size);

    res = nlmsg_unicast(nl_sk, skb_out, pid);


    if (res < 0)
        printk(KERN_ALERT "Error while sending bak to user\n");
}
