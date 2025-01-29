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
/// riscv's device_info interface.

#ifndef RISCV_DEVICE_INFO_H_INCLUDED
#define RISCV_DEVICE_INFO_H_INCLUDED

#include <hal.h>
#include <hal_library.h>
#include <mux/mux.h>

#include <array>

namespace riscv {
/// @addtogroup riscv
/// @{

struct device_info_s final : public mux_device_info_s {
  /// @brief constructor.
  device_info_s();

  /// @brief update device info from a hal_device_info.
  void update_from_hal_info(const hal::hal_device_info_t *);

  /// @brief returns true if this device_info has been initialized.
  bool is_valid() const { return valid; }

  /// @brief hal_device_info object used to fill out this device_info.
  const hal::hal_device_info_t *hal_device_info;

  /// @brief the hal index coresponding to this device.
  uint32_t hal_device_index;

  /// @brief true if this is an initialized device_info.
  bool valid;
};

/// @brief query the hal and update `device_infos` to reflect the available
/// devices.
bool enumerate_device_infos();

typedef device_info_s *device_info_t;
/// @}
}  // namespace riscv

#endif  // RISCV_DEVICE_INFO_H_INCLUDED
