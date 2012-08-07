// net-modeler.c
// Author: Chris Wacek (cwacek@cs.georgetown.edu)
//
// A netfilter based network emulator
//

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>

#include "log.h"

static struct nf_hook_ops nfho;
struct iphdr *iph;

#define NM_IP_MASK 0x000000FF
#define NM_IP_NET 0x0000000A
#define IS_NM_IP(ip) (((ip) & NM_IP_MASK) == (NM_IP_NET))

unsigned int hook_func(unsigned int hooknum, struct sk_buff *skb,
                       const struct net_device *in,
                       const struct net_device *out,
                       int (*okfn)(struct sk_buff *))
{
  if (!skb) { return NF_ACCEPT; }
  iph = (struct iphdr *) skb_network_header(skb);

  if (! IS_NM_IP(iph->daddr)){
    nm_log(NM_INFO,"Ignored packet destined for %pI4\n",&iph->daddr);
    nm_log(NM_DEBUG,"NM_IP_NET: %X, MASK: %X, IP & MASK: %X\n",NM_IP_NET,NM_IP_MASK,iph->daddr);
    return NF_ACCEPT;
  }

  switch (iph->protocol){
    case 17:
      printk(KERN_INFO "dropped UDP packet [%pI4 => %pI4] %u\n",&iph->saddr,&iph->daddr,iph->saddr);
      return NF_DROP;
      break;
    case 6:
      printk(KERN_INFO "accepted TCP packet [%pI4 => %pI4] \n",&iph->saddr,&iph->daddr);
      return NF_ACCEPT;
      break;
    }

  printk(KERN_INFO "got unrecognized protcol packet: %d \n",iph->protocol);
  return NF_ACCEPT;
}


int init_module(void)
{
  nfho.hook = hook_func;
  nfho.hooknum = NF_INET_PRE_ROUTING | NF_INET_LOCAL_OUT;
  nfho.pf = PF_INET;
  nfho.priority = NF_IP_PRI_FIRST;

  nf_register_hook(&nfho);
  printk(KERN_INFO "net-modeler initiialized "
                   #include "version.i"
                    "\n");

  printk(KERN_INFO "MASK: %X \n NET: %X \n ",
          NM_IP_MASK, 
          NM_IP_NET);
  return 0;
}

void cleanup_module(void)
{

  nf_unregister_hook(&nfho);
  printk(KERN_INFO "net-modeler cleaning up\n");
}


