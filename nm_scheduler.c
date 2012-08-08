
#include <linux/module.h>
#include <linux/hrtimer.h>

#include "nm_log.h"
#include "nm_magic.h"
#include "net-modeler.h"

struct nm_global_sched nm_sched;


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

  hrtimer_init(&nm_sched.timer,CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
  nm_sched.callback = func;

  return 0;
}

/** The actual hrtimer callback has a specific structure. Handle it
 *  and then grab the containing structure and call the callback we
 *  want.
 **/
static enum hrtimer_restart __nm_callback(struct hrtimer *hrt)
{
  struct nm_global_sched *sched;
  ktime_t interval;

  if (hrtimer_active(hrt)){
    nm_log(NM_NOTICE,"Timer callback called, but timer was in active state.\n");
    return HRTIMER_NORESTART;
  }

  sched = container_of(hrt,struct nm_global_sched, timer);
  interval = sched->callback(sched);

  if (hrtimer_start(&sched->timer,interval,HRTIMER_MODE_ABS) < 0){
    nm_log(NM_NOTICE,"Failed to schedule timer\n");
    return HRTIMER_NORESTART;
  }
  return HRTIMER_NORESTART;
}


/** Cancel any running schedulers **/
void nm_cleanup_sched(void)
{
  hrtimer_cancel(&nm_sched.timer);
}

