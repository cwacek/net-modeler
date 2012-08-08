#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <linux/hpet.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define USECREQ 1000000
#define LOOPS   100000

struct itimerval global_sched;

void event_handler (int signum)
{
  static unsigned long cnt = 0;
  static struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);

  cnt++;
  global_sched.it_interval.tv_sec += 1;
  global_sched.it_value.tv_sec = 1;
  fprintf(stderr, "itimer_alarm called. iteration: %lu  secs: %ld  nsecs: %ld interval_now: %ld \n",
          cnt, t.tv_sec , t.tv_nsec , global_sched.it_interval.tv_sec );
  
  setitimer (ITIMER_REAL, &global_sched, NULL);


  if (cnt > LOOPS){
    exit(0);
  }
}

/*enum hrtimer_restart fire_timer(struct hrtimer *hrt){*/

  /*uint64_t missed;*/
  /*missed = hrtimer_forward_now(hrt, ktime_set(0,500000));*/

  /*return HRTIMER_RESTART;*/
/*}*/

int main (int argc, char **argv)
{
  struct sigaction sa;

  /*struct hrtimer hrt;*/
  /*hrtimer_init(&hrt,CLOCK_MONOTONIC,HRTIMER_MODE_REL);*/
  /*hrt.function = fire_timer;*/

  /*hrtimer_start(&hrt,ktime_set(0,500000),HRTIMER_MODE_REL);*/




  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = &event_handler;
  sigaction (SIGALRM, &sa, NULL);
  global_sched.it_value.tv_sec = 1;
  global_sched.it_value.tv_usec = 0;
  global_sched.it_interval.tv_sec = 1;
  global_sched.it_interval.tv_usec = 0;
  setitimer (ITIMER_REAL, &global_sched, NULL);
  while (1);
}
