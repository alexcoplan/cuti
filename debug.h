#pragma once

#define UTFDBG(lvl, fmt, ...)\
  fprintf(stderr, fmt "\n", ##__VA_ARGS__);

#include <stdio.h>
