#include "test.h"

#include <stdio.h>

static void first_test(void)
{
  ASSERT_EQ(1, 1);
}

static void second_test(void)
{
  ASSERT_EQ(1, 2);
}

RUN_TESTS(
  first_test,
  second_test
)
