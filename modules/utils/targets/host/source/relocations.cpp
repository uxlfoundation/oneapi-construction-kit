// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <host/utils/relocations.h>
#include <utils/system.h>

#if !defined(NDEBUG) && !defined(_MSC_VER)
// 'nix debug builds do extra file checks
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Kernels can call library functions. The platform determines which library
// function calls LLVM might emit. The ELF resolver needs the addresses of these
// functions.
extern "C" {

#if defined(_MSC_VER)
// Windows uses chkstk() to ensure there is enough stack space paged in.
#if defined(UTILS_SYSTEM_64_BIT)
extern void __chkstk();
#else
extern void _chkstk();
#endif
#endif  // _MSC_VER

#if defined(UTILS_SYSTEM_32_BIT)
// On 32-bit (both x86 and Arm) long division is done in software.
extern void __divdi3();
extern void __udivdi3();
extern void __moddi3();
extern void __umoddi3();
#endif

#if defined(UTILS_SYSTEM_ARM) && defined(UTILS_SYSTEM_32_BIT)
// Arm uses these to do 64-bit integer division and modulo.
extern void __aeabi_ldivmod();
extern void __aeabi_uldivmod();
// Arm builds use these functions to convert between floats and longs
extern void __fixdfdi();
extern void __floatdidf();
extern void __floatdisf();
#endif
}

namespace {

#ifndef NDEBUG
void *dbg_memcpy(void *dest, const void *src, size_t count) {
  unsigned char *d = reinterpret_cast<unsigned char *>(dest);
  const unsigned char *s = reinterpret_cast<const unsigned char *>(src);

#if !defined(_MSC_VER)
  // On 'nix, check that the source is readable and the destination is writeable
  const int null_fd = open("/dev/null", O_WRONLY);
  const int zero_fd = open("/dev/zero", O_RDONLY);

  // Only read src if opened "/dev/null" successfully
  if (null_fd >= 0) {
    const size_t write_count = static_cast<size_t>(write(null_fd, src, count));
    close(null_fd);
    if (write_count != count) {
      (void)fprintf(stderr, "memcpy (called from kernel) out-of-bounds read\n");
      std::abort();
    }
  }

  // Only write zeros if opening "/dev/zero" was successful
  if (zero_fd >= 0) {
    const size_t read_count = static_cast<size_t>(read(zero_fd, dest, count));
    close(zero_fd);
    if (read_count != count) {
      (void)fprintf(stderr,
                    "memcpy (called from kernel) out-of-bounds write\n");
      std::abort();
    }
  }
#endif  // !_MSC_VER

  for (size_t i = 0; i < count; ++i) {
    d[i] = s[i];
  }
  return dest;
}

void *dbg_memset(void *dest, int ch, size_t count) {
  const unsigned char c = static_cast<unsigned char>(ch);
  unsigned char *d = reinterpret_cast<unsigned char *>(dest);

#if !defined(_MSC_VER)
  // On 'nix, check that the destination is writeable
  const int zero_fd = open("/dev/zero", O_RDONLY);

  // Only write zeros if opening "/dev/zero" was successful
  if (zero_fd >= 0) {
    const size_t cnt = static_cast<size_t>(read(zero_fd, dest, count));
    close(zero_fd);
    if (count != cnt) {
      (void)fprintf(stderr,
                    "memset (called from kernel) out-of-bounds write\n");
      std::abort();
    }
  }
#endif  // !_MSC_VER

  for (size_t i = 0; i < count; ++i) {
    d[i] = c;
  }
  return dest;
}
#endif  // NDEBUG
}  // namespace

namespace host {
namespace utils {

std::vector<std::pair<std::string, uint64_t>> getRelocations() {
  return {{
#ifndef NDEBUG
      {"memcpy", reinterpret_cast<uint64_t>(&dbg_memcpy)},
      {"memset", reinterpret_cast<uint64_t>(&dbg_memset)},
#else
      {"memcpy", reinterpret_cast<uint64_t>(&memcpy)},
      {"memset", reinterpret_cast<uint64_t>(&memset)},
#endif  // NDEBUG
      {"memmove", reinterpret_cast<uint64_t>(&memmove)},

#if defined(_MSC_VER)
#if defined(UTILS_SYSTEM_64_BIT)
      {"__chkstk", reinterpret_cast<uint64_t>(&__chkstk)},
#else
      {"_chkstk", reinterpret_cast<uint64_t>(&_chkstk)},
#endif  // UTILS_SYSTEM_64_BIT
#endif  // _MSC_VER

#if defined(UTILS_SYSTEM_32_BIT)
      {"__divdi3", reinterpret_cast<uint64_t>(&__divdi3)},
      {"__udivdi3", reinterpret_cast<uint64_t>(&__udivdi3)},
      {"__moddi3", reinterpret_cast<uint64_t>(&__moddi3)},
      {"__umoddi3", reinterpret_cast<uint64_t>(&__umoddi3)},
#endif  // defined(UTILS_SYSTEM_X86) && defined(UTILS_SYSTEM_32_BIT)

#if defined(UTILS_SYSTEM_ARM) && defined(UTILS_SYSTEM_32_BIT)
      // EABI combined div/mod helpers.
      {"__aeabi_ldivmod", reinterpret_cast<uint64_t>(&__aeabi_ldivmod)},
      {"__aeabi_uldivmod", reinterpret_cast<uint64_t>(&__aeabi_uldivmod)},
      // __fixdfdi and its EABI equivalent convert double to long.
      {"__fixdfdi", reinterpret_cast<uint64_t>(&__fixdfdi)},
      {"__aeabi_d2lz", reinterpret_cast<uint64_t>(&__fixdfdi)},
      // __floatdidf and its EABI equivalent convert long to double.
      {"__floatdidf", reinterpret_cast<uint64_t>(&__floatdidf)},
      {"__aeabi_l2d", reinterpret_cast<uint64_t>(&__floatdidf)},
      // __floatdisf and its EABI equivalent convert long to float.
      {"__floatdisf", reinterpret_cast<uint64_t>(&__floatdisf)},
      {"__aeabi_l2f", reinterpret_cast<uint64_t>(&__floatdisf)},
      // fminf and fmaxf are both used by the Arm32 backend when expanding
      // floating-point min/max reductions.
      {"fminf", reinterpret_cast<uint64_t>(&fminf)},
      {"fmaxf", reinterpret_cast<uint64_t>(&fmaxf)},
#endif  // defined(UTILS_SYSTEM_ARM) && defined(UTILS_SYSTEM_32_BIT)
  }};
}
}  // namespace utils
}  // namespace host
