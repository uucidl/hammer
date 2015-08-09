#include <string.h>
#include <stdlib.h>
#include "internal.h"

//#define DEBUG__MEMFILL 0xFF

typedef struct HSystemAllocHeader_
{
  size_t size;
} HSystemAllocHeader;

static inline size_t compute_block_size(size_t size) {
  return sizeof(HSystemAllocHeader) + size;
}

static inline HSystemAllocHeader* get_user_ptr_block(void *uptr) {
  return (HSystemAllocHeader*)((char*)uptr - sizeof(HSystemAllocHeader));
}

static inline void* get_user_ptr(HSystemAllocHeader *header) {
  return header + 1;
}

static void* system_alloc(HAllocator *allocator, size_t size) {
  size_t block_size = compute_block_size(size);
  HSystemAllocHeader *header = malloc(block_size);
  if (!header) {
    return NULL;
  }
  void *uptr = get_user_ptr(header);
#ifdef DEBUG__MEMFILL
  memset(uptr, DEBUG__MEMFILL, block_size);
#endif
  header->size = size;
  return uptr;
}

static void* system_realloc(HAllocator *allocator, void* uptr, size_t size) {
  if (!uptr) {
    return system_alloc(allocator, size);
  }
  HSystemAllocHeader* header = realloc(get_user_ptr_block(uptr), compute_block_size(size));
  if (!header) {
    return NULL;
  }
  uptr = get_user_ptr(header);

#ifdef DEBUG__MEMFILL
  size_t old_size = header->size;
  
  if (size > old_size)
    memset((char*)uptr+old_size, DEBUG__MEMFILL, size - old_size);
#endif
  header->size = size;
  return uptr;
}

static void system_free(HAllocator *allocator, void* uptr) {
  if (uptr) {
    free(get_user_ptr_block(uptr));
  }
}

HAllocator system_allocator = {
  .alloc = system_alloc,
  .realloc = system_realloc,
  .free = system_free,
};
