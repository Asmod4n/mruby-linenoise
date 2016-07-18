#include <mruby.h>
#include <mruby/value.h>
#include <mruby/variable.h>
#include <mruby/array.h>
#include <mruby/string.h>
#include <errno.h>
#include <mruby/error.h>
#include <string.h>
#include <mruby/throw.h>
#include <stdlib.h>
#include "linenoise.h"

static void
mrb_linenoise_completion_callback(const char *buf, linenoiseCompletions *lc, mrb_state *mrb)
{
  int ai = mrb_gc_arena_save(mrb);

  mrb_value completion_cb = mrb_cv_get(mrb, mrb_obj_value(mrb_module_get(mrb, "Linenoise")), mrb_intern_lit(mrb, "completion_cb"));
  if (mrb_type(completion_cb) != MRB_TT_PROC) {
    return;
  }

  mrb_value res = mrb_yield(mrb, completion_cb, mrb_str_new_static(mrb, buf, strlen(buf)));
  switch(mrb_type(res)) {
    case MRB_TT_FALSE:
      break;
    case MRB_TT_ARRAY: {
      for (mrb_int ary_pos = 0; ary_pos < RARRAY_LEN(res); ary_pos++) {
        mrb_value ref = mrb_ary_ref(mrb, res, ary_pos);
        const char *completion = mrb_string_value_cstr(mrb, &ref);
        linenoiseAddCompletion(lc, completion);
      }
    } break;
    default: {
      const char *completion = mrb_string_value_cstr(mrb, &res);
      linenoiseAddCompletion(lc, completion);
    }
  }

  mrb_gc_arena_restore(mrb, ai);
}

static mrb_value
mrb_linenoiseSetCompletionCallback(mrb_state *mrb, mrb_value self)
{
  mrb_value completion_cb = mrb_nil_value();

  mrb_get_args(mrb, "&", &completion_cb);

  if (mrb_type(completion_cb) != MRB_TT_PROC) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "no block given");
  }

  mrb_cv_set(mrb, self, mrb_intern_lit(mrb, "completion_cb"), completion_cb);

  linenoiseSetCompletionCallback(mrb_linenoise_completion_callback);

  return self;
}

static char*
mrb_linenoise_hints_callback(const char *buf, int *color, int *bold, mrb_state *mrb)
{
  mrb_value hints_cb = mrb_cv_get(mrb, mrb_obj_value(mrb_module_get(mrb, "Linenoise")), mrb_intern_lit(mrb, "hints_cb"));
  if (mrb_type(hints_cb) != MRB_TT_PROC) {
    return NULL;
  }

  int ai = mrb_gc_arena_save(mrb);

  mrb_value res = mrb_yield(mrb, hints_cb, mrb_str_new_static(mrb, buf, strlen(buf)));

  if (mrb_test(res)) {
    if (mrb_respond_to(mrb, res, mrb_intern_lit(mrb, "to_str"))) {
      mrb_value hint_val = mrb_funcall(mrb, res, "to_str", 0);
      if (mrb_test(hint_val)) {
        if (mrb_respond_to(mrb, res, mrb_intern_lit(mrb, "color"))) {
          mrb_value color_val = mrb_funcall(mrb, res, "color", 0);
          mrb_int col = mrb_int(mrb, color_val);
          if (col < INT_MIN||col > INT_MAX) {
            mrb_raise(mrb, E_RANGE_ERROR, "color doesn't fit into int");
          }
          *color = col;
        }
        if (mrb_respond_to(mrb, res, mrb_intern_lit(mrb, "bold"))) {
          mrb_value bold_val = mrb_funcall(mrb, res, "bold", 0);
          *bold = mrb_bool(bold_val);
        }
        char *hint = strdup(mrb_string_value_cstr(mrb, &hint_val));
        mrb_gc_arena_restore(mrb, ai);
        if (!hint) {
          mrb_exc_raise(mrb, mrb_obj_value(mrb->nomem_err));
        }
        return hint;
      }
    }
  }

  mrb_gc_arena_restore(mrb, ai);

  return NULL;
}

static void
mrb_linenoise_free_hints_callback(void *hint)
{
  free(hint);
}

static mrb_value
mrb_linenoiseSetHintsCallback(mrb_state *mrb, mrb_value self)
{
  mrb_value hints_cb = mrb_nil_value();

  mrb_get_args(mrb, "&", &hints_cb);

  if (mrb_type(hints_cb) != MRB_TT_PROC) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "no block given");
  }

  mrb_cv_set(mrb, self, mrb_intern_lit(mrb, "hints_cb"), hints_cb);

  linenoiseSetHintsCallback(mrb_linenoise_hints_callback);
  linenoiseSetFreeHintsCallback(mrb_linenoise_free_hints_callback);

  return self;
}

