#ifndef _LOCAL_OUT_H_
#define _LOCAL_OUT_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/if_packet.h>
#include <linux/crypto.h>
#include <linux/init.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/inet.h>
#include <linux/tcp.h>
#include <net/udp.h>
#include <net/inet_sock.h>
#include <net/flow.h>
#include <net/route.h>
#include <linux/inetdevice.h>

struct sk_buff * header_rewrite(struct sk_buff *skb){ 

    struct iphdr *iph, *iphOld;
    struct udphdr *udph, *udphOld;
    //whole IP packet now becomes a body
    unsigned int data_len = skb->len;

    iph = (struct iphdr *) ip_hdr (skb ); 
    udph = (struct udphdr *) udp_hdr (skb );
    printk(KERN_ALERT "Output: Initial udp port number is %d and %d \n", 
        ntohs(udph->source), ntohs(udph ->dest)); 
    size_t iphdr_len = sizeof (struct iphdr);
    size_t udphdr_len = sizeof (struct udphdr) ;
    //create new space at the head of socket buffer
    printk(KERN_ALERT "Output: Push header in front of the old header\n");

   // printk(KERN_ALERT "headroom is %d large\n", skb_headroom(skb));

    if (skb_headroom(skb) < (iphdr_len+udphdr_len)) {
        printk(KERN_ALERT "Output: After skb_push lalala Push header in front of the old header\n");
        struct sk_buff * skbOld = skb;
        skb = skb_realloc_headroom(skbOld, iphdr_len+udphdr_len);
        if (!skb) {
                printk(KERN_ERR "vlan: failed to realloc headroom\n");
                return NULL;
        }
         if(skbOld->sk){
            skb_set_owner_w(skb, skbOld->sk);
    }
    } 
    udphOld = udph ;
    iphOld =  iph;
    
    char * csumpointer= skb_push(skb, udphdr_len);
    skb_reset_transport_header(skb);   
    udph =  udp_hdr(skb);
    
    udph->source = udphOld->source ;
    udph->dest  = udphOld->dest;
    udph->len = htons( data_len + sizeof(struct udphdr));
    //update UDP header csum

    

    //This is for IP header
    //skb_push(skb, sizeof(struct iphdr));
    skb_push(skb, iphdr_len);
    skb_reset_network_header(skb);  
    iph =ip_hdr(skb);
    //bunch of values are copied to new ip header
    iph->version = iphOld->version;
    iph->ihl = iphOld->ihl;
    iph->tos = iphOld->tos;
    iph->frag_off = iphOld->frag_off;
    iph->id =iphOld->id;
    iph->ttl = iphOld->ttl;
    //change the protocol
    iph->protocol = IPPROTO_UDP; 
    //change the length
    iph->tot_len = htons(skb->len);

    //IP addresses
    iph->saddr = iphOld->saddr;
    /*****************************************************
    ******************************************************
    this is used for testing, should be dynamic eventually 
    *****************************************************/
    iph->daddr = in_aton("192.168.56.102");

   
    //update UPD checksum after update IP header
    udph->check = 0;
    skb->csum = csum_partial((char *)udph,
                 sizeof(struct udphdr)+data_len,0);
    udph->check = csum_tcpudp_magic((iph->saddr), (iph->daddr), data_len+sizeof(struct udphdr), IPPROTO_UDP,skb->csum);


    ip_send_check(iph);

    //skb->pkt_type = PACKET_OUTGOING;
    printk(KERN_ALERT "Output: Final udp port number is %d and %d \n", 
        ntohs(udph->source), ntohs(udph ->dest)); 
    printk(KERN_ALERT "Output: Packet length is %d\n", data_len);
    printk(KERN_ALERT "Output: Initial Source and Dest address is  %pI4 and  %pI4 \n", 
        &iphOld->saddr, &iphOld->daddr );
    printk(KERN_ALERT "Output: Src and Dest address is %pI4 and  %pI4\n", 
        &iph->saddr ,&iph->daddr );
    printk(KERN_ALERT "Output: Final Packet length is %d\n\n", skb->len);
    return  skb ;
}

struct sk_buff * tcp_header_rewrite(struct sk_buff *skb){ 

    struct iphdr *iph, *iphStore;
    struct tcphdr *tcph, *tcpStore;

    unsigned int data_len = skb->len;

    iph = (struct iphdr *) ip_hdr (skb ); 
    tcph = (struct tcphdr *) tcp_hdr (skb );



