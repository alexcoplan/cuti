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

  ASSERT_EQ(utfbuf_write_utf8_byte(&ub, 'a'),
      UTF_ERROR_SUCCESS);

  ASSERT_EQ(utfbuf_overflow(&ub), 2);

  uint8_t mem[2];
  memset(mem, 0xff, sizeof(mem));

  ASSERT_EQ(utfbuf_init(&ub, mem, 1, UTF_8),
      UTF_ERROR_SUCCESS);
  ASSERT_EQ(utfbuf_overflow(&ub), 0);
  ASSERT_EQ(mem[0], 0x0);
  ASSERT_EQ(mem[1], 0xff);

  ASSERT_EQ(utfbuf_write_utf8_byte(&ub, 'a'),
      UTF_ERROR_SUCCESS);
  ASSERT_EQ(mem[0], 0x0);
  ASSERT_EQ(mem[1], 0xff);
  ASSERT_EQ(utfbuf_overflow(&ub), 1);

  ASSERT_EQ(utfbuf_write_utf8_byte(&ub, 'b'),
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

  ASSERT_EQ(utfbuf_write_utf8_byte(&ub, 'a'), UTF_ERROR_SUCCESS);
  ASSERT_EQ(buf[0], 'a');
  ASSERT_EQ(buf[1], 0x0);
  ASSERT_EQ(buf[2], 0xff);

  ASSERT_EQ(utfbuf_write_utf8_byte(&ub, 'b'), UTF_ERROR_SUCCESS);
  ASSERT_EQ(buf[0], 'a');
  ASSERT_EQ(buf[1], 'b');
  ASSERT_EQ(buf[2], 0x0);
  ASSERT_EQ(buf[3], 0xff);
}

typedef struct {
  uint8_t u8[4];
  uint16_t u16[2];
  uint32_t u32[1];
} utf_char_t;

static const utf_char_t e_acute = {
  .u8  = { 0xC3, 0xA9 },
  .u16 = { 0xE9 },
  .u32 = { 0xE9 },
};

static const utf_char_t quaver = {
  .u8  = { 0xE2, 0x99, 0xAA },
  .u16 = { 0x266A },
  .u32 = { 0x266A },
};

static const utf_char_t unicorn = {
  .u8  = { 0xF0, 0x9F, 0xA6, 0x84 },
  .u16 = { 0xD83E, 0xDD84 },
  .u32 = { 0x1F984 },
};

static uint8_t utf8_count(const utf_char_t *uc)
{
  uint8_t i;
  for (i = 0; uc->u8[i] && i < 4; i++);
  return i;
}

