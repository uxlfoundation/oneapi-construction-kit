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

#include <mux/mux.h>
#include <riscv/device_info.h>
#include <riscv/device_info_get.h>
#include <riscv/hal.h>

#include <array>
#include <cassert>

namespace riscv {

bool enumerate_device_infos() {
  // if we have a valid device_info we have already enumerated and can return
  if (device_infos[0].is_valid()) {
    return true;
  }
  // load the hal library
  hal::hal_t *hal = riscv::hal_get();
  if (!hal) {
    return false;
  }
  const auto &hal_info = hal->get_info();
  // check we have something to enumerate
  if (hal_info.num_devices == 0) {
    return false;
  }
  // enumerate all reported devices
  uint32_t j = 0;
  for (uint32_t i = 0; i < hal_info.num_devices; ++i) {
    assert(i < device_infos.size());
    const auto *hal_dev_info = hal->device_get_info(i);
    // skip non riscv device types
    if (hal_dev_info->type != hal::hal_device_type_riscv) {
      continue;
    }
    // update this device_info entry and continue
    auto &dev_info = device_infos[j++];
    dev_info.update_from_hal_info(hal_dev_info);
    dev_info.hal_device_index = i;
    // device info should be valid at this point
    assert(dev_info.is_valid());
  }

  // success if we have at least one device
  return j > 0;
}

std::array<riscv::device_info_s, riscv::max_device_infos> device_infos;

cargo::array_view<riscv::device_info_s> GetDeviceInfosArray() {
  // ensure our device infos have been enumerated
  if (!riscv::enumerate_device_infos()) {
    return {};
  }
  return riscv::device_infos;
}

}  // namespace riscv
