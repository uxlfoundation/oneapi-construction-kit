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

#ifndef UR_KERNEL_H_INCLUDED
#define UR_KERNEL_H_INCLUDED

#include <unordered_map>

#include "cargo/expected.h"
#include "cargo/string_view.h"
#include "compiler/module.h"
#include "mux/mux.h"
#include "ur/base.h"

/// @brief Compute Mux specific implementation of the opaque
/// ur_kernel_handle_t_ API object.
struct ur_kernel_handle_t_ : ur::base {
  struct argument_data_t {
    struct {
      char *data;
      size_t size;
    } value;
    ur_mem_handle_t mem_handle;
  };

  /// @brief Constructor to construct kernel.
  ///
  /// @param[in] program Program to create the kernel.
  /// @param[in] kernel_name Name of the kernel in `program`.
  ur_kernel_handle_t_(ur_program_handle_t program,
                      cargo::string_view kernel_name)
      : program(program), kernel_name(kernel_name) {}
  ur_kernel_handle_t_(const ur_kernel_handle_t_ &) = delete;
  ur_kernel_handle_t_ &operator=(const ur_kernel_handle_t_ &) = delete;
  ~ur_kernel_handle_t_();

  /// @brief Factory method for creating kernel objects.
  ///
  /// @param[in] program Program to create the kernel from.
  /// @param[in] kernel_name Name of the kernel in `program` to create.
  ///
  /// @return Kernel object or an error code if something went wrong.
  static cargo::expected<ur_kernel_handle_t, ur_result_t> create(
      ur_program_handle_t program, cargo::string_view kernel_name);

  /// @brief Program from which this program was created.
  ur_program_handle_t program = nullptr;
  /// @brief The name of the kernel in the source.
  cargo::string_view kernel_name;
  /// @brief The arguments to the kernel in the order they appear.
  cargo::dynamic_array<argument_data_t> arguments;
  /// @brief Device specific kernel map, one for each device in the context
  /// increasing in the order of the devices in the context.
  std::unordered_map<ur_device_handle_t, mux_kernel_t> device_kernel_map;
};

#endif  // UR_KERNEL_H_INCLUDED