static void test_utf8_simple(void)
{
  uint8_t buf[32];
  memset(buf, 0xff, sizeof(buf));

  utfbuf_t ub;
  utfbuf_init(&ub, buf, sizeof(buf), UTF_8);

  ASSERT_EQ(utfbuf_overflow(&ub), 0);
  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);


  ASSERT_EQ(utfbuf_write_utf8_byte(&ub, e_acute.u8[0]),
      UTF_ERROR_SUCCESS);

  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);

  ASSERT_EQ(utfbuf_write_utf8_byte(&ub, e_acute.u8[1]),
      UTF_ERROR_SUCCESS);

  ASSERT_EQ(buf[0], e_acute.u8[0]);
  ASSERT_EQ(buf[1], e_acute.u8[1]);
  ASSERT_EQ(buf[2], 0x0);
  ASSERT_EQ(buf[3], 0xff);

  // Now test where it only just fits.
  memset(buf, 0xff, sizeof(buf));
  utfbuf_init(&ub, buf, 3, UTF_8);

  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);

  ASSERT_EQ(utfbuf_write_utf8_byte(&ub, e_acute.u8[0]),
        UTF_ERROR_SUCCESS);

  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);

  ASSERT_EQ(utfbuf_write_utf8_byte(&ub, e_acute.u8[1]),
        UTF_ERROR_SUCCESS);

  ASSERT_EQ(buf[0], e_acute.u8[0]);
  ASSERT_EQ(buf[1], e_acute.u8[1]);
  ASSERT_EQ(buf[2], 0x0);
  ASSERT_EQ(buf[3], 0xff);

  // Now test truncation.
  memset(buf, 0xff, sizeof(buf));
  utfbuf_init(&ub, buf, 3, UTF_8);

  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);

  const uint8_t to_write[] = {
    'a', e_acute.u8[0], e_acute.u8[1]
  };
  for (size_t i = 0; i < ARRAY_LENGTH(to_write); i++) {
    ASSERT_EQ(utfbuf_write_utf8_byte(&ub, to_write[i]),
          UTF_ERROR_SUCCESS);

    ASSERT_EQ(buf[0], 'a');
    ASSERT_EQ(buf[1], 0x0);
    ASSERT_EQ(buf[2], 0xff);
  }

  ASSERT_EQ(utfbuf_overflow(&ub), 1);

  memset(buf, 0xff, sizeof(buf));
  utfbuf_init(&ub, buf, sizeof(buf), UTF_8);

  for (size_t i = 0; i < utf8_count(&quaver); i++) {
    ASSERT_EQ(buf[0], 0x0);
    ASSERT_EQ(buf[1], 0xff);
    ASSERT_EQ(utfbuf_write_utf8_byte(&ub, quaver.u8[i]),
        UTF_ERROR_SUCCESS);
  }

  ASSERT_EQ(buf[0], quaver.u8[0]);
  ASSERT_EQ(buf[1], quaver.u8[1]);
  ASSERT_EQ(buf[2], quaver.u8[2]);
  ASSERT_EQ(buf[3], 0x0);
  ASSERT_EQ(buf[4], 0xff);

  memset(buf, 0xff, sizeof(buf));
  utfbuf_init(&ub, buf, sizeof(buf), UTF_8);

  const uint8_t uni_count = utf8_count(&unicorn);

  for (size_t i = 0; i < uni_count; i++) {
    ASSERT_EQ(buf[0], 0x0);
    ASSERT_EQ(buf[1], 0xff);
    ASSERT_EQ(utfbuf_write_utf8_byte(&ub, unicorn.u8[i]),
        UTF_ERROR_SUCCESS);
  }

  ASSERT_EQ(memcmp(unicorn.u8, buf, uni_count), 0);

  ASSERT_EQ(buf[uni_count], 0x0);
  ASSERT_EQ(buf[uni_count+1], 0xff);

  ASSERT_EQ(utfbuf_write_utf8_byte(&ub, 'a'),
      UTF_ERROR_SUCCESS);

  ASSERT_EQ(memcmp(unicorn.u8, buf, uni_count), 0);
  ASSERT_EQ(buf[uni_count], 'a');
  ASSERT_EQ(buf[uni_count+1], 0x0);
  ASSERT_EQ(buf[uni_count+2], 0xff);
}

static void test_utf8_invalid(void)
{
  uint8_t buf[32];
  memset(buf, 0xff, sizeof(buf));

  utfbuf_t ub;
  utfbuf_init(&ub, buf, sizeof(buf), UTF_8);

  // Invalid character.
  ASSERT_EQ(utfbuf_write_utf8_byte(&ub, 0xff),
      UTF_ERROR_INVALID_ARGUMENT);

  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);

  // Unexpected continuation byte.
  ASSERT_EQ(utfbuf_write_utf8_byte(&ub, 0x80),
      UTF_ERROR_INVALID_ARGUMENT);

  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);

  ASSERT_EQ(utfbuf_write_utf8_byte(&ub, e_acute.u8[0]),
      UTF_ERROR_SUCCESS);

  // Expect continuation char, but get 'a'.
  ASSERT_EQ(utfbuf_write_utf8_byte(&ub, 'a'),
      UTF_ERROR_INVALID_ARGUMENT);

  ASSERT_EQ(buf[0], 0x0);
  ASSERT_EQ(buf[1], 0xff);

  // Now check we can actually write an 'a'.
  ASSERT_EQ(utfbuf_write_utf8_byte(&ub, 'a'),
      UTF_ERROR_SUCCESS);

  ASSERT_EQ(buf[0], 'a');
  ASSERT_EQ(buf[1], 0x0);
  ASSERT_EQ(buf[2], 0xff);
}

static void test_utf32_simple(void)
{
  uint32_t buf32[8];
  utfbuf_t ub;
  ASSERT_EQ(utfbuf_init(&ub, buf32, sizeof(buf32), UTF_32),
      UTF_ERROR_SUCCESS);

  memset(buf32, 0xff, sizeof(buf32));
  ASSERT_EQ(utfbuf_write_utf8_byte(&ub, 'a'),
      UTF_ERROR_SUCCESS);

  ASSERT_EQ(buf32[0], 'a');
  ASSERT_EQ(buf32[1], 0x0);
  ASSERT_EQ(buf32[2], 0xffffffff);
}

RUN_TESTS(
    test_overflow_base_cases,
    test_overflow_counting,
    test_simple_ascii,
    test_utf8_simple,
    test_utf8_invalid,
    test_utf32_simple,
)
