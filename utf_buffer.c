#include "debug.h"
#include "minmax.h"
#include "utf_buffer.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define UTF_RASSERT(expr, ...)\
  do { \
    if (!(expr)) { \
      UTFDBG(1, "Assertion failed: " #expr ": " __VA_ARGS__); \
      abort(); \
    } \
  } while(0)

// TODO: disable in release build.
#define UTF_DASSERT(expr, ...) UTF_RASSERT(expr, __VA_ARGS__)

static uint8_t utf_bytes(utf_enc_t enc)
{
  return 1 << (enc - 1);
}

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
  case UTF_32:
    break; // TODO: UTF-16
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

static void ub_write_utf8_of_utf8(utfbuf_t *ub)
{
  UTF_RASSERT(ub->in.enc == UTF_8); // FIXME

  const uint8_t n = ub->in.count;
  const size_t avail = ub_bytes_remaining(ub);
  if (n <= avail) {
    if (ub->pos) {
      memcpy(ub->start + ub->pos - 1, ub->in.u8, n);
      ub->pos += n;
      ub->start[ub->pos - 1] = 0x0;
    }
  } else {
    ub->overflow += n - avail;
  }
}

// XXX: endianness!
static void write_utf32_internal(utfbuf_t *ub, uint32_t x)
{
  const size_t avail = ub_bytes_remaining(ub);
  if (avail >= 4) {
    if (ub->pos) {
      UTF_RASSERT(ub->pos >= 4);
      memcpy(ub->start + ub->pos - 4, &x, 4);
      memset(ub->start + ub->pos, 0x0, 4);
      ub->pos += 4;
    }
  } else {
    ub->overflow += 4 - avail;
  }
}

static void ub_write_utf32_of_utf8(utfbuf_t *ub)
{
  UTF_RASSERT(ub->in.enc == UTF_8); // FIXME

  const uint8_t *u8 = ub->in.u8;
  const uint8_t n = ub->in.count;

  uint32_t out = 0;
  uint8_t bits = 0;

  for (uint8_t i = n; i > 1; i--) {
    out |= (u8[i-1] & 0x3f) << bits;
    bits += 6;
  }

  static const uint8_t mask_table[] = {
    0x7f, 0x1f, 0x0f, 0x07,
  };

  out |= (u8[0] & mask_table[n-1]) << bits;

  write_utf32_internal(ub, out);
}

static void ub_write_codepoint(utfbuf_t *ub)
{
  switch (ub->enc) {
    case UTF_8:
      ub_write_utf8_of_utf8(ub);
      break;
    case UTF_32:
      ub_write_utf32_of_utf8(ub);
      break;
    default:
      UTF_RASSERT(0, "encoding %u", ub->enc);
  }

  ub->in = (ub_inbuf_t){ 0 };
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

size_t utfbuf_overflow(const utfbuf_t *ub)
{
  return ub->overflow;
}
