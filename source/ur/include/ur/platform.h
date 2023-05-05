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

#ifndef UR_PLATFORM_H_INCLUDED
#define UR_PLATFORM_H_INCLUDED

#include "cargo/expected.h"
#include "cargo/small_vector.h"
#include "compiler/context.h"
#include "compiler/loader.h"
#include "ur/device.h"
#include "ur_api.h"

/// @brief Compute Mux specific implementation of the opaque
/// ur_platform_handle_t_ API object.
struct ur_platform_handle_t_ {
  ur_platform_handle_t_() = default;
  ur_platform_handle_t_(const ur_platform_handle_t_ &) = delete;

  /// @brief Global instance of the platform.
  static ur_platform_handle_t instance;
  /// @brief allocator used to allocate and free memory.
  mux_allocator_info_t mux_allocator_info = {
      [](void *, size_t size, size_t alignment) -> void * {
        return cargo::alloc(size, alignment);
      },
      [](void *, void *pointer) -> void { cargo::free(pointer); },
      nullptr,
  };
  /// @brief Handle on the compiler library used by the platform to compile
  /// modules of spir-v.
  std::unique_ptr<compiler::Library> compiler_library;
  /// @brief Handle on the compiler context used to manage compiler resources.
  std::unique_ptr<compiler::Context> compiler_context;
  /// @brief Devices belonging to this platform.
  cargo::small_vector<ur_device_handle_t_, 4> devices;
};

#endif  // UR_PLATFORM_H_INCLUDED
