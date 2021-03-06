#include <linux/module.h>
#include <linux/hrtimer.h>

#include "nm_log.h"
#include "nm_structures.h"
#include "nm_magic.h"
#include "nm_main.h"

struct nm_global_sched nm_sched;
static enum hrtimer_restart __nm_callback(struct hrtimer *hrt);
static DEFINE_SPINLOCK(nm_calendar_lock);
static DEFINE_SPINLOCK(callback_func_lock);
static uint8_t shutdown_requested = 0;
static DEFINE_SEMAPHORE(callback_in_progress);

#if NM_LOG_LEVEL & NM_DEBUG_ID && 0
#define spin_lock_irq_debug(lock, flag) \
  do { \
    nm_debug(LD_TRACE,"Acquiring lock in %s\n",__func__); \
    spin_lock_irqsave((lock),(flag)); \
    local_bh_disable(); \
  } while(0 == 1)
#define spin_unlock_irq_debug(lock, flag) \
  do { \
    nm_debug(LD_TRACE,"Releasing lock in %s\n",__func__); \
    spin_unlock_irqrestore((lock),(flag)); \
    local_bh_enable(); \
  } while(0)
#else
#define spin_lock_irq_debug(lock,flag) \
  spin_lock_irqsave((lock),(flag)); \
  local_bh_disable();
#define spin_unlock_irq_debug(lock,flag) \
  spin_unlock_irqrestore((lock),(flag)); \
  local_bh_enable();
#endif

void nm_schedule_lock_release(unsigned long flags)
{
  spin_unlock_irq_debug(&nm_calendar_lock,flags);
}
void nm_schedule_lock_acquire(unsigned long flags)
{
  spin_lock_irq_debug(&nm_calendar_lock,flags);
}

inline ktime_t nm_get_time(void){
  return nm_sched.timer.base->get_time();
}


/** Initialize the global scheduler with the callback function 'func'.
 *
 *  The callback function should do whatever it wants to do, then
 *  MUST return the absolute ktime_t for when it wants to be rescheduled.
 *  The function above this one will take care of scheduling the actual
 *  timer.
 *
 *  If the callback function returns a zeroed out ktime_t, nothing will
 *  be scheduled.
 **/
