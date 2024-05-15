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

#ifndef _HAL_CLIENT_H
#define _HAL_CLIENT_H

#include <hal.h>
#include <hal_remote/hal_binary_decoder.h>
#include <hal_remote/hal_binary_encoder.h>
#include <hal_remote/hal_device_client.h>

#include <mutex>

namespace hal {

/// @brief Base class for hal_client, is a type of `hal::hal_t`, which can be
/// used to communicate with a remote server and will create `hal_device_client`
/// when requested via requests to the server.
/// @note It currently has the limitation of only supporting a single device.
class hal_client : public hal::hal_t {
 protected:
  std::mutex lock;
  bool made_connection = false;
  hal::hal_info_t hal_info;
  hal::hal_device_info_t *hal_device_info;
  uint16_t required_port;

 public:
  hal_client() {}
  virtual bool make_connection() = 0;
  virtual hal::hal_transmitter &get_transmitter() = 0;
  // request the creation of a new hal
  hal::hal_device_t *device_create(uint32_t index) override {
    const std::lock_guard<std::mutex> locker(lock);
    // Before we create a device check that the endianness described by the info
    // matches our own, as the data protocol assumes that endianness matches
    const uint32_t one = 1U;
    bool is_little_endian = reinterpret_cast<const char *>(&one)[0] == 1;
    assert(is_little_endian == hal_device_info->is_little_endian);

    if (is_little_endian != hal_device_info->is_little_endian) {
      return nullptr;
    }
    if (index > 0) {
      return nullptr;
    }

    if (!made_connection) {
      if (!make_connection()) {
        return nullptr;
      }
      made_connection = true;
    }

    hal::hal_binary_encoder encoder(index);
    hal::hal_binary_decoder decoder;
    hal::hal_binary_encoder::COMMAND command;
    encoder.encode_device_create();
    get_transmitter().send(encoder.data(), encoder.size(), true);
    bool ok = get_transmitter().receive(
        &command, sizeof(hal::hal_binary_encoder::COMMAND));
    if (command == hal::hal_binary_encoder::COMMAND::DEVICE_CREATE_REPLY) {
      const uint32_t data_required = decoder.decode_command_data_required(
          hal::hal_binary_encoder::COMMAND::DEVICE_CREATE_REPLY);
      std::vector<uint8_t> payload(data_required);

      ok = ok && get_transmitter().receive(payload.data(), data_required);
      if (!ok) {
        return nullptr;
      }
      if (decoder.decode(command, payload.data(), payload.size())) {
        if (decoder.message.device_create_reply) {
          return new hal::hal_device_client(hal_device_info, lock,
                                            &get_transmitter());
        }
      }
    }
    return nullptr;
  }

  // destroy a device instance
  bool device_delete(hal::hal_device_t *device) override {
    if (!made_connection) {
      made_connection = true;

      if (make_connection()) {
        return false;
      }
    }

    hal::hal_binary_encoder encoder(0);  // Assume is first device
    hal::hal_binary_decoder decoder;
    hal::hal_binary_encoder::COMMAND command;
    encoder.encode_device_delete();  // Assume is first device
    get_transmitter().send(encoder.data(), encoder.size(), true);
    bool ok = get_transmitter().receive(
        &command, sizeof(hal::hal_binary_encoder::COMMAND));
    if (command == hal::hal_binary_encoder::COMMAND::DEVICE_DELETE_REPLY) {
      const uint32_t data_required = decoder.decode_command_data_required(
          hal::hal_binary_encoder::COMMAND::DEVICE_DELETE_REPLY);
      std::vector<uint8_t> payload(data_required);
      ok = ok && get_transmitter().receive(payload.data(), data_required);
      ok = ok && decoder.decode(command, payload.data(), payload.size());
      ok = ok && decoder.message.device_delete_reply;
    }
    delete static_cast<hal::hal_device_client *>(device);
    return ok;
  }
};
}  // namespace hal
#endif
