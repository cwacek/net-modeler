#ifndef __KERN_NM_MODELER
#define __KERN_NM_MODELER
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/atomic.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/kfifo.h>
#include <net/netfilter/nf_queue.h>
#include "nm_log.h"
#include "nm_structures.h"

/** The number of milliseconds the calendar tracks forward **/
#define UPDATE_INTERVAL_MSECS 1

#include "nm_scheduler.h"

#define MSECS_TO_NSECS(x) ((x) * 1000000)
#define ptr_size sizeof(void *)

#define IPH_FMT "[id:%hu src:%pI4 dst:%pI4 proto:%u ttl:%u]"
#define IPH_FMT_DATA(iph) (iph)->id, &(iph)->saddr, &(iph)->daddr, (iph)->protocol, (iph)->ttl

#define check_call(x) if ((x) < 0) nm_warn(LD_TRACE,"Call "#x" failed\n")
#define NM_IP_MASK 0x000000FF
#define NM_IP_NET 0x0000000A
#define NM_IP_BASE 0x0A000000

/* Translate an IP address into an array index */
#define ip_int_idx(ip) ip - NM_IP_BASE


/** Injector **/
void nm_cleanup_injector(void);
int nm_init_injector(void);
int nm_inject(struct iphdr *pkt, uint32_t len);


/** Scheduler **/
struct nm_global_sched;
typedef ktime_t (*nm_cb_func)(struct nm_global_sched *);

int nm_init_sched(nm_cb_func);
void nm_cleanup_sched(void);
int nm_enqueue(nm_packet_t *data, uint16_t offset);
void nm_schedule(ktime_t time);


#endif /*__KERN_NM_MODELER*/
