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

#ifndef UR_DEVICE_H_INCLUDED
#define UR_DEVICE_H_INCLUDED

#include <compiler/info.h>
#include <compiler/module.h>
#include <compiler/target.h>

#include "mux/mux.h"
#include "ur_api.h"

/// @brief Compute Mux specific implementation of the opaque
/// ur_device_handle_t_ API object.
struct ur_device_handle_t_ {
  /// @brief Constructor for creating device.
  ///
  /// @param[in] platform The platform to which the device should belong.
  /// @param[in] mux_device Underlying mux device for the target.
  /// @param[in] compiler_info Information about the compiler for this mux
  /// device.
  /// @param[in] target The compiler target for this device.
  /// @param[in] spv_device_info spirv device info for this target.
  ur_device_handle_t_(ur_platform_handle_t platform, mux_device_t mux_device,
                      const compiler::Info *compiler_info,
                      std::unique_ptr<compiler::Target> target,
                      compiler::spirv::DeviceInfo spv_device_info);
  ur_device_handle_t_(ur_device_handle_t_ &&) = default;
  ur_device_handle_t_(const ur_device_handle_t_ &) = delete;
  ur_device_handle_t_ &operator=(const ur_device_handle_t_ &) = delete;
  ~ur_device_handle_t_();

  /// @brief Query if the device supports host allocations.
  ///
  /// @returns true or false if the device supports host allocations or not.
  bool supportsHostAllocations() const;

  /// @brief Platform to which this device belongs.
  ur_platform_handle_t platform = nullptr;
  /// @brief Underlying mux device for the target.
  mux_device_t mux_device = nullptr;
  /// @brief Compiler info for this target.
  const compiler::Info *compiler_info = nullptr;
  /// @brief Compiler target for this device.
  std::unique_ptr<compiler::Target> target = nullptr;
  /// @brief Spir-v device info for this target.
  compiler::spirv::DeviceInfo spv_device_info;
};

#endif  // UR_DEVICE_H_INCLUDED
