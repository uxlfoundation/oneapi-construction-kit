// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
/// Host's buffer interface.
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef HOST_BUFFER_H_INCLUDED
#define HOST_BUFFER_H_INCLUDED

#include <mux/mux.h>

namespace host {
/// @addtogroup host
/// @{

struct buffer_s final : public mux_buffer_s {
  buffer_s(mux_memory_requirements_s memory_requirements);

  void *data;
};

/// @}
}  // namespace host

#endif  // HOST_BUFFER_H_INCLUDED
