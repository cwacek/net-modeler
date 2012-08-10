// net-modeler.c
// Author: Chris Wacek (cwacek@cs.georgetown.edu)
//
// A netfilter based network emulator
//
#define AUTHOR "Chris Wacek <cwacek@cs.georgetown.edu>"

#include "version.i"
#include "nm_main.h"
#include "nm_magic.h"
#include "nm_structures.h"

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
  nm_packet_t *pkt; 

  if (!skb) { return NF_ACCEPT; }
  iph = (struct iphdr *) skb_network_header(skb);

  if (! IS_NM_IP(iph->daddr) ){
    return NF_ACCEPT;
  }
  if (NM_SEEN(iph)){
    nm_log(NM_DEBUG,"Accepting previously seen packet. "IPH_FMT"\n",
                    IPH_FMT_DATA(iph));
    return NF_ACCEPT;
  }
    
  nm_log(NM_DEBUG,"Enqueuing new packet. "IPH_FMT"\n",
                    IPH_FMT_DATA(iph));

  pkt = nm_packet_init(iph,
                        skb_tail_pointer(skb)- ((unsigned char *)iph),
                        iph->saddr, iph->daddr);

  nm_enqueue(&pkt,sizeof(nm_packet_t *));

  return NF_STOLEN;
}

ktime_t insert_delay(struct nm_global_sched * s){
  log_func_entry;

  return ktime_set(0,0);
}

ktime_t repeat_1ms(struct nm_global_sched *sch)
{
  struct nm_packet *pkt;
  int res;
  log_func_entry;
  
  nm_log(NM_DEBUG, "Callback fired at %lld\n",ktime_to_ns(sch->timer.base->get_time()));

  /* If we have nothing to dequeue, bail out */
  if (kfifo_is_empty(sch->calendar))
    return ktime_set(0, MSECS_TO_NSECS(50));  

  res = kfifo_out(sch->calendar,&pkt,sizeof(struct nm_packet *));
  if (res != sizeof(struct nm_packet *)){
    nm_log(NM_WARN,"Tried to dequeue packet pointer, but "
            "didn't have enough data. Wanted %zu. Had %u",
            sizeof(struct nm_packet *),
            res);
  } else {
    nm_log(NM_DEBUG,"dequeued packet: "IPH_FMT"\n",
                    IPH_FMT_DATA((struct iphdr *)pkt->data));

    res = nm_inject(pkt->data,pkt->len);
    if (res < 0){
      nm_log(NM_WARN,"Failed to inject packet: %d",res);
    }
    nm_packet_free(pkt);
  }

  return ktime_set(0,MSECS_TO_NSECS(50));
}

static int __init nm_init(void)
{
  log_func_entry;
  printk(KERN_INFO "Starting up");

  nm_structures_init();

  nfho.hook = hook_func;
  nfho.hooknum =  NF_INET_LOCAL_OUT;
  nfho.pf = PF_INET;
  nfho.priority = NF_IP_PRI_FIRST;

  nf_register_hook(&nfho);

  /* Initialize the global scheduler */
  if (nm_init_sched(repeat_1ms) < 0){
    nm_log(NM_WARN, "Failed to initialize scheduler\n");
    cleanup_module();
    return -1;
  }

  /* Initialize the packet reinjection stuff */
  if (nm_init_injector() < 0){
    nm_log(NM_WARN,"Failed to initialize injector\n");
    cleanup_module();
    return -1;
  }

  nm_log(NM_NOTICE,"net-modeler initiialized ("VERSION")\n");

  nm_schedule(ktime_set(5,0));
  
  return 0;
}

static void nm_exit(void)
{
  nm_cleanup_sched();
  nm_cleanup_injector();
  nf_unregister_hook(&nfho);
  nm_structures_release();
  printk(KERN_INFO "net-modeler cleaning up\n");
}

module_init(nm_init);
module_exit(nm_exit);

MODULE_AUTHOR(AUTHOR);
MODULE_LICENSE("GPL");
