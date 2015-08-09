#include "platform.h"

#include <err.h>
#include <stdarg.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#ifdef __NetBSD__
#include <sys/resource.h>
#endif

void h_platform_errx(int errno, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  verrx(errno, format, ap);
}

static void h_benchmark_clock_gettime(struct timespec *ts) {
  if (ts == NULL)
    return;
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
  /* 
   * This returns real time, not CPU time. See http://stackoverflow.com/a/6725161
   * Possible solution: http://stackoverflow.com/a/11659289
   */
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;
#elif defined(__NetBSD__)
  // NetBSD doesn't have CLOCK_THREAD_CPUTIME_ID. We'll use getrusage instead
  struct rusage rusage;
  getrusage(RUSAGE_SELF, &rusage);
  ts->tv_nsec = (rusage.ru_utime.tv_usec + rusage.ru_stime.tv_usec) * 1000;
  // not going to overflow; can be at most 2e9-2
  ts->tv_sec = rusage.ru_utime.tv_sec + rusage.ru_utime.tv_sec;
  if (ts->tv_nsec >= 1000000000) {
    ts->tv_nsec -=   1000000000; // subtract a second
    ts->tv_sec += 1; // add it back.
  }
  assert (ts->tv_nsec <= 1000000000);
#else
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, ts);
#endif
}

void h_platform_stopwatch_reset(struct HStopWatch* stopwatch) {
  h_benchmark_clock_gettime(&stopwatch->start);
}

int64_t platform_stopwatch_ns(struct HStopWatch* stopwatch) {
  struct timespec ts_now;
  h_benchmark_clock_gettime(&ts_now);

  // time_diff is in ns
  return (ts_now.tv_sec - ts_start.tv_sec) * 1000000000 + (ts_now.tv_nsec - ts_start.tv_nsec);
}
