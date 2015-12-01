#include <glib.h>
#include <string.h>
#include <sys/resource.h>
#include "test_suite.h"
#include "hammer.h"

static void test_tt_user(void) {
  g_check_cmp_int32(TT_USER, >, TT_NONE);
  g_check_cmp_int32(TT_USER, >, TT_BYTES);
  g_check_cmp_int32(TT_USER, >, TT_SINT);
  g_check_cmp_int32(TT_USER, >, TT_UINT);
  g_check_cmp_int32(TT_USER, >, TT_SEQUENCE);
  g_check_cmp_int32(TT_USER, >, TT_ERR);
}

static void test_tt_registry(void) {
  int id = h_allocate_token_type("com.upstandinghackers.test.token_type");
  g_check_cmp_int32(id, >=, TT_USER);
  int id2 = h_allocate_token_type("com.upstandinghackers.test.token_type_2");
  g_check_cmp_int32(id2, !=, id);
  g_check_cmp_int32(id2, >=, TT_USER);
  g_check_cmp_int32(id, ==, h_get_token_type_number("com.upstandinghackers.test.token_type"));
  g_check_cmp_int32(id2, ==, h_get_token_type_number("com.upstandinghackers.test.token_type_2"));
  g_check_string("com.upstandinghackers.test.token_type", ==, h_get_token_type_name(id));
  g_check_string("com.upstandinghackers.test.token_type_2", ==, h_get_token_type_name(id2));
  if (h_get_token_type_name(0) != NULL) {
    g_test_message("Unknown token type should not return a name");
    g_test_fail();
  }
  g_check_cmp_int32(h_get_token_type_number("com.upstandinghackers.test.unkown_token_type"), ==, 0);
}

// perform a big allocation during parsing to trigger out-of-memory handling
static HParsedToken *act_big_alloc(const HParseResult *r, void *user) {
  void *buf = h_arena_malloc(r->arena, 500*1024*1024);
  assert(buf != NULL);
  g_test_message("Memory allocation was supposed to fail");
  return NULL;
}
static void test_oom(void) {
  HParser *p = h_action(h_ch('x'), act_big_alloc, NULL);
    // this should always fail, but never crash

  struct rlimit bak, lim;
  int i;
  i = getrlimit(RLIMIT_DATA, &bak);
  assert(i == 0);
  lim.rlim_cur = 499*1024*1024;   // never enough
  if(lim.rlim_cur > bak.rlim_max)
    lim.rlim_cur = bak.rlim_max;
  lim.rlim_max = bak.rlim_max;
  i = setrlimit(RLIMIT_DATA, &lim);
  assert(i == 0);

  g_check_parse_failed(p, PB_PACKRAT, "x",1);
  g_check_parse_failed(p, PB_REGULAR, "x",1);
  g_check_parse_failed(p, PB_LLk, "x",1);
  g_check_parse_failed(p, PB_LALR, "x",1);
  g_check_parse_failed(p, PB_GLR, "x",1);

  //g_check_parse_chunks_failed(p, PB_REGULAR, "",0, "x",1);
  g_check_parse_chunks_failed(p, PB_LLk, "",0, "x",1);
  g_check_parse_chunks_failed(p, PB_LALR, "",0, "x",1);
  //g_check_parse_chunks_failed(p, PB_GLR, "",0, "x",1);

  i = setrlimit(RLIMIT_DATA, &bak);
  assert(i == 0);
}

void register_misc_tests(void) {
  g_test_add_func("/core/misc/tt_user", test_tt_user);
  g_test_add_func("/core/misc/tt_registry", test_tt_registry);
  g_test_add_func("/core/misc/oom", test_oom);
}