    printk(KERN_ALERT "Output: Initial tcp port number is %d and %d \n", ntohs(tcph->source), ntohs(tcph ->dest)); 
    size_t iphdr_len = sizeof (struct iphdr);
    size_t tcphdr_len =  tcph->doff*4 ;
    //create new space at the head of socket buffer
    printk(KERN_ALERT "Output: header length is %d\n", tcphdr_len);
    printk(KERN_ALERT "headroom is %d large\n", skb_headroom(skb));

    struct iphdr ipmem;
    memcpy(&ipmem, iph, iphdr_len);
    iphStore = & ipmem;

    printk(KERN_ALERT "Output: Initial Source and Dest address is  %pI4 and  %pI4 \n", 
        &iphStore->saddr, &iphStore->daddr );

    __u32 memaddr[tcph->doff];
    memcpy(&memaddr, tcph, tcphdr_len);
    tcpStore = (struct tcphdr *) &memaddr;
    printk(KERN_ALERT "Output: Initial tcp port number is %d and %d \n", ntohs(tcpStore->source), ntohs(tcpStore ->dest)); 


    //update the head room if needed
    if (skb_headroom(skb) < 60- tcphdr_len ) {
        printk(KERN_ALERT "Output: After skb_push Push header in front of the old header\n");
        struct sk_buff * skbOld = skb;
        skb = skb_realloc_headroom(skbOld, 60- tcphdr_len);
        if (!skb) {
                printk(KERN_ERR "vlan: failed to realloc headroom\n");
                return NULL;
        }
        if(skbOld->sk){
            skb_set_owner_w(skb, skbOld->sk);
        }
    } 

    skb_push(skb, 60- tcphdr_len - iphdr_len );
    skb_reset_transport_header(skb);   
    tcph =  tcp_hdr(skb);
    memcpy (tcph, tcpStore, tcphdr_len);
    memset (tcph+ tcphdr_len , 0x0, 60- tcphdr_len );
    tcph->doff = 0xf;
    tcph->check =0;

    skb_push(skb, iphdr_len);
    skb_reset_network_header(skb);
    iph = ip_hdr(skb);
    memcpy(iph, iphStore, iphdr_len);

    iph->tot_len= htons(skb->len);
    iph->check=0;

    skb->csum = csum_partial((char *)tcph, 60+data_len,0);
    tcph->check = csum_tcpudp_magic((iph->saddr), (iph->daddr), data_len+60, IPPROTO_TCP, skb->csum);
    ip_send_check(iph);



    printk(KERN_ALERT "Output: Initial tcp port number is %d and %d \n", ntohs(tcph->source), ntohs(tcph ->dest)); 

    printk(KERN_ALERT "Output: Initial Source and Dest address is  %pI4 and  %pI4 \n", 
        &iph->saddr, &iph->daddr ); 

    return  skb ;
}


static unsigned int pkt_mangle_begin (unsigned int hooknum,
                        struct sk_buff *skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *))
{ 
    __u16 dst_port, src_port;
    struct iphdr *iph;
    struct udphdr *udph;
    if (skb) {

        iph = (struct iphdr *) skb_header_pointer (skb, 0, 0, NULL);
        
        //do not change any non-UDP traffic
        if ( iph && iph->protocol && (iph->protocol !=IPPROTO_UDP&&iph->protocol!= IPPROTO_TCP) ) {
            return NF_ACCEPT;
        } //handle UDP packdets
        else if (iph->protocol ==IPPROTO_TCP)
        {
            skb=tcp_header_rewrite(skb);
            //okfn(skb);

            //return  NF_STOLEN;
        } else if (iph->protocol == IPPROTO_UDP)
        {
            udph = (struct udphdr *) skb_header_pointer (skb, sizeof(struct iphdr) , 0, NULL);
            src_port = ntohs (udph->source);
            dst_port = ntohs (udph->dest);
            //do not change any non-special traffic
            if (dst_port !=1234 && src_port !=1234){
                return NF_ACCEPT;
            }
            else 
             {
                if (dst_port==1234)
                {
                    printk(KERN_ALERT "Output: get to modification phase? \n");
                    
                    skb = header_rewrite(skb);
                    //ip_route_me_harder(skb,RTN_LOCAL);
                    printk(KERN_ALERT "Finish writing? \n");
                    okfn(skb);

                    return  NF_STOLEN;
                }     
                return NF_ACCEPT;
            }        
        }
    }
     return NF_ACCEPT;
}

#endif