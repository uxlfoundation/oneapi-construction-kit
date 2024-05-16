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

#ifndef _HAL_SERVER_H
#define _HAL_SERVER_H

#include <hal_remote/hal_binary_encoder.h>
#include <hal_remote/hal_transmitter.h>

#include <vector>

namespace hal {
class hal_t;
class hal_device_t;

/// @brief A server for taking binary data and translating into actions
/// on a HAL and HAL devices
/// @note This uses an abstract class `hal_transmitter` which can be used to
/// read and write the binary data. This transmitter may use sockets for example
/// but other approaches are possible. It is designed to only talk to one end
/// point and it is expected that end point will only make a single request at a
/// time and wait for the result. All requests to the server take a 4 byte
/// command (which maps the hal_binary_encoder::COMMAND), and a 4 byte device
/// id. The actual command will dictate how many more bytes are required using
/// the hal_binary_decoder.
///
/// The actual command will be processed will be done in process_command.
/// This will only process that first 8 bytes before calling one of the
/// process_* functions, which are expected to fetch the rest, all of which can
/// be overridden This takes a hal as an input, but is currently only able to
/// handle hals which support a single device. This could be remedied by storing
/// more than a single `hal_device` - and mapping indexes to devices and
/// vice-versa
///
/// `process_commands()` provides a method of repeatedly calling
/// `process_command()` until an error condition happens.
///

class hal_server {
 public:
  enum error_code {
    status_success,
    status_transmitter_failed,
    status_device_not_supported,
    status_unknown_command,
    status_decode_failed
  };
  hal_server(hal::hal_transmitter *transmitter, hal::hal_t *hal);
  ~hal_server();

  virtual error_code process_commands();

 protected:
  virtual error_code process_command();
  virtual hal_server::error_code process_mem_alloc(uint32_t device);
  virtual hal_server::error_code process_mem_free(uint32_t device);
  virtual hal_server::error_code process_mem_write(uint32_t device);
  virtual hal_server::error_code process_mem_read(uint32_t device);
  virtual hal_server::error_code process_mem_fill(uint32_t device);
  virtual hal_server::error_code process_mem_copy(uint32_t device);
  virtual hal_server::error_code process_program_free(uint32_t device);
  virtual hal_server::error_code process_find_kernel(uint32_t device);
  virtual hal_server::error_code process_program_load(uint32_t device);
  virtual hal_server::error_code process_kernel_exec(uint32_t device);
  virtual hal_server::error_code process_device_create(uint32_t device);
  virtual hal_server::error_code process_device_delete(uint32_t device);

  error_code receive_payload(hal_binary_encoder::COMMAND command,
                             std::vector<uint8_t> &payload);
  void *receive_data(uint32_t size);

  hal::hal_t *hal;
  hal::hal_device_t *hal_device = nullptr;
  hal_transmitter *transmitter;
  std::vector<uint8_t> payload;
  bool debug = false;
};
}  // namespace hal

#endif
