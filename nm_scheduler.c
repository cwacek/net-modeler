#include <linux/module.h>
#include <linux/hrtimer.h>

#include "nm_log.h"
#include "nm_structures.h"
#include "nm_magic.h"
#include "nm_main.h"

struct nm_global_sched nm_sched;
static enum hrtimer_restart __nm_callback(struct hrtimer *hrt);
static DEFINE_SPINLOCK(nm_calendar_lock);
static uint8_t shutdown_requested = 0;


#if NM_LOG_LEVEL & NM_DEBUG_ID
#define spin_lock_irq_debug(lock, flag) \
  do { \
    nm_debug(LD_TRACE,"Acquiring lock in %s\n",__func__); \
    spin_lock_irqsave(lock,flag); \
  } while(0)
#define spin_unlock_irq_debug(lock, flag) \
  do { \
    nm_debug(LD_TRACE,"Releasing lock in %s\n",__func__); \
    spin_unlock_irqrestore(lock,flag); \
  } while(0)
#else
#define spin_lock_irq_debug(lock,flag) spin_lock_irqsave(lock,flag)
#define spin_unlock_irq_debug(lock,flag) spin_unlock_irqrestore(lock,flag)
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
  spin_lock_irqsave(&nm_calendar_lock,spin_flags);
  for (i = 0; i < CALENDAR_BUF_LEN; i++){
    SLOT_INIT(nm_sched.calendar[i]);
  }
  spin_unlock_irqrestore(&nm_calendar_lock,spin_flags);

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
  if (shutdown_requested)
    return HRTIMER_NORESTART;

  /*if (hrtimer_in(hrt)){*/
    /*nm_log(NM_NOTICE,"Timer callback called, but timer was in active state.\n");*/
    /*return HRTIMER_NORESTART;*/
  /*}*/

  interval = nm_sched.callback(&nm_sched);
  if (unlikely(ktime_to_ns(interval) == 0))
    return HRTIMER_NORESTART;

  if (unlikely(hrtimer_start(&nm_sched.timer,interval,HRTIMER_MODE_ABS) < 0)){
    nm_notice(LD_ERROR,"Failed to schedule timer\n");
    return HRTIMER_NORESTART;
  }
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
nm_packet_t * slot_pull(struct calendar_slot *slot)
{
  nm_packet_t *pulled;

  pulled = slot->head;
  if (pulled)
    slot->head = pulled->next;
  
  slot->n_packets--;
  return pulled;
}

/** Enqueue a packet into the calendar at an offset from now **/
int nm_enqueue(nm_packet_t *data,uint16_t offset)
{
  if (unlikely(!one_hop_schedulable(offset))){
    offset = CALENDAR_BUF_LEN;
    data->hop_progress += CALENDAR_BUF_LEN;
    data->flags |= NM_FLAG_HOP_INCOMPLETE; 
  } else {
    data->flags  = data->flags & ~NM_FLAG_HOP_INCOMPLETE;
  }

  spin_lock(&nm_calendar_lock);
  slot_add_packet(&scheduler_slot((&nm_sched),offset), data);
  spin_unlock(&nm_calendar_lock);
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
