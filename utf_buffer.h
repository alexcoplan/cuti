#pragma once

#include <stdint.h>
#include <stddef.h>

#define UTFBUF_DEFINE_LOCAL(name, size, enc)\
  uint8_t name##_storage[size];\
  utfbuf_t name;\
  utfbuf_init(&name, name##_storage, size, enc);


typedef enum {
  UTF_ERROR_SUCCESS = 0,
  UTF_ERROR_FAILURE = 1,
  UTF_ERROR_INVALID_ARGUMENT = 2,
  UTF_ERROR_NOT_IMPLEMENTED = 3,
} utf_error_t;

typedef enum {
  UTF_ENC_NONE = 0,
  UTF_8  = 1,
  UTF_16 = 2, // LE
  UTF_32 = 3, // LE
} utf_enc_t;

typedef struct utfbuf utfbuf_t;

utf_error_t utfbuf_init(utfbuf_t *ub,
    void *mem, size_t mem_size, utf_enc_t encoding);

utf_error_t utfbuf_write_utf8(utfbuf_t *ub, uint8_t byte);

utf_error_t utfbuf_write_utf8_string(utfbuf_t *ub,
    const char *str);

utf_error_t utfbuf_write_utf32(utfbuf_t *ub, uint32_t ch);

size_t utfbuf_overflow(const utfbuf_t *ub);

// {{{ opaque

typedef struct {
  utf_enc_t enc;
  uint8_t count;
  union {
    uint8_t u8[4];
    uint16_t u16[2];
    uint32_t u32;
  };
} ub_inbuf_t;

struct utfbuf {
  // Fixed for the lifetime of the buffer.
  uint8_t *start;
  size_t pos;
  utf_enc_t enc;

  // Changed over the course of the buffer's lifetime.
  size_t size;
  size_t overflow;

  // Input buffer.
  ub_inbuf_t in;
};

// }}}

// vim: foldmethod=marker
