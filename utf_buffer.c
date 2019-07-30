#include "debug.h"
#include "minmax.h"
#include "utf_buffer.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static size_t ub_bytes_remaining(utfbuf_t *ub)
{
  return ub->size - ub->pos;
}

static void ub_write_entire_pattern(utfbuf_t *ub,
    uint8_t pat, size_t n)
{
  const size_t rem = ub_bytes_remaining(ub);

  if (rem >= n) {
    memset(ub->start + ub->pos, pat, n);
    ub->pos += n;
  } else {
    ub->overflow += (n - rem);
  }
}

static void ub_write_null(utfbuf_t *ub)
{
  ub_write_entire_pattern(ub, 0x0, utf_bytes(ub->enc));
}

utf_error_t utfbuf_init(utfbuf_t *ub,
    void *mem, size_t mem_size, utf_enc_t encoding)
{
  switch (encoding) {
  case UTF_8:
  case UTF_16:
  case UTF_32:
    break;
  default:
    return UTF_ERROR_INVALID_ARGUMENT;
  }

  *ub = (utfbuf_t){
    .start = mem,
    .size = mem_size,
    .enc = encoding,
  };

  ub_write_null(ub);
  return UTF_ERROR_SUCCESS;
}

static void write_utf_internal(utfbuf_t *ub,
    const void *utf, uint8_t n, utf_enc_t enc)
{
  const uint8_t width = utf_bytes(enc);
  const size_t required = n * width;
  const size_t avail = ub_bytes_remaining(ub);
  if (required <= avail) {
    if (ub->pos) {
      memcpy(ub->start + ub->pos - width, utf, required);
      memset(ub->start + ub->pos + (n-1)*width, 0x0, width);
      ub->pos += required;
    }
  } else {
    ub->overflow += (required - avail);
  }
}

static const uint8_t utf8_mask_table[] = {
  0x7f, 0x1f, 0x0f, 0x07,
};

static void ub_write_utf8_of_utf32(utfbuf_t *ub)
{
  uint32_t u32 = ub->in.u32[0];
  uint8_t u8[4] = { 0 };
  uint8_t count, i;

  static const uint32_t limits[] = {
    0x7f, 0x7ff, 0xffff
  };
  for (count = 1; count < 4; count++) {
    if (u32 < limits[count-1])
      break;
  }

  for (i = count; i > 1; i--) {
    u8[i-1] = 0x80 | (u32 & 0x3f);
    u32 >>= 6;
  }

  static const uint8_t top_bits[] = {
    0x0, 0xc0, 0xe0, 0xf0,
  };

  i = count-1;
  u8[0] = top_bits[i] | (u32 & utf8_mask_table[i]);

  write_utf_internal(ub, u8, count, UTF_8);
}

static void ub_write_utf32_of_utf8(utfbuf_t *ub)
{
  const uint8_t *u8 = ub->in.u8;
  const uint8_t n = ub->in.count;

  uint32_t out = 0;
  uint8_t bits = 0;

  for (uint8_t i = n; i > 1; i--) {
    out |= (u8[i-1] & 0x3f) << bits;
    bits += 6;
  }
  out |= (u8[0] & utf8_mask_table[n-1]) << bits;

  write_utf_internal(ub, &out, 1, UTF_32);
}

static void ub_write_utf8_cp(utfbuf_t *ub)
{
  switch (ub->enc) {
    case UTF_8:
      write_utf_internal(ub, ub->in.u8, ub->in.count, UTF_8);
      break;
    case UTF_32:
      ub_write_utf32_of_utf8(ub);
      break;
    default:
      UTF_RASSERT(0, "encoding %u", ub->enc);
  }
}

static void ub_write_utf32_cp(utfbuf_t *ub)
{
  switch (ub->enc) {
    case UTF_8:
      ub_write_utf8_of_utf32(ub);
      break;
    case UTF_32:
      write_utf_internal(ub, &ub->in.u32, 1, UTF_32);
      break;
    default:
      UTF_RASSERT(0, "encoding %u", ub->enc);
  }
}

static void ub_write_utf16_cp(utfbuf_t *ub)
{
  switch (ub->enc) {
    case UTF_16:
      write_utf_internal(ub,
          ub->in.u16, ub->in.count, UTF_16);
      break;
    default:
      UTF_RASSERT(0, "encoding %u", ub->enc);
  }
}

static void ub_write_codepoint(utfbuf_t *ub)
{
  switch (ub->in.enc) {
    case UTF_8:
      ub_write_utf8_cp(ub);
      break;
    case UTF_16:
      ub_write_utf16_cp(ub);
      break;
    case UTF_32:
      ub_write_utf32_cp(ub);
      break;
    default:
      UTF_RASSERT(0, "encoding %u", ub->in.enc);
  }

  ub->in = (ub_inbuf_t) { 0 };
}

