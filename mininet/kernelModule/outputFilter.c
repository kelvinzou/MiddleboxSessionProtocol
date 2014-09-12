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

#undef __KERNEL__
#include <linux/netfilter_ipv4.h>
#define __KERNEL__


#define IP_HDR_LEN 20
#define UDP_HDR_LEN 8
#define TCP_HDR_LEN 20
#define TOT_HDR_LEN 28


//value is 192.168.0.1
//static unsigned char *ip_address = "\xC0\xA8\x00\x01"; 
//127.0.0.1
static unsigned char *ip_address2 = "\x7F\x00\x00\x01"; 


static unsigned int pkt_mangle_begin(unsigned int hooknum,
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *));

static int header_rewrite(struct sk_buff *skb, char * ip_dest);

static struct nf_hook_ops pkt_mangle_ops __read_mostly = {
    .pf = NFPROTO_IPV4,
    .priority = 1,
    .hooknum = NF_IP_LOCAL_OUT,
    .hook = pkt_mangle_begin,
};


//standard init and exit for a module 

static int __init pkt_mangle_init(void)
{
    printk(KERN_ALERT "\npkt_mangle output module started ...\n");
    return nf_register_hook(&pkt_mangle_ops);
}

static void __exit pkt_mangle_exit(void)
{
    nf_unregister_hook(&pkt_mangle_ops);
    printk(KERN_ALERT "\n pkt_mangle output module stopped ...\n");
} 




//body for packet registration and modification
static int header_rewrite(struct sk_buff *skb, char * ip_dest){
    __u16 dst_port, src_port;
    struct iphdr *iph, *iphOld;
    struct tcphdr *tcph;
    struct udphdr *udph;
    unsigned char * headerPtr, *ptr;
    //whole IP packet now becomes a body
    unsigned int data_len = skb->len;
    printk(KERN_ALERT "Old skb checksum is %lu\n", skb->csum);

    iph = (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);
    iphOld = iph;
    printk(KERN_ALERT "Output: Initial Source and Dest address is  %pI4 and  %pI4 \n", 
        &iph->saddr, &iph->daddr );
    tcph = (struct tcphdr *) skb_header_pointer (skb, IP_HDR_LEN, 0, NULL);
    printk(KERN_ALERT "Output: Initial udp port number is %d and %d \n", 
        ntohs(tcph->source), ntohs(tcph ->dest));
    src_port = ntohs (tcph->source);
    dst_port = ntohs (tcph->dest);
    
    //create new space at the head of socket buffer
    printk(KERN_ALERT "Output: Push header in front of the old header\n");
    ptr= skb_push(skb, sizeof(struct udphdr));
    udph = (struct udphdr *) ptr;
    udph->source = htons(src_port) ;
    udph->dest  = htons(dst_port) ;
    udph->len = htons( data_len + sizeof(struct udphdr));
    //update UDP header csum
    skb->csum = csum_partial((char *)udph,
                 sizeof(struct udphdr), skb->csum);
    udph->check = 0;

    udph->check = ~csum_tcpudp_magic((iph->saddr), (iph->daddr), data_len+sizeof(struct udphdr), IPPROTO_UDP, skb->csum);
    if (udph->check == 0)
        udph->check = -1;


    //THis is for IP header
    headerPtr= skb_push(skb, sizeof(struct iphdr));
    iph = (struct iphdr *) headerPtr;
    iph->version = 4;
    iph->ihl = 5;
    iph->tos = iphOld->tos;
    iph->tot_len = htons(skb->len);
    iph->frag_off = 0;
    iph->id =iphOld->id;
    iph->ttl = 64;
    iph->protocol = IPPROTO_UDP; /* IPPROTO_UDP in this case */
    iph->saddr = iphOld->saddr;
    iph->daddr = in_aton("10.0.0.1");//iphOld->daddr;
    ip_send_check(iph);


    /*

    //copy IP and create UDP header, including copying the ports number.
    memcpy(headerPtr, headerPtr+TOT_HDR_LEN, IP_HDR_LEN);
    //move the new header pointer to 28 bytes ahead
    iph = (struct iphdr *)skb_header_pointer (skb, 0, 0, NULL);
    iph->protocol = IPPROTO_UDP;
    ptr = headerPtr + IP_HDR_LEN;

    udph = (struct udphdr *) ptr;
    udph->source = htons(src_port) ;
    udph->dest  = htons(dst_port) ;


    //re-compute the length
    iph->tot_len = htons( data_len + TOT_HDR_LEN);
    

    //update  IP header
    iph->daddr =in_aton("127.0.0.1");

    */
    //log information
    printk(KERN_ALERT "Output: Packet length is %d\n\n", data_len);

    printk(KERN_ALERT "Output: Src and Dest address is %pI4 and  %pI4\n", 
        &iph->saddr ,&iph->daddr );
     
    printk(KERN_ALERT "Output: Final Packet length is %d\n", skb->len);

    printk(KERN_ALERT "Output: Final udp port number is %d and %d \n", 
        ntohs(udph->source), ntohs(udph ->dest));

    
    
    //update checksum

   // ip_send_check(iph);

    return 0;

    //return NULL;
}


