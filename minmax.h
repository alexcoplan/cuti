#pragma once

#include <stddef.h>

static inline size_t min_zu(size_t x, size_t y)
{
  return x < y ? x : y;
}
