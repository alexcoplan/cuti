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

static void test_overflow_counting(void)
{
  utfbuf_t ub;
  ASSERT_EQ(utfbuf_init(&ub, NULL, 0, UTF_8),
      UTF_ERROR_SUCCESS);

  ASSERT_EQ(utfbuf_overflow(&ub), 1);

  ASSERT_EQ(utfbuf_write_utf8(&ub, 'a'),
      UTF_ERROR_SUCCESS);

  ASSERT_EQ(utfbuf_overflow(&ub), 2);

  uint8_t mem[2];
  memset(mem, 0xff, sizeof(mem));

  ASSERT_EQ(utfbuf_init(&ub, mem, 1, UTF_8),
      UTF_ERROR_SUCCESS);
  ASSERT_EQ(utfbuf_overflow(&ub), 0);
  ASSERT_EQ(mem[0], 0x0);
  ASSERT_EQ(mem[1], 0xff);

  ASSERT_EQ(utfbuf_write_utf8(&ub, 'a'),
      UTF_ERROR_SUCCESS);
  ASSERT_EQ(mem[0], 0x0);
  ASSERT_EQ(mem[1], 0xff);
  ASSERT_EQ(utfbuf_overflow(&ub), 1);

  ASSERT_EQ(utfbuf_write_utf8(&ub, 'b'),
      UTF_ERROR_SUCCESS);
  ASSERT_EQ(mem[0], 0x0);
  ASSERT_EQ(mem[1], 0xff);
  ASSERT_EQ(utfbuf_overflow(&ub), 2);
}

static void test_simple_ascii(void)
{
  uint8_t buf[32];
  memset(buf, 0xff, sizeof(buf));

  utfbuf_t ub;
  utfbuf_init(&ub, buf, sizeof(buf), UTF_8);

  ASSERT_EQ(utfbuf_overflow(&ub), 0);
  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);

  ASSERT_EQ(utfbuf_write_utf8(&ub, 'a'), UTF_ERROR_SUCCESS);
  ASSERT_EQ(buf[0], 'a');
  ASSERT_EQ(buf[1], 0x0);
  ASSERT_EQ(buf[2], 0xff);

  ASSERT_EQ(utfbuf_write_utf8(&ub, 'b'), UTF_ERROR_SUCCESS);
  ASSERT_EQ(buf[0], 'a');
  ASSERT_EQ(buf[1], 'b');
  ASSERT_EQ(buf[2], 0x0);
  ASSERT_EQ(buf[3], 0xff);
}

RUN_TESTS(
    test_overflow_base_cases,
    test_overflow_counting,
    test_simple_ascii
)
