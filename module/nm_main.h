#ifndef __KERN_NM_MODELER
#define __KERN_NM_MODELER
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/atomic.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/semaphore.h>
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

/** This enqueue request is for a new hop **/
#define ENQUEUE_HOP_NEW (0x1 << 0)
/** This enqueue request is for the same hop we were on before **/
#define ENQUEUE_HOP_CURRENT (0x1 << 1)

#define check_call(x) if ((x) < 0) nm_warn(LD_TRACE,"Call "#x" failed\n")
#define NM_IP_MASK 0x000000FF
#define NM_IP_NET 0x0000000A

/* Base is the same as the net, plus one, so that we index 10.0.0.1 into 0 */
#define NM_IP_BASE 0x0A000001

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
int nm_enqueue(nm_packet_t *data, char flags, int adjust);
void nm_schedule(ktime_t time);


#endif /*__KERN_NM_MODELER*/
