// net-modeler.c
// Author: Chris Wacek (cwacek@cs.georgetown.edu)
//
// A netfilter based network emulator
//


#include "nm_log.h"
#include "net-modeler.h"
#include "nm_magic.h"

static struct nf_hook_ops nfho;
struct iphdr *iph;

#define NM_IP_MASK 0x000000FF
#define NM_IP_NET 0x0000000A
#define IS_NM_IP(ip) (((ip) & NM_IP_MASK) == (NM_IP_NET))
#define NM_SEEN(iph) (iph->tos == TOS_MAGIC)

unsigned int hook_func(unsigned int hooknum, struct sk_buff *skb,
                       const struct net_device *in,
                       const struct net_device *out,
                       int (*okfn)(struct sk_buff *))
{
  if (!skb) { return NF_ACCEPT; }
  iph = (struct iphdr *) skb_network_header(skb);

  if (! IS_NM_IP(iph->daddr) ){
    nm_log(NM_INFO,"Ignored packet destined for %pI4\n",&iph->daddr);
    nm_log(NM_DEBUG,"NM_IP_NET: %X, MASK: %X, IP & MASK: %X\n",NM_IP_NET,NM_IP_MASK,iph->daddr);
    return NF_ACCEPT;
  }
  if (NM_SEEN(iph)){
    nm_log(NM_DEBUG,"Accepting previously seen packet. [id: %u]\n",iph->id);
    return NF_ACCEPT;
  }


  nm_inject(iph,skb_tail_pointer(skb)- ((unsigned char *)iph));

  return NF_STOLEN;
}

ktime_t insert_delay(struct nm_global_sched * s){

  return ktime_set(0,0);
}


int init_module(void)
{
  nfho.hook = hook_func;
  nfho.hooknum = NF_INET_PRE_ROUTING | NF_INET_LOCAL_OUT;
  nfho.pf = PF_INET;
  nfho.priority = NF_IP_PRI_FIRST;

  nf_register_hook(&nfho);

  /* Initialize the global scheduler */
  nm_init_sched(insert_delay);

  /* Initialize the packet reinjection stuff */
  if (nm_init_injector() < 0){
    nm_log(NM_WARN,"Failed to initialize injector\n");
    cleanup_module();
    return -1;
  }

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
  nm_cleanup_sched();
  nm_cleanup_injector();
  nf_unregister_hook(&nfho);
  printk(KERN_INFO "net-modeler cleaning up\n");
}


