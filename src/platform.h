#ifndef HAMMER_PLATFORM__H
#define HAMMER_PLATFORM__H

/**
 * @file interface between hammer and the operating system /
 * underlying platform.
 */

#include "compiler_specifics.h"

#include <stddef.h>
#include <stdint.h>

/* Call Stack Allocation i.e. `alloca` */

/**
 * Allocate an array of elements on the stack and returns a pointer to it.
 *
 * Result is platform dependent when the stack is exhausted.
 */
void* h_platform_stack_alloc_n(size_t elem_count, size_t elem_size);

/* Error Reporting */

/* BSD errx function, seen in err.h */
H_MSVC_DECLSPEC(noreturn) \
void h_platform_errx(int err, const char* format, ...)	\
  H_GCC_ATTRIBUTE((noreturn, format (printf,2,3)));

/* Time Measurement */

struct HStopWatch; /* forward definition */

/* initialize a stopwatch */
void h_platform_stopwatch_reset(struct HStopWatch* stopwatch);

/* return difference between last reset point and now */
uint64_t h_platform_stopwatch_ns(struct HStopWatch* stopwatch);

/* Platform dependent definitions for HStopWatch */
#if defined(_MSC_VER)

__pragma(warning(push,1))
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
__pragma(warning(pop))

struct HStopWatch {
  LARGE_INTEGER qpf;
  LARGE_INTEGER start;
};

#else
/* Unix like platforms */

#include <time.h>

struct HStopWatch {
  struct timespec ts_start;
};

#endif

#endif
