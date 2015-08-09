#ifndef HAMMER_MA__H
#define HAMMER_MA__H

/**
 * @file utilities for writing multiple argument parsers.
 */

#include "../hammer.h" /* for HParser */
#include "../internal.h" /* for h_new */

typedef struct
{
  size_t len;
  HParser **p_array;
} HParserArray;

/* TODO(uucidl): I think the pattern I replaced with this is actually
   repeated in the code base quite a bit, see if we can share it. */
static inline size_t count_args_with_sentinel(void *args[]) {
  size_t count = 0;
  void** arg = args;
  while (*arg) {
    ++count;
    ++arg;
  }
  return count;
}

/**
 * Precondition: dest must contain at least
 * `count_args_with_sentinel(args)` elements.
 */
static inline HParser** copy_args_with_sentinel(void *args[], HParser *dest[]) {
  void** arg = args;
  while (*arg) {
    *dest = *arg;
    ++arg;
    ++dest;
  }
  return dest;
}

static inline
HParserArray* sequence_for_args_with_sentinel(HAllocator* mm__, void *args[]) {
  size_t len = count_args_with_sentinel(args);

  HParserArray *s = h_new(HParserArray, 1);
  s->p_array = h_new(HParser *, len);
  copy_args_with_sentinel(args, s->p_array);

  s->len = len;
  return s;
}

#endif
