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

#ifndef _HAL_SOCKET_CLIENT_H
#define _HAL_SOCKET_CLIENT_H

#include <mutex>

#include "hal.h"
#include "hal_remote/hal_client.h"
#include "hal_remote/hal_socket_transmitter.h"

namespace hal {

/// @brief A simple socket based version of `hal_client`.
class hal_socket_client : public hal::hal_client {
 protected:
  hal::hal_socket_transmitter transmitter;
  uint16_t required_port;

 public:
  hal_socket_client(uint16_t port) : hal_client() { required_port = port; }

  void set_port(uint16_t port) { required_port = port; }

  bool make_connection() {
    transmitter.set_port(required_port);
    const hal::hal_socket_transmitter::error_code res =
        transmitter.make_connection();
    if (res != hal::hal_socket_transmitter::success) {
      return false;
    }
    return true;
  }
  hal::hal_transmitter &get_transmitter() { return transmitter; }
};
}  // namespace hal
#endif