static uint8_t count_bits_u8(uint8_t byte)
{
  uint8_t count = 0;
  for (uint8_t x = 1 << 7; x > 0; x >>= 1) {
    if (!(byte & x))
      return count;

    count += 1;
  }

  return count;
}

utf_error_t utfbuf_write_utf8_string(utfbuf_t *ub, const char *str)
{
  const bool real_ub = !!ub->size;
  utf_error_t err;

  for (size_t i = 0; str[i]; i++) {
    err = utfbuf_write_utf8(ub, str[i]);
    if (err)
      return err;

    if (real_ub && utfbuf_overflow(ub)) {
      // If we're not just a 'byte counting' utfbuf,
      // and we've overflowed, then refuse to
      // write any more.
      return UTF_ERROR_SUCCESS;
    }
  }

  return UTF_ERROR_SUCCESS;
}

utf_error_t utfbuf_write_utf8(utfbuf_t *ub, uint8_t byte)
{
  if (ub->enc == UTF_16) {
    return UTF_ERROR_NOT_IMPLEMENTED;
  }

  const uint8_t count = count_bits_u8(byte);
  if (!ub->in.enc) {
    switch (count) {
      case 0: // single-octet codepoint
        ub->in.enc = UTF_8;
        ub->in.u8[0] = byte;
        ub->in.count = 1;
        ub_write_codepoint(ub);
        return UTF_ERROR_SUCCESS;
      case 2:
      case 3:
      case 4:
        ub->in.enc = UTF_8;
        ub->in.u8[0] = byte;
        ub->in.count = count;
        return UTF_ERROR_SUCCESS;
    }

    return UTF_ERROR_INVALID_ARGUMENT;
  }

  if (ub->in.enc != UTF_8) {
    // Mid-codepoint encoding switch.
    ub->in = (ub_inbuf_t){ 0 };
    return UTF_ERROR_INVALID_ARGUMENT;
  }

  if (count != 1) {
    // Expected continuation byte.
    ub->in = (ub_inbuf_t){ 0 };
    return UTF_ERROR_INVALID_ARGUMENT;
  }

  size_t i;
  for (i = 1; ub->in.u8[i]; i++);
  UTF_RASSERT(i < 4);

  ub->in.u8[i] = byte;

  if (i+1 == ub->in.count) {
    ub_write_codepoint(ub);
  } else {
    UTF_RASSERT(i+1 < ub->in.count);
  }

  return UTF_ERROR_SUCCESS;
}

typedef enum {
  SURROGATE_NONE = 0,
  SURROGATE_HIGH = 1,
  SURROGATE_LOW = 2,
} surrogate_type_t;

surrogate_type_t get_surrogate(uint16_t cu)
{
  if (cu < 0xD800 || cu > 0xDFFF)
    return SURROGATE_NONE;

  return (cu <= 0xDBFF) ? SURROGATE_HIGH : SURROGATE_LOW;
}

utf_error_t utfbuf_write_utf16(utfbuf_t *ub, uint16_t cu)
{
  const surrogate_type_t surrogate = get_surrogate(cu);

  if (!ub->in.enc) {
    if (surrogate == SURROGATE_LOW) {
      // Expected NONE or HIGH.
      return UTF_ERROR_INVALID_ARGUMENT;
    }

    ub->in.u16[0] = cu;
    ub->in.count = 1 + !!surrogate;
    ub->in.enc = UTF_16;

    if (!surrogate) {
      ub_write_codepoint(ub);
    }

    return UTF_ERROR_SUCCESS;
  }

  if (surrogate != SURROGATE_LOW) {
    ub->in = (ub_inbuf_t){ 0 };
    return UTF_ERROR_INVALID_ARGUMENT;
  }

  ub->in.u16[1] = cu;
  ub_write_codepoint(ub);
  return UTF_ERROR_SUCCESS;
}

utf_error_t utfbuf_write_utf32(utfbuf_t *ub, uint32_t ch)
{
  if (ub->in.enc) {
    ub->in = (ub_inbuf_t){ 0 };
    return UTF_ERROR_INVALID_ARGUMENT;
  }

  ub->in.enc = UTF_32;
  ub->in.u32[0] = ch;
  ub_write_codepoint(ub);
  return UTF_ERROR_SUCCESS;
}

size_t utfbuf_overflow(const utfbuf_t *ub)
{
  return ub->overflow;
}
