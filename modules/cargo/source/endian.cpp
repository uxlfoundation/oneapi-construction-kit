// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cargo/endian.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

uint8_t cargo::byte_swap(uint8_t v) { return v; }

uint16_t cargo::byte_swap(uint16_t v) {
#ifdef _MSC_VER
  return _byteswap_ushort(v);
#else
  return ((v >> 8) & 0x00FF) | ((v << 8) & 0xFF00);
#endif
}

uint32_t cargo::byte_swap(uint32_t v) {
#ifdef _MSC_VER
  return _byteswap_ulong(v);
#elif defined(__GNUC__)
  return __builtin_bswap32(v);
#else
  return ((v >> 24) & 0x000000FF) | ((v >> 8) & 0x0000FF00) |
         ((v << 8) & 0x00FF0000) | ((v << 24) & 0xFF000000);
#endif
}

uint64_t cargo::byte_swap(uint64_t v) {
#ifdef _MSC_VER
  return _byteswap_uint64(v);
#elif defined(__GNUC__)
  return __builtin_bswap64(v);
#else
  return ((v >> 56) & 0xFFULL) | ((v >> 40) & (0xFFULL << 8)) |
         ((v >> 24) & (0xFFULL << 16)) | ((v >> 8) & (0xFFULL << 24)) |
         ((v << 8) & (0xFFULL << 32)) | ((v << 24) & (0xFFULL << 40)) |
         ((v << 40) & (0xFFULL << 48)) | ((v << 56) & (0xFFULL << 56));
#endif
}
