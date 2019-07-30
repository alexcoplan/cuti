#pragma once

#define UTFDBG(lvl, fmt, ...)\
  fprintf(stderr, fmt "\n", ##__VA_ARGS__);

#define UTF_RASSERT(expr, ...)\
  do { \
    if (!(expr)) { \
      UTFDBG(1, "Assertion failed: " #expr ": " __VA_ARGS__); \
      abort(); \
    } \
  } while(0)

// TODO: disable in release build.
#define UTF_DASSERT(expr, ...) UTF_RASSERT(expr, __VA_ARGS__)

#include <stdio.h>
#include <stdlib.h>
