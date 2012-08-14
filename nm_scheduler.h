#ifndef __KERN_NM_SCHEDULER
#define __KERN_NM_SCHEDULER

#include "nm_main.h"
#include "nm_structures.h"

/** The size of the calendar scheduling array */
#define CALENDAR_BUF_LEN 500
#define CALENDAR_MSECS  CALENDAR_BUF_LEN / UPDATE_INTERVAL_MSECS 
#define one_hop_schedulable(offset) (offset < CALENDAR_BUF_LEN) ? 1 : 0
#define scheduler_slot(scheduler, offset) (scheduler->calendar[(offset + scheduler->now_index) % CALENDAR_BUF_LEN])

#define SLOT_INIT(slot) \
    slot.n_packets = 0;\
    slot.head = slot.tail = 0 

/** 
 * @entries   then number of entries stored into this buffer slot.
 * @head      the front of the entry list;
 * @tail      the end of the entry list.
 **/
struct calendar_slot { 
  uint16_t n_packets;
  nm_packet_t *head;
  nm_packet_t *tail;
};


/**
 * @now_index  the current location in the packet buffer.
 * @timer       global hrtimer instance
 * @callback    the desired callback function. Will be passed the
 *              nm_global_sched struct, and must return the desired
 *              next timeout.
 **/
struct nm_global_sched {
  struct calendar_slot calendar[CALENDAR_BUF_LEN];
  uint32_t now_index;
  ktime_t last_update;
  struct hrtimer timer;
  ktime_t (*callback)(struct nm_global_sched *);
};


nm_packet_t * slot_pull(struct calendar_slot *slot);

#endif
