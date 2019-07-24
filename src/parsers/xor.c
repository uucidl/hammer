#include "parser_internal.h"

static HParseResult* parse_xor(void *env, HParseState *state) {
  HTwoParsers *parsers = (HTwoParsers*)env;
  // cache the initial state of the input stream
  HInputStream start_state = state->input_stream;
  HParseResult *r1 = h_do_parse(parsers->p1, state);
  HInputStream after_p1_state = state->input_stream;
  // reset input stream, parse again
  state->input_stream = start_state;
  HParseResult *r2 = h_do_parse(parsers->p2, state);
  if (NULL == r1) {
    if (NULL != r2) {
      return r2;
    } else {
      return NULL;
    }
  } else {
    if (NULL == r2) {
      state->input_stream = after_p1_state;
      return r1;
    } else {
      return NULL;
    }
  }
}

static const HParserVtable xor_vt = {
  .parse = parse_xor,
  .isValidRegular = h_false,
  .isValidCF = h_false, // XXX should this be true if both p1 and p2 are CF?
  .compile_to_rvm = h_not_regular,
  .higher = true,
};

HParser* h_xor(const HParser* p1, const HParser* p2) {
  return h_xor__m(&system_allocator, p1, p2);
}
HParser* h_xor__m(HAllocator* mm__, const HParser* p1, const HParser* p2) { 
  HTwoParsers *env = h_new(HTwoParsers, 1);
  env->p1 = p1;
  env->p2 = p2;
  return h_new_parser(mm__, &xor_vt, env);
}
