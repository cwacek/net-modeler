// net-modeler.c
// Author: Chris Wacek (cwacek@cs.georgetown.edu)
//
// A netfilter based network emulator
//

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>

static struct nf_hook_ops nfho;

unsigned int hook_func(unsigned int hooknum, struct sk_buff *skb,
                       const struct net_device *in,
                       const struct net_device *out,
                       int (*okfn)(struct sk_buff *))
{
  printk(KERN_INFO "packet dropped\n");
  return NF_ACCEPT;
}


int init_module(void)
{
  nfho.hook = hook_func;
  nfho.hooknum = NF_INET_PRE_ROUTING;
  nfho.pf = PF_INET;
  nfho.priority = NF_IP_PRI_FIRST;

  nf_register_hook(&nfho);
  printk(KERN_INFO "net-modeler initiialized "
                   #include "version.i"
                    "\n");
  return 0;
}

void cleanup_module(void)
{
  printk(KERN_INFO "net-modeler cleaning up\n");
}


