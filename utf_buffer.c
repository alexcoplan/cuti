#include "utf_buffer.h"
#include "minmax.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

static uint8_t utf_bytes(utf_enc_t enc)
{
  return 1 << (enc - 1);
}

static size_t ub_bytes_remaining(utfbuf_t *ub)
{
  return ub->size - ub->pos;
}

static void ub_write_pattern(utfbuf_t *ub,
    uint8_t pat, size_t n)
{
  const size_t rem = ub_bytes_remaining(ub);
  const size_t to_memset = min_zu(rem, n);

  if (to_memset) {
    memset(ub->start + ub->pos, pat, to_memset);
    ub->pos += to_memset;
  }

  ub->overflow += (n - to_memset);
}

static void ub_write_null(utfbuf_t *ub)
{
  ub_write_pattern(ub, 0x0, utf_bytes(ub->enc));
}

utf_error_t utfbuf_init(utfbuf_t *ub,
    void *mem, size_t mem_size, utf_enc_t encoding)
{
  switch (encoding) {
  case UTF_8:
    break; // only support UTF-8 for now
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

static void ub_write_utf8(utfbuf_t *ub)
{
  size_t n = 0;
  while (ub->in.u8[n])
    n++;

  UTF_DASSERT(n);

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

static void ub_write_codepoint(utfbuf_t *ub)
{
  switch (ub->enc) {
    case UTF_8:
      ub_write_utf8(ub);
      break;
    default:
      UTF_RASSERT(0, "encoding %u", ub->enc);
  }

  ub->in = (ub_inbuf_t){ 0 };
}

utf_error_t utfbuf_write_utf8(utfbuf_t *ub, uint8_t byte)
{
  if (ub->enc != UTF_8) {
    return UTF_ERROR_NOT_IMPLEMENTED;
  }

  if (!ub->in.enc) {
    if (!(byte & 0x80)) {
      ub->in.enc = UTF_8;
      ub->in.u8[0] = byte;
      ub_write_codepoint(ub);
      return UTF_ERROR_SUCCESS;
    }

    return UTF_ERROR_NOT_IMPLEMENTED;
  }

  return UTF_ERROR_NOT_IMPLEMENTED;
}

size_t utfbuf_overflow(const utfbuf_t *ub)
{
  return ub->overflow;
}
