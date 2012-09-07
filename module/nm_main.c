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
#include "nm_proc.h"

static struct nf_hook_ops nfho;
struct iphdr *iph;

#define IS_NM_IP(ip) (((ip) & NM_IP_MASK) == (NM_IP_NET))
#define NM_SEEN(iph) (iph->tos == TOS_MAGIC)

#define queue_entry_iph(x) ((struct iphdr *)skb_network_header((x)->skb))

unsigned int hook_func(unsigned int hooknum, struct sk_buff *skb,
                       const struct net_device *in,
                       const struct net_device *out,
                       int (*okfn)(struct sk_buff *))
{

  if (!skb) { return NF_ACCEPT; }
  iph = (struct iphdr *) skb_network_header(skb);

  if (! IS_NM_IP(iph->daddr) ){
    return NF_ACCEPT;
  }
  if (NM_SEEN(iph)){
    nm_debug(LD_GENERAL,"Accepting previously seen packet. "IPH_FMT"\n",
                    IPH_FMT_DATA(iph));
    return NF_ACCEPT;
  }

  nm_debug(LD_TIMING,"Received packet at %lldns\n",ktime_to_ns(nm_get_time()));
  if (!nm_model._initialized){
    nm_notice(LD_GENERAL,"Rejecting filtered packet because no model is initialized\n");
    return NF_DROP;
  }

  return NF_QUEUE;
}

ktime_t insert_delay(struct nm_global_sched * s){
  log_func_entry;

  return ktime_set(0,0);
}

ktime_t update(struct nm_global_sched *sch)
{
  struct nm_packet *pkt;
  int missed_intervals, i, dequeued_ctr = 0;
  int16_t prog_past_tail;
  unsigned long lock_flags;
  nm_hop_t *hop;
  ktime_t now = sch->timer.base->get_time();
  log_func_entry;
  lock_flags = 0;
  
  /** Make sure we process any intervals we missed **/
  missed_intervals = ktime_to_ns(ktime_sub(now,sch->last_update)) 
                        / MSECS_TO_NSECS(UPDATE_INTERVAL_MSECS);
  nm_debug(LD_TIMING, "Callback fired. Missed %d intervals \n",missed_intervals);

  for (i = 1; i <= missed_intervals; i++)
  {
    /** We dequeue everything in the slot one at at time **/
    while (1)
    {       
      nm_schedule_lock_acquire(lock_flags);
      pkt = slot_pull(&scheduler_slot(sch,i));
      nm_schedule_lock_release(lock_flags);

      if (likely(!pkt))
        break;

      /* Figure out the tailexit for the hop */
      if (!pkt->path)
        break;
      hop = &nm_model._hoptable[pkt->path->hops[pkt->path_idx]];
      nm_debug(LD_SCHEDULE, "Dequeued packet on hop %u. [tailexit: %u]",
                pkt->path->hops[pkt->path_idx], hop->tailexit);
      
      /** If this was a hop in progress, we want to enqueue rather than
       * reinject **/
      if (unlikely(pkt->flags & NM_FLAG_HOP_INCOMPLETE)) 
      {
        /* Adjust hop exit by the amount we've traveled. */
        /* If hop_progress > tailwait, that means that we're now
         * addressing the portion of the delay that affects the tailwait. 
         * This is likely to occur if the packet payload is simply bigger than
         * the delay window. */
        prog_past_tail = pkt->hop_progress - pkt->hop_tailwait;
        if (prog_past_tail > 0)
        {
          /* At most, we can only remove as much as we were scheduled for */
          hop->tailexit -= (prog_past_tail > pkt->scheduled_amt) ? 
                                pkt->scheduled_amt : prog_past_tail;
        }

        nm_debug(LD_SCHEDULE, "Set tailexit for partial hop %u "
                              "[scheduled: %u total: %u] to %u. [index: %llu]",
                  pkt->path->hops[pkt->path_idx],
                  pkt->scheduled_amt,
                  pkt->hop_progress,
                  hop->tailexit,scheduler_index());

        nm_enqueue(pkt, total_pkt_cost(pkt) - pkt->hop_progress);
      } 
      else 
      {
        hop->tailexit -= pkt->scheduled_amt;
        nm_debug(LD_SCHEDULE, "Set tailexit for partial hop %u "
                              "[scheduled: %u total: %u] to %u. [index: %llu]",
                  pkt->path->hops[pkt->path_idx],
                  pkt->scheduled_amt,
                  pkt->hop_progress,
                  hop->tailexit,scheduler_index());

        /* If we have reached the last hop, then we should reinject.
         * Otherwise we should enqueue for the next hop */
        if (++(pkt->path_idx) < pkt->path->len){
          pkt->hop_progress =  pkt->scheduled_amt = pkt->hop_tailwait = pkt->hop_cost = 0;
          if (unlikely((nm_enqueue(pkt,calc_delay(pkt))) < 0)){
            nm_warn(LD_ERROR,"CRITICAL ERROR: Failed to reenqueue packet "
                             "for hop %u\n",pkt->path_idx);
          }
          nm_debug(LD_SCHEDULE,"Scheduling packet on next hop %u\n",
                                pkt->path_idx);
        } else {
          dequeued_ctr++;
          nm_debug(LD_GENERAL,"Delivering packet: "IPH_FMT"\n",
                      IPH_FMT_DATA(queue_entry_iph(pkt->data)));
          nf_reinject(pkt->data,NF_ACCEPT);
          nm_packet_free(pkt);
        }
      }
    }
  }

  atomic64_add(missed_intervals,&sch->now_index);
  sch->last_update = now;

  /*nm_debug(LD_TIMING, "Callback finished in %lldns\n",ktime_to_ns(ktime_sub(sch->timer.base->get_time(),now)));*/
  /*nm_info(LD_GENERAL, "Dequeued %u packets from %u intervals \n",dequeued_ctr,missed_intervals);*/

  return ktime_add_ns(now,MSECS_TO_NSECS(UPDATE_INTERVAL_MSECS));
}

