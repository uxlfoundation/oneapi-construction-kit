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

#include <cstdint>

#ifndef _HAL_TRANSMITTER_H
#define _HAL_TRANSMITTER_H

namespace hal {

///  @brief A simple class interface that can be used for transmitting and
///  sending data - derive from this to support sockets, file descriptors etc.
class hal_transmitter {
 public:
  /// @brief Send `size` bytes of `data` with an optional flush
  /// @return true if the send succeeds
  virtual bool send(const void *data, uint32_t size, bool flush) = 0;

  /// Receive `size` bytes of data into `data`
  /// @return true if the receive succeeds
  virtual bool receive(void *data, uint32_t size) = 0;
  virtual ~hal_transmitter() {};

  void enable_debug(bool debug_enabled) { debug = debug_enabled; }
  bool debug_enabled() { return debug; }

 private:
  bool debug = false;
};
}  // namespace hal
#endif