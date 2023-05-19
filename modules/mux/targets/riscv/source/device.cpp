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

#include "riscv/device.h"

#include <cassert>

#include "riscv/device_info.h"
#include "riscv/hal.h"

mux_result_t riscvCreateDevices(uint64_t devices_length,
                                mux_device_info_t *device_infos,
                                mux_allocator_info_t allocator_info,
                                mux_device_t *out_devices) {
  // validate parameters
  if (devices_length == 0 || device_infos == nullptr) {
    return mux_error_invalid_value;
  }
  // access the hal
  hal::hal_t *hal = riscv::hal_get();
  if (!hal) {
    return mux_error_failure;
  }
  const auto &hal_info = hal->get_info();
  (void)hal_info;

  for (uint64_t i = 0; i < devices_length; ++i) {
    // access the derived riscv device info
    const riscv::device_info_t info =
        static_cast<riscv::device_info_t>(device_infos[i]);
    assert(info);
    // create our hal device
    assert(info->hal_device_index < hal_info.num_devices);
    hal::hal_device_t *hal_device = hal->device_create(info->hal_device_index);
    if (!hal_device) {
      return mux_error_failure;
    }
    // create our device
    mux::allocator allocator{allocator_info};
    riscv::device_s *rv_device =
        allocator.create<riscv::device_s>(info, allocator);
    if (nullptr == rv_device) {
      return mux_error_out_of_memory;
    }
    rv_device->hal = hal;
    rv_device->hal_device = hal_device;
    rv_device->profiler.setup_counters(*hal_device);
    const char *csv_path = std::getenv("CA_PROFILE_CSV_PATH");
    if (!csv_path) {
      csv_path = "/tmp/riscv.csv";
    }
    rv_device->profiler.set_output_path(csv_path);
    // save this output device
    out_devices[i] = rv_device;
  }
  return mux_success;
}

void riscvDestroyDevice(mux_device_t device,
                        mux_allocator_info_t allocator_info) {
  riscv::device_s *riscvDevice = static_cast<riscv::device_s *>(device);
  riscvDevice->profiler.write_summary();
  if (riscvDevice->hal && riscvDevice->hal_device) {
    riscvDevice->hal->device_delete(riscvDevice->hal_device);
    riscvDevice->hal_device = nullptr;
  }

  mux::allocator allocator(allocator_info);
  allocator.destroy(riscvDevice);
}