int nm_init_sched(nm_cb_func func)
{
  int i;
  unsigned long spin_flags;
  log_func_entry;

  hrtimer_init(&nm_sched.timer,CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  nm_sched.timer.function = __nm_callback;
  nm_sched.callback = func;
  atomic64_set(&nm_sched.now_index,0);

  /** Zero initialize our calendar slots or all hell will break loose */
  spin_lock_irq_debug(&nm_calendar_lock,spin_flags);
  for (i = 0; i < CALENDAR_BUF_LEN; i++){
    SLOT_INIT(nm_sched.calendar[i]);
  }
  spin_unlock_irq_debug(&nm_calendar_lock,spin_flags);

  nm_debug(LD_GENERAL,"Initialized scheduler. [User callback: %p, system callback: %p]\n",
              nm_sched.callback, nm_sched.timer.function);
  return 0;
}

/** The actual hrtimer callback has a specific structure. Handle it
 *  and then grab the containing structure and call the callback we
 *  want.
 **/
static enum hrtimer_restart __nm_callback(struct hrtimer *hrt)
{
  ktime_t interval;
  log_func_entry;

  spin_lock(&callback_func_lock);

  if (shutdown_requested)
    goto release;

  interval = nm_sched.callback(&nm_sched);
  if (unlikely(ktime_to_ns(interval) == 0))
    goto release;

  if (unlikely(hrtimer_start(&nm_sched.timer,interval,HRTIMER_MODE_ABS) < 0)){
    nm_notice(LD_ERROR,"Failed to schedule timer\n");
    goto release;
  }

release:
  spin_unlock(&callback_func_lock);
  return HRTIMER_NORESTART;
}

/** Add a packet to the slot */
static inline void slot_add_packet(struct calendar_slot *slot, nm_packet_t *p)
{
  if (slot->head)
    p->next = slot->head;

  slot->head = p;
  slot->n_packets++;
}

/** Pull a nm_packet_t from the calendar slot, or return
 * zero if none exist */
inline nm_packet_t * slot_pull(struct calendar_slot *slot)
{
  nm_packet_t *pulled;

  pulled = slot->head;
  if (pulled)
    slot->head = pulled->next;

  slot->n_packets--;
  return pulled;
}

/** Calculate how much to delay a packet as it
 *  transits a hop. Here we also apply flags for whether or
 *  not it's going to take multiple hops.
 *
 *  Return -1 if we can't figure it out for some reason.**/
inline int32_t calc_delay(nm_packet_t *pkt,nm_hop_t *hop)
{
  uint32_t delay;
  delay = 0;
  /* The delay should be the latency + how long it
   * will take the packet to cross the link based on
   * bw */

  if (unlikely( !pkt || !pkt->path || pkt->path->valid != TOS_MAGIC)){
    nm_warn(LD_ERROR,"Can't route packet, "
                      "it has no designated path\n");
    return -1;
  }

  if (hop->bw_limit != 0)
  {
    /* We need to do bandwidth */
    /* packet size is given by pkt->data->skb->len */
    delay += pkt->data->skb->len / hop->bw_limit;
  }
  delay += hop->delay_ms;

  pkt->hop_cost = delay;

  delay += hop->tailexit;
  pkt->hop_tailwait = hop->tailexit;

  nm_debug(LD_SCHEDULE, "Calculated delay for packet (size: %u) on hop %u as %u ms. "
                        "[bw: %u, latency: %u, curr_tailexit: %u] \n",
                          pkt->data->skb->len, pkt->path->hops[pkt->path_idx],
                          delay, hop->bw_limit, hop->delay_ms, hop->tailexit );

  hop->tailexit = delay;

  return delay;
}

/** Enqueue a packet into the calendar at an offset from now **/
int nm_enqueue(nm_packet_t *data,char flags,int adjust)
{
  uint16_t offset;
  nm_hop_t *hop;
  unsigned long irqflags;
  offset = 0;

  if (flags == ENQUEUE_HOP_CURRENT)
  {
    offset = total_pkt_cost(data) - data->hop_progress;
    offset -= adjust;
  }
  else if (flags == ENQUEUE_HOP_NEW)
  {
    hop = &nm_model._hoptable[data->path->hops[data->path_idx]];
    if ( (++(hop->qfill)) >= hop->qlen)
    {
      nm_debug(LD_GENERAL,"Hop %u queue full. Cannot enqueue.", data->path_idx);
      hop->qfill--;
      return -1;
    }
    offset = calc_delay(data,hop);
    if (offset < 0)
      return -1;
    /* If we have to adjust because we missed a time interval, subtract it
     * from the offset, but add it to the hop_progress or we'll end off on
     * our math.
     */
    offset -= adjust;
  } else
  {
    nm_warn(LD_ERROR,"Bad flag to nm_enqueue");
    return -1;
  }

  if (unlikely(!one_hop_schedulable(offset))){
    offset = CALENDAR_BUF_LEN -1;
    data->flags |= NM_FLAG_HOP_INCOMPLETE;
    nm_debug(LD_SCHEDULE, "Scheduling partial packet with offset %u."
                          " Total cost: %u. Progress: %u [index: %llu]",
                          offset, data->hop_cost,data->hop_progress,scheduler_index());
  } else {
    data->flags  = data->flags & ~NM_FLAG_HOP_INCOMPLETE;
    nm_debug(LD_SCHEDULE, "Scheduling complete packet with offset %u. "
                          "Total cost: %u. Progress: %u [index: %llu]",
                          offset, data->hop_cost,data->hop_progress,scheduler_index());
  }

  /* We set scheduled amount to offset + adjust, because while we didn't actually
   * schedule it, it's only because we skipped it. The time was accounted for. */
  data->scheduled_amt = offset + adjust;
  data->hop_progress += offset + adjust;

  spin_lock_irq_debug(&nm_calendar_lock, irqflags);
  slot_add_packet(&scheduler_slot((&nm_sched),offset), data);
  spin_unlock_irq_debug(&nm_calendar_lock, irqflags);
  return 0;
}

/** Cancels any running schedulers if they are for a time
 *  <b>after</b> this one, and reschedule for this time.
 */
void nm_schedule(ktime_t time){
  log_func_entry;

  if (unlikely(hrtimer_start(&nm_sched.timer,time,HRTIMER_MODE_REL) < 0)){
    nm_warn(LD_ERROR,"Failed to schedule timer for %lldns from now\n",
                ktime_to_ns(time));
  }
}

static inline void __slot_free(struct calendar_slot * slot)
{
  nm_packet_t * tofree;
  while ((tofree = slot_pull(slot)))
  {
    nm_free(NM_PKT_ALLOC,tofree);
  }
}

/** Cancel any running schedulers **/
void nm_cleanup_sched(void)
{
  int i;
  unsigned long spin_flags;

  shutdown_requested = 1;
  hrtimer_cancel(&nm_sched.timer);
  nm_notice(LD_GENERAL,"Canceled timer\n");

  spin_lock_irq_debug(&nm_calendar_lock,spin_flags);
  for (i = 0; i < CALENDAR_BUF_LEN; i++){
    __slot_free(&nm_sched.calendar[i]);
  }
  spin_unlock_irq_debug(&nm_calendar_lock,spin_flags);
  nm_notice(LD_GENERAL,"Freed slots\n");
}

inline uint64_t scheduler_index(void){
  return atomic64_read(&nm_sched.now_index);
}

MODULE_LICENSE("GPL");
