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

#include "arg_pack.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "hal_types.h"

namespace hal {
/// @addtogroup hal
/// @{

namespace util {
/// @addtogroup util
/// @{

namespace {
// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
// This will round up to power of 2 for all values except 0, or above 2^63 which
// will result in 0. This is good enough for our purposes where it is based on
// a non-zero size value.
inline uint64_t round_up_power_2(uint64_t v) {
  --v;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v |= v >> 32;
  return ++v;
}

inline bool is_power_2(uint64_t v) { return 0 == (v & (v - 1)); }

inline uint64_t round_to_multiple_p2(uint64_t val, uint64_t mult) {
  assert(is_power_2(mult) && "mult must be a power of two");
  return (val + (mult - 1)) & ~(mult - 1);
}

inline uint64_t calc_padding(uint64_t write_point, uint64_t align_size) {
  const uint64_t alignedIndex = round_to_multiple_p2(write_point, align_size);
  return alignedIndex - write_point;
}
}  // namespace

bool hal_argpack_t::add_arg(const hal_arg_t &arg) {
  // dispatch based on arg type
  switch (arg.space) {
    case hal_space_global:
      switch (arg.kind) {
        case hal_arg_address: {
          // copy the address in directly - we don't worry about rounding to
          // power of 2 as size is already a power of 2
          assert(is_power_2(sizeof(arg.address)));
          const uint64_t padding =
              calc_padding(write_point, sizeof(arg.address));
          if (!expand(sizeof(arg.address) + padding)) {
            return false;
          }
          write_point += padding;
          std::memcpy(pack + write_point, &arg.address, sizeof(arg.address));
          write_point += sizeof(arg.address);
          break;
        }
        case hal_arg_value: {
          // copy POD data in directly
          const uint64_t alignSize = round_up_power_2(arg.size);
          const uint64_t padding = calc_padding(write_point, alignSize);
          write_point += padding;
          if (!expand(arg.size + padding)) {
            return false;
          }
          std::memcpy(pack + write_point, arg.pod_data, arg.size);
          write_point += arg.size;
          break;
        }
        default:
          assert(!"Not implemented");
          return false;
      }
      break;
    case hal_space_local:
      switch (arg.kind) {
        case hal_arg_address:
          if (wg_mode) {
            if (word_size_in_bits == 64) {
              // for RV64 devices, size is 64 bits
              const uint64_t size = uint64_t(arg.size);
              const uint64_t padding = calc_padding(write_point, sizeof(size));
              if (!expand(sizeof(size) + padding)) {
                return false;
              }
              write_point += padding;
              std::memcpy(pack + write_point, &size, sizeof(size));
              write_point += sizeof(size);
            } else if (word_size_in_bits == 32) {
              // for RV32 devices, size is 32 bits
              const uint32_t size = uint32_t(arg.size);
              const uint64_t padding = calc_padding(write_point, sizeof(size));
              if (!expand(sizeof(size) + padding)) {
                return false;
              }
              write_point += padding;
              std::memcpy(pack + write_point, &size, sizeof(size));
              write_point += sizeof(size);
            } else {
              assert(!"Not implemented");
              return false;
            }
          } else {
            if (!expand(word_size_in_bits / 8)) {
              return false;
            }
            if (word_size_in_bits == 32) {
              *(reinterpret_cast<uint32_t *>(pack + write_point)) =
                  local_current_ptr;
            } else {
              *(reinterpret_cast<uint64_t *>(pack + write_point)) =
                  local_current_ptr;
            }
            write_point += word_size_in_bits / 8;
            local_current_ptr += arg.size;
            if (local_current_ptr >= local_start + local_size) {
              return false;
            }
          }
          break;
        default:
          assert(!"Not implemented");
          return false;
      }
      break;
    default:
      assert(!"Not implemented");
      return false;
  }
  return true;
}

bool hal_argpack_t::build(const hal_arg_t *args, uint32_t num_args) {
  // clear the argument pack
  clear();

  for (uint32_t i = 0; i < num_args; ++i) {
    if (!add_arg(args[i])) {
      return false;
    }
  }
  // success
  return true;
}

hal_argpack_t::~hal_argpack_t() {
  if (pack) {
    std::free(pack);
  }
}

void hal_argpack_t::clear() {
  write_point = 0;
  local_current_ptr = local_start;
}

bool hal_argpack_t::expand(size_t num_bytes) {
  constexpr uint32_t alignment = 128;
  constexpr uint32_t chunk_size = 128;

  if (!pack || (write_point + num_bytes > pack_alloc_size)) {
    // Allocate in chunks so we don't continually reallocate
    pack_alloc_size =
        ((write_point + num_bytes) + chunk_size - 1) / chunk_size * chunk_size;
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
    // Visual Studio and MinGW do not properly support std::aligned_alloc
    uint8_t *replacement_pack =
        static_cast<uint8_t *>(_aligned_malloc(pack_alloc_size, alignment));
#else
    uint8_t *replacement_pack =
        static_cast<uint8_t *>(std::aligned_alloc(alignment, pack_alloc_size));
#endif
    if (!replacement_pack) {
      return false;
    }
    if (pack) {
      if (write_point) {
        // copy from old to new and free old one
        std::memcpy(replacement_pack, pack, write_point);
      }
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
      _aligned_free(pack);
#else
      std::free(pack);
#endif
    }
    pack = replacement_pack;
  }
  return true;
}

/// @}
}  // namespace util
/// @}
}  // namespace hal
