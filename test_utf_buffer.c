#include "utf_buffer.h"
#include "test.h"

#include <string.h>

static void test_overflow_base_cases(void)
{
  utfbuf_t ub;
  ASSERT_EQ(utfbuf_init(&ub, NULL, 0, UTF_8),
      UTF_ERROR_SUCCESS);

  // We always need a null terminator, so we've
  // already overflowed by 1 here.
  ASSERT_EQ(utfbuf_overflow(&ub), 1);

  uint8_t buf[2];
  memset(buf, 0xff, sizeof(buf));

  ASSERT_EQ(utfbuf_init(&ub, buf, 0, UTF_8),
      UTF_ERROR_SUCCESS);

  ASSERT_EQ(utfbuf_overflow(&ub), 1);

  ASSERT_EQ(buf[0], 0xff);
  ASSERT_EQ(buf[1], 0xff);

  ASSERT_EQ(utfbuf_init(&ub, buf, 1, UTF_8),
      UTF_ERROR_SUCCESS);
  ASSERT_EQ(utfbuf_overflow(&ub), 0);

  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);
}

RUN_TESTS(test_overflow_base_cases)