static unsigned int pkt_mangle_begin (unsigned int hooknum,
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *))
{ 
    struct iphdr *iph;
    struct tcphdr *tcph;
    __u16 dst_port, src_port;

    if (skb) {
        iph = (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);

        //do not change any non-TCP traffic
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_TCP) ) {
            return NF_ACCEPT;
        } else{
            //printk(KERN_ALERT "ever pass TCP\n");
            tcph = (struct tcphdr *) skb_header_pointer (skb, IP_HDR_LEN, 0, NULL);
            src_port = ntohs (tcph->source);
            dst_port = ntohs (tcph->dest);
            //do not change any non-special traffic
            if (dst_port !=1234 && src_port !=1234){
                return NF_ACCEPT;
            }
            else 
             {
                printk(KERN_ALERT "Output: ever get to modification phase? \n");
                header_rewrite(skb, ip_address2);
                return NF_ACCEPT;
            } 
            
        }
    }
     return NF_ACCEPT;
}

    /*
    struct iphdr *iph;
    struct udphdr *tcph;
    unsigned char *data;

    unsigned int data_len;
    unsigned char extra_data[] = "12345";
    unsigned char *temp;
    unsigned int extra_data_len;
    unsigned int tot_data_len;

    unsigned int i;

    __u16 dst_port, src_port;

    if (skb) {
        iph = (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);

        if (iph && iph->protocol &&(iph->protocol == IPPROTO_UDP)) {
            udph = (struct udphdr *) skb_header_pointer (skb, IP_HDR_LEN, 0, NULL);
            src_port = ntohs (udph->source);
            dst_port = ntohs (udph->dest);

            if (src_port == 6000) {
                printk(KERN_ALERT "UDP packet goes out");
                data = (unsigned char *) skb_header_pointer (skb, IP_HDR_LEN+UDP_HDR_LEN, 0, NULL);
                data_len = skb->len - TOT_HDR_LEN;

                temp = kmalloc(512 * sizeof(char), GFP_ATOMIC);
                memcpy(temp, data, data_len);

                unsigned char *ptr = temp + data_len - 1;
                extra_data_len = sizeof(extra_data);
                memcpy(ptr, extra_data, extra_data_len);
                tot_data_len = data_len + extra_data_len - 1;

                skb_put(skb, extra_data_len - 1);

                memcpy(data, temp, tot_data_len);

                //Manipulating necessary header fields 
                iph->tot_len = htons(tot_data_len + TOT_HDR_LEN);
                udph->len = htons(tot_data_len + UDP_HDR_LEN);

                //Calculation of IP header checksum 
                iph->check = 0;
                ip_send_check (iph);

                // Calculation of UDP checksum 
                udph->check = 0;
                int offset = skb_transport_offset(skb);
                int len = skb->len - offset;
                udph->check = ~csum_tcpudp_magic((iph->saddr), (iph->daddr), len, IPPROTO_UDP, 0);

                 }
        }
    }
    */


module_init(pkt_mangle_init);
module_exit(pkt_mangle_exit);

MODULE_AUTHOR("Kelvin Zou: <xuanz@cs.princeton.edu>");
MODULE_DESCRIPTION("add ip/udp header on top of ip header");
MODULE_LICENSE("GPL");