static int _nm_queue_cb(struct nf_queue_entry *entry, unsigned int queuenum)
{
  nm_packet_t *pkt; 
  int err;
  uint64_t index;

  index = scheduler_index();

  nm_debug(LD_GENERAL,"Enqueuing new packet. "IPH_FMT"\n",
                    IPH_FMT_DATA(queue_entry_iph(entry)));

  pkt = nm_packet_init(entry,
                       queue_entry_iph(entry)->saddr, 
                       queue_entry_iph(entry)->daddr);
  
  
  if (unlikely(!pkt))
    return -ENOMEM;
  
  if (unlikely((err = nm_enqueue(pkt,calc_delay(pkt) - (scheduler_index() - index))) < 0))
    return err;

  nm_debug(LD_TIMING,"Completed packet enqueue at %lldns [index: %llu]\n",ktime_to_ns(nm_get_time()),index);

  return 0;
}

static const struct nf_queue_handler _queueh = {
      .name   = "net-modeler",
      .outfn  = _nm_queue_cb,
};

static int __init nm_init(void)
{
  log_func_entry;
  nm_notice(LD_GENERAL,"Starting up");

  nm_structures_init();

  check_call(nf_register_queue_handler(PF_INET,&_queueh));

  nfho.hook = hook_func;
  nfho.hooknum =  NF_INET_LOCAL_OUT;
  nfho.pf = PF_INET;
  nfho.priority = NF_IP_PRI_FIRST;

  nf_register_hook(&nfho);

  check_call(initialize_proc_interface());

  /* Initialize the global scheduler */
  if (nm_init_sched(update) < 0){
     nm_warn(LD_ERROR,"Failed to initialize scheduler\n");
    cleanup_module();
    return -1;
  }

  nm_notice(LD_GENERAL,"net-modeler initialized ("VERSION")\n");
  nm_notice(LD_GENERAL,"logging at: %s\n",nm_loglevel_string(NM_LOG_LEVEL) );

  nm_schedule(ktime_set(2,0));
  
  return 0;
}

static void nm_exit(void)
{
  nm_notice(LD_GENERAL,"Cleaning up\n");
  nf_unregister_hook(&nfho);
  nf_unregister_queue_handler(PF_INET,&_queueh);
  nm_cleanup_sched();
  nm_structures_release();
  cleanup_proc_interface();
  nm_notice(LD_GENERAL,"Unloading\n");
}

module_init(nm_init);
module_exit(nm_exit);

MODULE_AUTHOR(AUTHOR);
MODULE_LICENSE("GPL");
