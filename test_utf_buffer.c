#include "utf_buffer.h"
#include "test.h"
#include "macros.h"

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

static const uint8_t e_acute[] = { 0xC3, 0xA9 };
static const uint8_t quaver[] = { 0xE2, 0x99, 0xAA };

static void test_utf8_simple(void)
{
  uint8_t buf[32];
  memset(buf, 0xff, sizeof(buf));

  utfbuf_t ub;
  utfbuf_init(&ub, buf, sizeof(buf), UTF_8);

  ASSERT_EQ(utfbuf_overflow(&ub), 0);
  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);


  ASSERT_EQ(utfbuf_write_utf8(&ub, e_acute[0]),
      UTF_ERROR_SUCCESS);

  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);

  ASSERT_EQ(utfbuf_write_utf8(&ub, e_acute[1]),
      UTF_ERROR_SUCCESS);

  ASSERT_EQ(buf[0], e_acute[0]);
  ASSERT_EQ(buf[1], e_acute[1]);
  ASSERT_EQ(buf[2], 0x0);
  ASSERT_EQ(buf[3], 0xff);

  // Now test where it only just fits.
  memset(buf, 0xff, sizeof(buf));
  utfbuf_init(&ub, buf, 3, UTF_8);

  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);

  ASSERT_EQ(utfbuf_write_utf8(&ub, e_acute[0]),
        UTF_ERROR_SUCCESS);

  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);

  ASSERT_EQ(utfbuf_write_utf8(&ub, e_acute[1]),
        UTF_ERROR_SUCCESS);

  ASSERT_EQ(buf[0], e_acute[0]);
  ASSERT_EQ(buf[1], e_acute[1]);
  ASSERT_EQ(buf[2], 0x0);
  ASSERT_EQ(buf[3], 0xff);

  // Now test truncation.
  memset(buf, 0xff, sizeof(buf));
  utfbuf_init(&ub, buf, 3, UTF_8);

  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);

  const uint8_t to_write[] = { 'a', e_acute[0], e_acute[1] };
  for (size_t i = 0; i < ARRAY_LENGTH(to_write); i++) {
    ASSERT_EQ(utfbuf_write_utf8(&ub, to_write[i]),
          UTF_ERROR_SUCCESS);

    ASSERT_EQ(buf[0], 'a');
    ASSERT_EQ(buf[1], 0x0);
    ASSERT_EQ(buf[2], 0xff);
  }

  ASSERT_EQ(utfbuf_overflow(&ub), 1);

  memset(buf, 0xff, sizeof(buf));
  utfbuf_init(&ub, buf, sizeof(buf), UTF_8);

  for (size_t i = 0; i < sizeof(quaver); i++) {
    ASSERT_EQ(buf[0], 0x0);
    ASSERT_EQ(buf[1], 0xff);
    ASSERT_EQ(utfbuf_write_utf8(&ub, quaver[i]),
        UTF_ERROR_SUCCESS);
  }

  ASSERT_EQ(buf[0], quaver[0]);
  ASSERT_EQ(buf[1], quaver[1]);
  ASSERT_EQ(buf[2], quaver[2]);
  ASSERT_EQ(buf[3], 0x0);
  ASSERT_EQ(buf[4], 0xff);
}

static void test_utf8_invalid(void)
{
  uint8_t buf[32];
  memset(buf, 0xff, sizeof(buf));

  utfbuf_t ub;
  utfbuf_init(&ub, buf, sizeof(buf), UTF_8);

  // Invalid character.
  ASSERT_EQ(utfbuf_write_utf8(&ub, 0xff),
      UTF_ERROR_INVALID_ARGUMENT);

  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);

  // Unexpected continuation byte.
  ASSERT_EQ(utfbuf_write_utf8(&ub, 0x80),
      UTF_ERROR_INVALID_ARGUMENT);

  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);

  ASSERT_EQ(utfbuf_write_utf8(&ub, e_acute[0]),
      UTF_ERROR_SUCCESS);

  // Expect continuation char, but get 'a'.
  ASSERT_EQ(utfbuf_write_utf8(&ub, 'a'),
      UTF_ERROR_INVALID_ARGUMENT);

  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);

  // Now check we can actually write an 'a'.
  ASSERT_EQ(utfbuf_write_utf8(&ub, 'a'),
      UTF_ERROR_SUCCESS);

  ASSERT_EQ(buf[0], 'a');
  ASSERT_EQ(buf[1], 0x0);
  ASSERT_EQ(buf[2], 0xff);
}

RUN_TESTS(
    test_overflow_base_cases,
    test_overflow_counting,
    test_simple_ascii,
    test_utf8_simple,
    test_utf8_invalid,
)
