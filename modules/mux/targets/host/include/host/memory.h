// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Host's memory interface.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef HOST_MEMORY_H_INCLUDED
#define HOST_MEMORY_H_INCLUDED

#include <mux/mux.h>

namespace host {
/// @addtogroup host
/// @{

struct memory_s : public mux_memory_s {
  enum heap_e : uint32_t {
    HEAP_ALL = 0x1 << 0,
    HEAP_BUFFER = 0x1 << 1,
    HEAP_IMAGE = 0x1 << 2,
  };

  memory_s(uint64_t size, uint32_t properties, void *data, bool useHost);

  void *data;
  bool useHost;
};

/// @}
}  // namespace host

#endif  // HOST_MEMORY_H_INCLUDED
