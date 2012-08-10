#ifndef __KERN_NM_MODELER
#define __KERN_NM_MODELER
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/kfifo.h>
#include <net/netfilter/nf_queue.h>
#include "nm_log.h"

#define CALENDAR_SIZE (sizeof(struct nm_packet *))<<4
#define MSECS_TO_NSECS(x) ((x) * 1000000)
#define ptr_size sizeof(void *)

#define IPH_FMT "[id:%hu src:%pI4 dst:%pI4 proto:%u]"
#define IPH_FMT_DATA(iph) (iph)->id, &(iph)->saddr, &(iph)->daddr, (iph)->protocol

#define check_call(x) if ((x) < 0) nm_log(NM_WARN,"Call "#x" failed\n")

/** Injector **/
void nm_cleanup_injector(void);
int nm_init_injector(void);
int nm_inject(struct iphdr *pkt, uint32_t len);


/** Scheduler **/
struct nm_global_sched;
typedef ktime_t (*nm_cb_func)(struct nm_global_sched *);


/**
 * @curr_index  the current location in the packet buffer.
 * @timer       global hrtimer instance
 * @callback    the desired callback function. Will be passed the
 *              nm_global_sched struct, and must return the desired
 *              next timeout.
 **/
struct nm_global_sched {
  struct kfifo *calendar;
  uint32_t curr_index;
  struct hrtimer timer;
  ktime_t (*callback)(struct nm_global_sched *);
};

int nm_init_sched(nm_cb_func);
void nm_cleanup_sched(void);
int nm_enqueue(void *data,size_t len);
void nm_schedule(ktime_t time);


#endif /*__KERN_NM_MODELER*/
