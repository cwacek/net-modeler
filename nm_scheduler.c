
#include <linux/module.h>
#include <linux/hrtimer.h>

#include "nm_log.h"
#include "nm_structures.h"
#include "nm_magic.h"
#include "nm_main.h"

struct nm_global_sched nm_sched;
static enum hrtimer_restart __nm_callback(struct hrtimer *hrt);

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
  log_func_entry;

  hrtimer_init(&nm_sched.timer,CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  nm_sched.timer.function = __nm_callback;
  nm_sched.callback = func;

  /** Zero initialize our calendar slots or all hell will break loose */
  for (i = 0; i < CALENDAR_BUF_LEN; i++){
    SLOT_INIT(nm_sched.calendar[i]);
  }

  nm_log(NM_DEBUG,"Initialized scheduler. [User callback: %p, system callback: %p]\n",
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

  /*if (hrtimer_in(hrt)){*/
    /*nm_log(NM_NOTICE,"Timer callback called, but timer was in active state.\n");*/
    /*return HRTIMER_NORESTART;*/
  /*}*/

  interval = nm_sched.callback(&nm_sched);
  if (ktime_to_ns(interval) == 0)
    return HRTIMER_NORESTART;

  if (hrtimer_start(&nm_sched.timer,ktime_add(interval,hrt->base->get_time()),HRTIMER_MODE_ABS) < 0){
    nm_log(NM_NOTICE,"Failed to schedule timer\n");
    return HRTIMER_NORESTART;
  }
  return HRTIMER_NORESTART;
}

/** Add a packet to the slot */
static void slot_add_packet(struct calendar_slot *slot, nm_packet_t *p)
{
  if (slot->n_packets == 0)
  {
    slot->head = p;
    slot->tail = p;
  } else {
    slot->tail->next = p;
    p->prev = slot->tail;
    slot->tail = p;
  }
  slot->n_packets++;
}

/** Pull a nm_packet_t from the calendar slot, or return
 * zero if none exist */
nm_packet_t * slot_pull(struct calendar_slot *slot)
{
  nm_packet_t *pulled;

  if (slot->n_packets == 0){
    pulled = 0;
  } else {
    pulled = slot->tail;
    slot->tail = (pulled->prev != 0) ? pulled->prev : 0;
    pulled->prev = pulled->next = 0;
    slot->tail->next = 0;
    if (--slot->n_packets == 0)
      slot->head = 0;
  }

  return pulled;
}

/** Enqueue a packet into the calendar at an offset from now **/
int nm_enqueue(nm_packet_t *data,uint16_t offset)
{
  if (!one_hop_schedulable(offset)){
    offset = CALENDAR_BUF_LEN;
    data->hop_progress += CALENDAR_BUF_LEN;
    data->flags |= NM_FLAG_HOP_INCOMPLETE; 
  } else {
    data->flags  = data->flags & ~NM_FLAG_HOP_INCOMPLETE;
  }

  slot_add_packet(&scheduler_slot((&nm_sched),offset), data);
  return 0;
}

/** Cancels any running schedulers if they are for a time
 *  <b>after</b> this one, and reschedule for this time.
 */
void nm_schedule(ktime_t time){
  log_func_entry;
  
  if (hrtimer_start(&nm_sched.timer,time,HRTIMER_MODE_REL) < 0){
    nm_log(NM_WARN,"Failed to schedule timer for %lldns from now\n",
                ktime_to_ns(time));
  }
}

/** Cancel any running schedulers **/
void nm_cleanup_sched(void)
{
  hrtimer_cancel(&nm_sched.timer);
}

MODULE_LICENSE("GPL");
