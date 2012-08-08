#include <sys/time.h>
#include <linux/hpet.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

static uint16_t hpet_sigio_count;
static uint64_t secs;

static void
hpet_alarm(int val)
{
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);

  if (!secs) secs = t.tv_sec;

  fprintf(stderr, "hpet_alarm called. iteration: %2d  secs: %ld  nsecs: %ld \n",
      hpet_sigio_count, t.tv_sec , t.tv_nsec  );

  hpet_sigio_count++;
}

int
main (int argc, const char **argv)
{
  struct sigaction old,new;
  struct hpet_info info;
  int frequency, iterations, retval = 0, fd, r, i, value;

  if (argc != 3){
    fprintf(stderr, "Usage: %s frequency(1-64) iterations(10-99)\n", argv[0]);
    return -1;
  }

  frequency = atoi(argv[1]);
  iterations = atoi(argv[2]);

  if (frequency > 64 || frequency < 1 ) {
    fprintf(stderr, "ERROR: Invalid value for frequency\n");
    return -1;
  }

  if (iterations < 10 || iterations > 99 ) {
    fprintf(stderr, "ERROR: Invalid value for iterations\n");
    return -1;
  }

  hpet_sigio_count = 0;

  sigemptyset(&new.sa_mask);
  new.sa_flags = 0;
  new.sa_handler = hpet_alarm;

  sigaction(SIGIO, NULL, &old);
  sigaction(SIGIO, &new, NULL);

  fd = open("/dev/hpet", O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Error: Failed to open /dev/hpet\n");
    return -1;
  }

  if ((fcntl(fd,F_SETOWN, getpid()) == 1) ||
      ((value = fcntl(fd,F_GETFL)) == 1) || 
      (fcntl(fd, F_SETFL, value |O_ASYNC) == 1)){
    perror("fnctl");
    fprintf(stderr, "Error fcntl failed\n");
    retval = 1;
    goto fail;
  }

  if (ioctl(fd,HPET_IRQFREQ, frequency) < 0) {
    fprintf(stderr, "Error: Could not set /dev/hpet to have a %2dHztimer\n",frequency);
    retval = 2;
    goto fail;
  }

  if (ioctl(fd,HPET_INFO, &info) < 0) {
    fprintf(stderr, "ERROR: Failed to get info\n");
    retval = 3;
    goto fail;
  }

  fprintf(stdout, "\n hi_ireqfreq: 0x%lx hi_flags: 0x%lx hi_hpet: 0x%x hi_timer: 0x%x\n\n",
      info.hi_ireqfreq, info.hi_flags, info.hi_hpet, info.hi_timer);

  r = ioctl(fd, HPET_EPI, 0);
  if (info.hi_flags && (r < 0)) {
    fprintf(stderr, "Error: HPET_EPI failed\n");
    retval = 4;
    goto fail;
  }

  if (ioctl(fd,HPET_IE_ON, 0) < 0) {
    fprintf(stderr, "Error: HPET_IE_ON failed \n");
    retval = 5;
    goto fail;
  }

  for (i = 0; i < iterations; i++){
    (void) pause();
  }

  if (ioctl(fd,HPET_IE_OFF, 0) < 0) {
    fprintf(stderr, "Error: HPET_IE_OFF failed\n");
    retval=6;
  }

fail:
  sigaction(SIGIO, &old, NULL);

  if (fd > 0)
    close(fd);

  return retval;
}
