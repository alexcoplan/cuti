#include "test.h"
#include "macros.h"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

extern const test_manifest_t test_manifest_;

static struct {
  bool failed;
  char msg[1024];
} run_state;

int test_main(void)
{
  const test_manifest_t *data = &test_manifest_;

  char names[1024];
  snprintf(names, sizeof(names), "%s", data->names);

  char *tmp = names;

  for (size_t i = 0; i < data->n_fns; i++) {
    char *comma = strchr(tmp, ',');
    if (comma) {
      *comma = '\0';
    }

    fprintf(stderr, "TEST %30s... ", tmp);
    data->test_fns[i]();
    if (run_state.failed) {
      fprintf(stderr, "NAY\n");
      fprintf(stderr, "%s\n", run_state.msg);
      run_state.failed = false;
    } else {
      fprintf(stderr, "YAY\n");
    }

    if (!comma)
      break;

    tmp = comma + 2;
  }

  return 0;
}

void test_fail(const char *fmt, ...)
{
  run_state.failed = true;

  va_list va;
  va_start(va, fmt);
  vsnprintf(run_state.msg, sizeof(run_state.msg), fmt, va);
  va_end(va);
}
