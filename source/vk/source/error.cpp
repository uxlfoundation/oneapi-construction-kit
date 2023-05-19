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

#include <vk/error.h>

#include <string>

namespace vk {

VkResult getVkResult(mux_result_t error) {
  // TODO: more complete mapping between mux_result and VkResult, see CA-3182.
  switch (error) {
    case mux_error_feature_unsupported:
      return VK_ERROR_FEATURE_NOT_PRESENT;
    case mux_error_out_of_memory:
    case mux_error_device_entry_hook_failed:
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    case mux_error_failure:
    case mux_error_invalid_value:
      return VK_ERROR_INITIALIZATION_FAILED;
    default:
      VK_ABORT(
          ("Unknown mux_result_t " + std::to_string(static_cast<int>(error)))
              .c_str());
  }
}

VkResult getVkResult(compiler::Result error) {
  switch (error) {
    case compiler::Result::OUT_OF_MEMORY:
      return VK_ERROR_OUT_OF_HOST_MEMORY;
    case compiler::Result::INVALID_BUILD_OPTIONS:
    case compiler::Result::INVALID_COMPILER_OPTIONS:
    case compiler::Result::INVALID_LINKER_OPTIONS:
    case compiler::Result::BUILD_PROGRAM_FAILURE:
    case compiler::Result::COMPILE_PROGRAM_FAILURE:
    case compiler::Result::LINK_PROGRAM_FAILURE:
    case compiler::Result::FINALIZE_PROGRAM_FAILURE:
      return VK_ERROR_INITIALIZATION_FAILED;
    default:
      VK_ABORT(("Unknown compiler::Result " +
                std::to_string(static_cast<int>(error)))
                   .c_str());
  }
}

}  // namespace vk
