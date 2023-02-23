// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "arg_pack.h"

#include <cassert>
#include <cstring>

#include "hal_types.h"

namespace hal {
/// @addtogroup hal
/// @{

namespace util {
/// @addtogroup util
/// @{

namespace {
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
  // current argument write point
  uint64_t write_point = uint64_t(pack.size());
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
          expand(sizeof(arg.address) + padding);
          write_point += padding;
          memcpy(pack.data() + write_point, &arg.address, sizeof(arg.address));
          break;
        }
        case hal_arg_value: {
          // copy POD data in directly
          const uint64_t alignSize = round_up_power_2(arg.size);
          const uint64_t padding = calc_padding(write_point, alignSize);
          write_point += padding;
          expand(arg.size + padding);
          memcpy(pack.data() + write_point, arg.pod_data, arg.size);
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
          if (word_size == 64) {
            // for RV64 devices, size is 64 bits
            const uint64_t size = uint64_t(arg.size);
            const uint64_t padding = calc_padding(write_point, sizeof(size));
            write_point += padding;
            expand(sizeof(size) + padding);
            memcpy(pack.data() + write_point, &size, sizeof(size));
          } else if (word_size == 32) {
            // for RV32 devices, size is 32 bits
            const uint32_t size = uint32_t(arg.size);
            const uint64_t padding = calc_padding(write_point, sizeof(size));
            write_point += padding;
            expand(sizeof(size) + padding);
            memcpy(pack.data() + write_point, &size, sizeof(size));
          } else {
            assert(!"Not implemented");
            return false;
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
  // a simple heuristic to cut down on reallocations during resize
  pack.reserve(num_args * sizeof(uint64_t));
  // iterate over all of the argument descriptors
  for (uint32_t i = 0; i < num_args; ++i) {
    if (!add_arg(args[i])) {
      return false;
    }
  }
  // success
  return true;
}

/// @}
}  // namespace util
/// @}
}  // namespace hal
