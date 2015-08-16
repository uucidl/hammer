/* Parser combinators for binary formats.
 * Copyright (C) 2012  Meredith L. Patterson, Dan "TQ" Hirsch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#define USE_TSEARCH 0

#if USE_TSEARCH
#include <search.h> // TODO(uucidl) win32 implementation (binary tree? hashtable?)
#endif

#include <stdlib.h>
#include "hammer.h"
#include "internal.h"

typedef struct Entry_ {
  const char* name;
  HTokenType value;
} Entry;

static void *tt_registry = NULL;
// TODO(uucidl): this is also a stretchy buffer
static const char** tt_name_by_id = NULL;
static unsigned int tt_name_by_id_sz = 0;
#define TT_START TT_USER
static HTokenType tt_next = TT_START;

/*
  // TODO: These are for the extension registry, which does not yet have a good name.
static void *ext_registry = NULL;
static Entry** ext_by_id = NULL;
static int ext_by_id_sz = 0;
static int ext_next = 0;
*/

/**
 * flat, growable array storing all token type entries by name,
 * sorted.
 *
 * We reuse the structure, with new semantics here and allocated differently.
 */
// TODO(uucidl): use a stretchy buffer abstraction and look for all
// calls to realloc elsewhere to port to it. HCountedArray can be
// adapted to use it, as well as the code below (see tt_id array) look
// for _sz conventions and _next
static HCountedArray tt_by_names_sorted;

/** lookup first entry with a name not < to our search name */
static Entry* tt_by_names_lookup_lower_bound(Entry* first, Entry* last, const char * name) {
  // TODO(uucidl): dummy O(n) implementation for now, change to binary search
  while (first != last) {
    int elem_is_less = strcmp(first->name, name) < 0;
    if (!elem_is_less) {
      break;
    }
    ++first;
  }

  return first;
}

static Entry* tt_names_begin(HCountedArray* array) {
  return (Entry*)array->elements;
}

static Entry* tt_names_end(HCountedArray* array) {
  return tt_names_begin(array) + array->used;
}

/** inserts entry before position and returns new entry pointer */
static Entry* tt_names_insert(HCountedArray* array, Entry* position, const Entry* entry) {
  Entry* first = tt_names_begin(array);
  ptrdiff_t position_offset = position - first;
  size_t needed_capacity = array->used + 1;
  if (needed_capacity > array->capacity) {
    size_t new_capacity = array->capacity < 16 ? 16 : 2*array->capacity;
    array->elements = (&system_allocator)->realloc(&system_allocator, array->elements, sizeof(Entry)*new_capacity);
    // TODO(uucidl): memory error handling
    array->capacity = new_capacity;
    first = tt_names_begin(array);
  }
  position = first + position_offset;
  memmove(position + 1, position, sizeof(Entry)*(array->used - position_offset));
  *position = *entry;
  array->used += 1;

  return position;
}

#if USE_TSEARCH
static int compare_entries(const void* v1, const void* v2) {
  const Entry *e1 = (Entry*)v1, *e2 = (Entry*)v2;
  return strcmp(e1->name, e2->name);
}
#endif

HTokenType h_allocate_token_type(const char* name) {
#if USE_TSEARCH
  Entry* new_entry = (&system_allocator)->alloc(&system_allocator, sizeof(*new_entry));
  if (!new_entry) {
    return TT_INVALID;
  }
  new_entry->name = name;
  new_entry->value = 0;
  // TODO(uucidl) tsearch for windows
  Entry* probe = *(Entry**)tsearch(new_entry, &tt_registry, compare_entries);
  if (probe->value != 0) {
    // Token type already exists...
    // TODO: treat this as a bug?
    (&system_allocator)->free(&system_allocator, new_entry);
    return probe->value;
  }
#else
  Entry* last = tt_names_end(&tt_by_names_sorted);
  Entry* probe = tt_by_names_lookup_lower_bound(tt_names_begin(&tt_by_names_sorted), last, name);
  if (probe != last && 0 == strcmp(probe->name, name)) {
    // Token type already exists...
    // TODO: treat this as a bug?
    return probe->value;
  }
  Entry entry;
  probe = tt_names_insert(&tt_by_names_sorted, probe, &entry);
#endif
  {
    // new value
/*
  TODO(uucidl): what is this comment saying really?
    probe->name = strdup(probe->name); // drop ownership of name
*/
    probe->name = strdup(name);
    probe->value = tt_next++;
    if ((size_t)(probe->value - TT_START) >= tt_name_by_id_sz) {
      if (tt_name_by_id_sz == 0) {
        // TODO(uucidl): why not system allocator?
        tt_name_by_id = malloc(sizeof(*tt_name_by_id) * ((tt_name_by_id_sz = (tt_next - TT_START) * 16)));
      } else {
        tt_name_by_id = realloc((void*)tt_name_by_id, sizeof(*tt_name_by_id) * ((tt_name_by_id_sz *= 2)));
      }
      if (!tt_name_by_id) {
        return TT_INVALID;
      }
    }
    assert((size_t)(probe->value - TT_START) < tt_name_by_id_sz);
    tt_name_by_id[probe->value - TT_START] = probe->name;
    return probe->value;
  }
}
HTokenType h_get_token_type_number(const char* name) {
#if USE_TSEARCH
  Entry e;
  e.name = name;
  // TODO(uucidl): tfind for windows
  Entry **ret = (Entry**)tfind(&e, &tt_registry, compare_entries);
  if (ret == NULL)
    return 0; // TODO(uucidl): TT_INVALID
  else
    return (*ret)->value;
#else
  Entry* first = tt_names_begin(&tt_by_names_sorted);
  Entry* last = tt_names_end(&tt_by_names_sorted);
  Entry* name_lower_bound = tt_by_names_lookup_lower_bound(first, last, name);
  if (name_lower_bound == last || 0 != strcmp(name_lower_bound->name, name)) {
    return TT_INVALID;
  }

  return name_lower_bound->value;
#endif
}
const char* h_get_token_type_name(HTokenType token_type) {
  if (token_type >= tt_next || token_type < TT_START)
    return NULL;
  else
    return tt_name_by_id[token_type - TT_START];
}
