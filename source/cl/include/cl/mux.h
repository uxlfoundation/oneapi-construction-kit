// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
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
/// @brief Utilities to interface with the Mux API.

#ifndef CL_MUX_H_INCLUDED
#define CL_MUX_H_INCLUDED

#include <CL/cl.h>
#include <compiler/result.h>
#include <mux/mux.h>

namespace cl {
/// @brief Get an OpenCL error code from a Mux error code.
///
/// @param mux_result The mux error code to translate.
///
/// @return Returns an OpenCL error code related to the Mux error code.
inline cl_int getErrorFrom(mux_result_t mux_result) {
  switch (mux_result) {
  case mux_success:
    return CL_SUCCESS;
  case mux_error_feature_unsupported:
  case mux_error_internal:
    return CL_INVALID_OPERATION;
  case mux_error_null_out_parameter:
  case mux_error_invalid_value:
    return CL_INVALID_VALUE;
  case mux_error_out_of_memory:
    return CL_OUT_OF_HOST_MEMORY;
  case mux_error_device_entry_hook_failed:
    return CL_DEVICE_NOT_FOUND;
  case mux_error_invalid_binary:
    return CL_INVALID_BINARY;
  case mux_error_missing_kernel:
    return CL_INVALID_KERNEL_NAME;
  case mux_error_failure:
  case mux_error_null_allocator_callback:
  case mux_error_fence_failure:
  case mux_fence_not_ready:
  default:
    return CL_OUT_OF_RESOURCES;
  }
}

/// @brief Get an OpenCL error code from a compiler status code.
///
/// @param compiler_result The compiler status code to translate.
///
/// @return Returns an OpenCL error code related to the compiler status code.
inline cl_int getErrorFrom(compiler::Result compiler_result) {
  switch (compiler_result) {
  case compiler::Result::SUCCESS:
    return CL_SUCCESS;
  case compiler::Result::INVALID_VALUE:
    return CL_INVALID_VALUE;
  case compiler::Result::OUT_OF_MEMORY:
    return CL_OUT_OF_HOST_MEMORY;
  case compiler::Result::INVALID_BUILD_OPTIONS:
    return CL_INVALID_BUILD_OPTIONS;
  case compiler::Result::INVALID_COMPILER_OPTIONS:
    return CL_INVALID_COMPILER_OPTIONS;
  case compiler::Result::INVALID_LINKER_OPTIONS:
    return CL_INVALID_LINKER_OPTIONS;
  case compiler::Result::BUILD_PROGRAM_FAILURE:
    return CL_BUILD_PROGRAM_FAILURE;
  case compiler::Result::COMPILE_PROGRAM_FAILURE:
    return CL_COMPILE_PROGRAM_FAILURE;
  case compiler::Result::LINK_PROGRAM_FAILURE:
    return CL_LINK_PROGRAM_FAILURE;
  case compiler::Result::FINALIZE_PROGRAM_FAILURE:
    return CL_INVALID_PROGRAM;
  case compiler::Result::FAILURE:
  default:
    return CL_INVALID_OPERATION;
  }
}
} // namespace cl

#endif // CL_MUX_H_INCLUDED
