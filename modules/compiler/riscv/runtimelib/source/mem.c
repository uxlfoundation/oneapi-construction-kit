// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <stddef.h>

void *memset(void *ptr, int value, size_t num) {
  unsigned char *dst = ptr;
  while (num--) {
    *(dst++) = (unsigned char)value;
  }
  return ptr;
}

void *memcpy(void *restrict dst, const void *restrict src, size_t num) {
  unsigned char *d = dst;
  unsigned char *s = (unsigned char *)src;
  while (num--) {
    *(d++) = *(s++);
  }
  return dst;
}
