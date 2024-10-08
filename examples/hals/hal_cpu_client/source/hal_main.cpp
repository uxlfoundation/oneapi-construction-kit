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

#include <cpu_hal.h>
#include <hal_remote/hal_socket_client.h>

namespace hal {
class hal_cpu_socket_client : public hal::hal_socket_client {
 public:
  hal_cpu_socket_client() : hal_socket_client(0) {
    static constexpr uint32_t implemented_api_version = 6;
    static_assert(
        implemented_api_version == hal_t::api_version,
        "Implemented API version for hal_socket_client does not match hal.h");
    hal_device_info = &cpu_hal::setup_cpu_hal_device_info();
    hal_info.platform_name = hal_device_info->target_name;
    hal_info.num_devices = 1;
    hal_info.api_version = implemented_api_version;
    uint32_t port = 0;
    if (const char *env = getenv("HAL_REMOTE_PORT")) {
      port = atoi(env);
    }
    set_port(port);
  }

  // return generic platform information
  const hal::hal_info_t &get_info() override { return hal_info; }

  // return generic target information
  const hal::hal_device_info_t *device_get_info(uint32_t index) override {
    return hal_device_info;
  }
};
}  // namespace hal
static hal::hal_cpu_socket_client hal_object;

hal::hal_t *get_hal(uint32_t &api_version) {
  api_version = hal_object.get_info().api_version;
  return &hal_object;
}
