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

/// @file
///
/// @brief

#ifndef UR_MUX_H_INCLUDED
#define UR_MUX_H_INCLUDED

#include <cstdio>
#include <cstdlib>

#include "cargo/attributes.h"
#include "mux/mux.h"
#include "ur_api.h"

namespace ur {
/// @brief Convert mux_result_t to appropriate ur_result_t.
///
/// @param[in] error The mux result to convert.
///
/// @return The unified runtime result which best maps to `error`.
inline ur_result_t resultFromMux(mux_result_t error) {
  switch (error) {
    case mux_success:
      return UR_RESULT_SUCCESS;
    case mux_error_failure:
      return UR_RESULT_ERROR_UNKNOWN;
    case mux_error_null_out_parameter:
      return UR_RESULT_ERROR_INVALID_NULL_POINTER;
    case mux_error_invalid_value:
      return UR_RESULT_ERROR_INVALID_VALUE;
    case mux_error_out_of_memory:
      return UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
    default:
      switch (error) {
        // TODO: There are no obvious mappings to these error codes so we abort,
        // this may be naughty but if you find yourself here the error code in
        // question may need special casing in the usage code or translated to a
        // more generic error code.
#define CASE(MUX_ENUM)                                                \
  case MUX_ENUM:                                                      \
    (void)std::fprintf(stderr, "ur::resultFromMux(" #MUX_ENUM         \
                               ") unknown mapping to ur_result_t\n"); \
    std::abort();
        CASE(mux_error_null_allocator_callback)
        CASE(mux_error_device_entry_hook_failed)
        CASE(mux_error_invalid_binary)
        CASE(mux_error_feature_unsupported)
        CASE(mux_error_missing_kernel)
        CASE(mux_error_internal)
        CASE(mux_error_fence_failure)
        CASE(mux_fence_not_ready)
#undef CASE
        default:
          (void)std::fprintf(
              stderr, "ur::resultFromMux(%d) unknown mux_result_t\n", error);
          std::abort();
      }
  }
}
}  // namespace ur

#endif  // UR_MUX_H_INCLUDED