static mrb_value
mrb_linenoise(mrb_state *mrb, mrb_value self)
{
  const char *prompt = "> ";

  mrb_get_args(mrb, "|z", &prompt);

  struct RString* s = (struct RString*)mrb_obj_alloc(mrb, MRB_TT_STRING, mrb->string_class);
  int errno_save = errno;
  char *line = NULL;
  size_t capa = 0;
  do {
    errno = 0;
    line = linenoise(prompt, mrb, &capa);
  } while (!line && (errno == EAGAIN||errno == EWOULDBLOCK));

  if (!line) {
    if (errno) {
      mrb_sys_fail(mrb, "linenoise");
    }
    errno = errno_save;
    return mrb_nil_value();
  }

  errno = errno_save;

  if (capa > MRB_INT_MAX) {
    memset(line, 0, capa);
    free(line);
    mrb_raise(mrb, E_ARGUMENT_ERROR, "string size too big");
  }

  s->as.heap.len = strlen(line);
  s->as.heap.aux.capa = capa;
  s->as.heap.ptr = line;

  return mrb_obj_value(s);
}

static mrb_value
mrb_linenoiseHistoryAdd(mrb_state *mrb, mrb_value self)
{
  const char *line;

  mrb_get_args(mrb, "z", &line);

  int errno_save = errno;

  errno = 0;
  if (!linenoiseHistoryAdd(line)) {
    if (errno)
      mrb_sys_fail(mrb, "linenoiseHistoryAdd");
  }

  errno = errno_save;

  return self;
}

static mrb_value
mrb_linenoiseHistorySetMaxLen(mrb_state *mrb, mrb_value self)
{
  mrb_int len;

  mrb_get_args(mrb, "i", &len);

  if (!linenoiseHistorySetMaxLen(len)) {
    mrb_sys_fail(mrb, "linenoiseHistorySetMaxLen");
  }

  return self;
}

static mrb_value
mrb_linenoiseHistorySave(mrb_state *mrb, mrb_value self)
{
  const char *filename;

  mrb_get_args(mrb, "z", &filename);

  if (linenoiseHistorySave(filename) == -1) {
    mrb_sys_fail(mrb, "linenoiseHistorySave");
  }

  return self;
}

static mrb_value
mrb_linenoiseHistoryLoad(mrb_state *mrb, mrb_value self)
{
  const char *filename;

  mrb_get_args(mrb, "z", &filename);

  if (linenoiseHistoryLoad(filename) == -1) {
    mrb_sys_fail(mrb, "linenoiseHistoryLoad");
  }

  return self;
}

static mrb_value
mrb_linenoiseClearScreen(mrb_state *mrb, mrb_value self)
{
  linenoiseClearScreen();

  return self;
}

static mrb_value
mrb_linenoiseSetMultiLine(mrb_state *mrb, mrb_value self)
{
  mrb_bool ml;

  mrb_get_args(mrb, "b", &ml);

  linenoiseSetMultiLine(ml);

  return self;
}

static mrb_value
mrb_linenoisePrintKeyCodes(mrb_state *mrb, mrb_value self)
{
  linenoisePrintKeyCodes();

  return self;
}

void
mrb_mruby_linenoise_gem_init(mrb_state* mrb)
{
  struct RClass *linenoise_mod = mrb_define_module(mrb, "Linenoise");
  struct RClass *linenoise_history_mod = mrb_define_module_under(mrb, linenoise_mod, "History");

  mrb_define_module_function(mrb, linenoise_mod, "completion", mrb_linenoiseSetCompletionCallback, MRB_ARGS_BLOCK());
  mrb_define_module_function(mrb, linenoise_mod, "hints", mrb_linenoiseSetHintsCallback, MRB_ARGS_BLOCK());
  mrb_define_module_function(mrb, mrb->kernel_module, "linenoise", mrb_linenoise, MRB_ARGS_OPT(1));

  mrb_define_module_function(mrb, linenoise_history_mod, "add", mrb_linenoiseHistoryAdd, MRB_ARGS_REQ(1));
  mrb_define_alias(mrb, linenoise_history_mod, "<<", "add");
  mrb_define_module_function(mrb, linenoise_history_mod, "max_len=", mrb_linenoiseHistorySetMaxLen, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, linenoise_history_mod, "save", mrb_linenoiseHistorySave, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, linenoise_history_mod, "load", mrb_linenoiseHistoryLoad, MRB_ARGS_REQ(1));

  mrb_define_module_function(mrb, linenoise_mod, "clear_screen", mrb_linenoiseClearScreen, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, linenoise_mod, "multi_line=", mrb_linenoiseSetMultiLine, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, linenoise_mod, "print_key_codes", mrb_linenoisePrintKeyCodes, MRB_ARGS_NONE());
}

void mrb_mruby_linenoise_gem_final(mrb_state* mrb) {}
