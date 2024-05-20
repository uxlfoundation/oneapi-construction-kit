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

#include <assert.h>
#include <hal.h>
#include <hal_remote/hal_binary_decoder.h>
#include <hal_remote/hal_binary_encoder.h>
#include <hal_remote/hal_server.h>

#include <cstring>
#include <iostream>

namespace hal {
hal_server::hal_server(hal_transmitter *transmitter, hal::hal_t *hal)
    : hal(hal), transmitter(transmitter) {
  if (getenv("HAL_DEBUG_SERVER") && *getenv("HAL_DEBUG_SERVER") == '1') {
    debug = true;
  }
}

hal_server::~hal_server() {
  if (hal_device) {
    hal->device_delete(hal_device);
  }
}

void *hal_server::receive_data(uint32_t size) {
  payload.resize(size);
  if (!transmitter->receive(payload.data(), size)) {
    return nullptr;
  }
  return payload.data();
}

hal_server::error_code hal_server::process_commands() {
  hal_server::error_code ret = hal_server::status_success;
  do {
    ret = process_command();
  } while (ret == hal::hal_server::status_success);
  return ret;
}

hal_server::error_code hal_server::receive_payload(
    hal_binary_encoder::COMMAND command, std::vector<uint8_t> &payload) {
  hal_binary_decoder decoder;
  const uint32_t data_required = decoder.decode_command_data_required(command);
  if (data_required == hal_binary_decoder::data_required_unknown) {
    return hal_server::status_decode_failed;
  }
  payload.resize(data_required);
  bool receive_ok = true;
  if (data_required) {
    receive_ok = transmitter->receive(payload.data(), data_required);
  }
  if (!receive_ok) {
    return hal_server::status_transmitter_failed;
  } else {
    return hal_server::status_success;
  }
}

hal_server::error_code hal_server::process_mem_alloc(uint32_t device) {
  auto payload_status =
      receive_payload(hal_binary_encoder::COMMAND::MEM_ALLOC, payload);
  if (payload_status != hal_server::status_success) {
    return payload_status;
  }
  hal_binary_decoder decoder;
  const bool decoder_status = decoder.decode(
      hal_binary_encoder::COMMAND::MEM_ALLOC, payload.data(), payload.size());
  if (!decoder_status) {
    return hal::hal_server::status_decode_failed;
  }

  auto res = hal_device->mem_alloc(decoder.message.alloc.size,
                                   decoder.message.alloc.alignment);
  if (debug) {
    std::cerr << "Hal Server:: mem alloc " << decoder.message.alloc.size << " "
              << decoder.message.alloc.alignment << " -> " << res << "\n";
  }
  hal_binary_encoder encode_reply;
  encode_reply.encode_mem_alloc_reply(res);
  auto send_res =
      transmitter->send(encode_reply.data(), encode_reply.size(), true);
  if (!send_res) {
    return hal_server::status_transmitter_failed;
  }
  return hal_server::status_success;
}

hal_server::error_code hal_server::process_mem_free(uint32_t device) {
  hal_binary_decoder decoder;
  auto payload_status =
      receive_payload(hal_binary_encoder::COMMAND::MEM_FREE, payload);
  if (payload_status != hal_server::status_success) {
    return payload_status;
  }
  const bool decoder_status = decoder.decode(
      hal_binary_encoder::COMMAND::MEM_FREE, payload.data(), payload.size());
  if (!decoder_status) {
    return hal::hal_server::status_decode_failed;
  }

  auto res = hal_device->mem_free(decoder.message.free.addr);
  if (debug) {
    std::cerr << "Hal Server:: mem free " << decoder.message.free.addr << " -> "
              << res << "\n";
  }
  hal_binary_encoder encode_reply;
  encode_reply.encode_mem_free_reply(res);
  auto send_res =
      transmitter->send(encode_reply.data(), encode_reply.size(), true);
  if (!send_res) {
    return hal_server::status_transmitter_failed;
  }
  return hal_server::status_success;
}

hal_server::error_code hal_server::process_mem_write(uint32_t device) {
  auto payload_status =
      receive_payload(hal_binary_encoder::COMMAND::MEM_WRITE, payload);
  if (payload_status != hal_server::status_success) {
    return payload_status;
  }
  hal_binary_decoder decoder;
  const bool decoder_status = decoder.decode(
      hal_binary_encoder::COMMAND::MEM_WRITE, payload.data(), payload.size());
  if (!decoder_status) {
    return hal::hal_server::status_decode_failed;
  }
  const void *src_data = receive_data(decoder.message.write.size);
  if (!src_data) {
    return hal_server::status_transmitter_failed;
  }

  assert(src_data && "Src data must be non Null");
  auto res = hal_device->mem_write(decoder.message.write.dst, src_data,
                                   decoder.message.write.size);
  if (debug) {
    std::cerr << "Hal Server:: mem_write " << decoder.message.write.dst << " "
              << src_data << " " << decoder.message.write.size << " -> " << res
              << "\n";
  }

  hal_binary_encoder encode_reply;
  encode_reply.encode_mem_write_reply(res);
  auto send_res =
      transmitter->send(encode_reply.data(), encode_reply.size(), true);
  if (!send_res) {
    return hal_server::status_transmitter_failed;
  }
  return hal_server::status_success;
}

hal_server::error_code hal_server::process_mem_fill(uint32_t device) {
  hal_binary_decoder decoder;
  auto payload_status =
      receive_payload(hal_binary_encoder::COMMAND::MEM_FILL, payload);
  if (payload_status != hal_server::status_success) {
    return payload_status;
  }
  const bool decoder_status = decoder.decode(
      hal_binary_encoder::COMMAND::MEM_FILL, payload.data(), payload.size());
  if (!decoder_status) {
    return hal::hal_server::status_decode_failed;
  }
  const void *pattern_data = receive_data(decoder.message.fill.pattern_size);
  if (!pattern_data) {
    return hal::hal_server::status_transmitter_failed;
  }
  auto res = hal_device->mem_fill(decoder.message.fill.dst, pattern_data,
                                  decoder.message.fill.pattern_size,
                                  decoder.message.fill.size);
  hal_binary_encoder encode_reply;
  encode_reply.encode_mem_fill_reply(res);
  auto send_res =
      transmitter->send(encode_reply.data(), encode_reply.size(), true);
  if (!send_res) {
    return hal_server::status_transmitter_failed;
  }
  return hal_server::status_success;
}

hal_server::error_code hal_server::process_mem_read(uint32_t device) {
  hal_binary_decoder decoder;
  auto payload_status =
      receive_payload(hal_binary_encoder::COMMAND::MEM_READ, payload);
  if (payload_status != hal_server::status_success) {
    return payload_status;
  }
  const bool decoder_status = decoder.decode(
      hal_binary_encoder::COMMAND::MEM_READ, payload.data(), payload.size());
  if (!decoder_status) {
    return hal::hal_server::status_decode_failed;
  }
  std::vector<uint8_t> dst;
  dst.resize(decoder.message.read.size);
  auto res = hal_device->mem_read(dst.data(), decoder.message.read.src,
                                  decoder.message.read.size);
  if (debug) {
    std::cerr << "Hal Server:: mem_read " << dst.data() << " "
              << decoder.message.read.src << " " << decoder.message.read.size
              << " -> " << res << "\n";
  }
  hal_binary_encoder encode_reply;
  encode_reply.encode_mem_read_reply(res);
  auto send_res =
      transmitter->send(encode_reply.data(), encode_reply.size(), true);
  send_res = send_res &&
             transmitter->send(dst.data(), decoder.message.read.size, true);
  if (!send_res) {
    return hal_server::status_transmitter_failed;
  }
  return hal_server::status_success;
}

hal_server::error_code hal_server::process_mem_copy(uint32_t device) {
  hal_binary_decoder decoder;
  auto payload_status =
      receive_payload(hal_binary_encoder::COMMAND::MEM_COPY, payload);
  if (payload_status != hal_server::status_success) {
    return payload_status;
  }
  const bool decoder_status = decoder.decode(
      hal_binary_encoder::COMMAND::MEM_COPY, payload.data(), payload.size());
  if (!decoder_status) {
    return hal::hal_server::status_decode_failed;
  }
  auto res =
      hal_device->mem_copy(decoder.message.copy.dst, decoder.message.copy.src,
                           decoder.message.copy.size);
  hal_binary_encoder encode_reply;
  encode_reply.encode_mem_copy_reply(res);
  auto send_res =
      transmitter->send(encode_reply.data(), encode_reply.size(), true);

  if (!send_res) {
    return hal_server::status_transmitter_failed;
  }
  return hal_server::status_success;
}

hal_server::error_code hal_server::process_program_free(uint32_t device) {
  hal_binary_decoder decoder;
  auto payload_status =
      receive_payload(hal_binary_encoder::COMMAND::PROGRAM_FREE, payload);
  if (payload_status != hal_server::status_success) {
    return payload_status;
  }
  const bool decoder_status =
      decoder.decode(hal_binary_encoder::COMMAND::PROGRAM_FREE, payload.data(),
                     payload.size());
  if (!decoder_status) {
    return hal::hal_server::status_decode_failed;
  }
  auto res = hal_device->program_free(decoder.message.prog_free.program);
  if (debug) {
    std::cerr << "Hal Server:: program_free "
              << decoder.message.prog_free.program << " -> " << res << "\n";
  }
  hal_binary_encoder encode_reply;
  encode_reply.encode_program_free_reply(res);
  auto send_res =
      transmitter->send(encode_reply.data(), encode_reply.size(), true);

  if (!send_res) {
    return hal_server::status_transmitter_failed;
  }
  return hal_server::status_success;
}

hal_server::error_code hal_server::process_find_kernel(uint32_t device) {
  hal_binary_decoder decoder;
  auto payload_status =
      receive_payload(hal_binary_encoder::COMMAND::FIND_KERNEL, payload);
  if (payload_status != hal_server::status_success) {
    return payload_status;
  }
  const bool decoder_status = decoder.decode(
      hal_binary_encoder::COMMAND::FIND_KERNEL, payload.data(), payload.size());
  if (!decoder_status) {
    return hal::hal_server::status_decode_failed;
  }

  const void *name_data =
      receive_data(decoder.message.prog_find_kernel.kernel_name_size);
  if (!name_data) {
    return hal::hal_server::status_transmitter_failed;
  }

  auto res = hal_device->program_find_kernel(
      decoder.message.prog_find_kernel.program, (const char *)name_data);
  if (debug) {
    std::cerr << "Hal Server:: program_find_kernel "
              << decoder.message.prog_find_kernel.program << " "
              << (const char *)name_data << " -> " << res << "\n";
  }

  hal_binary_encoder encode_reply;
  encode_reply.encode_find_kernel_reply(res);
  auto send_res =
      transmitter->send(encode_reply.data(), encode_reply.size(), true);

  if (!send_res) {
    return hal_server::status_transmitter_failed;
  }
  return hal_server::status_success;
}

hal_server::error_code hal_server::process_program_load(uint32_t device) {
  hal_binary_decoder decoder;
  auto payload_status =
      receive_payload(hal_binary_encoder::COMMAND::PROGRAM_LOAD, payload);
  if (payload_status != hal_server::status_success) {
    return payload_status;
  }
  const bool decoder_status =
      decoder.decode(hal_binary_encoder::COMMAND::PROGRAM_LOAD, payload.data(),
                     payload.size());
  if (!decoder_status) {
    return hal::hal_server::status_decode_failed;
  }
  const void *data = receive_data(decoder.message.prog_load.size);
  if (!data) {
    return hal::hal_server::status_transmitter_failed;
  }

  auto res = hal_device->program_load(data, decoder.message.prog_load.size);
  if (debug) {
    std::cerr << "Hal Server:: program_load " << decoder.message.prog_load.size
              << " (" << data << ") -> " << res << "\n";
  }
  hal_binary_encoder encode_reply;
  encode_reply.encode_program_load_reply(res);
  auto send_res =
      transmitter->send(encode_reply.data(), encode_reply.size(), true);

  if (!send_res) {
    return hal_server::status_transmitter_failed;
  }
  return hal_server::status_success;
}

hal_server::error_code hal_server::process_kernel_exec(uint32_t device) {
  hal_binary_decoder decoder;
  auto payload_status =
      receive_payload(hal_binary_encoder::COMMAND::KERNEL_EXEC, payload);
  if (payload_status != hal_server::status_success) {
    return payload_status;
  }
  // Download and convert args
  const bool decoder_status = decoder.decode(
      hal_binary_encoder::COMMAND::KERNEL_EXEC, payload.data(), payload.size());
  if (!decoder_status) {
    return hal::hal_server::status_decode_failed;
  }
  void *data = nullptr;
  if (decoder.message.kernel_exec.args_data_size) {
    data = receive_data(decoder.message.kernel_exec.args_data_size);

    const bool receive_ok = decoder.decode_kernel_exec_args(
        data, decoder.message.kernel_exec.num_args,
        decoder.message.kernel_exec.args_data_size);
    if (!receive_ok) {
      return hal_server::status_transmitter_failed;
    }
  }

  auto res = hal_device->kernel_exec(
      decoder.message.kernel_exec.program, decoder.message.kernel_exec.kernel,
      &decoder.message.kernel_exec.nd_range, decoder.kernel_args.data(),
      decoder.message.kernel_exec.num_args,
      decoder.message.kernel_exec.work_dim);
  if (debug) {
    std::cerr << "Hal Server:: kernel_exec "
              << decoder.message.kernel_exec.args_data_size << " "
              << decoder.message.kernel_exec.program << " "
              << decoder.message.kernel_exec.kernel << " "
              << decoder.message.kernel_exec.nd_range.global[0] << " "
              << decoder.message.kernel_exec.nd_range.global[1] << " "
              << decoder.message.kernel_exec.nd_range.global[2] << " "
              << decoder.message.kernel_exec.nd_range.local[0] << " "
              << decoder.message.kernel_exec.nd_range.local[1] << " "
              << decoder.message.kernel_exec.nd_range.local[2] << " "
              << decoder.kernel_args.data() << " "
              << decoder.message.kernel_exec.num_args << " "
              << decoder.message.kernel_exec.work_dim << " -> " << res << "\n";
  }
  hal_binary_encoder encode_reply;
  encode_reply.encode_kernel_exec_reply(res);
  auto send_res =
      transmitter->send(encode_reply.data(), encode_reply.size(), true);

  if (!send_res) {
    return hal_server::status_transmitter_failed;
  }
  return hal_server::status_success;
}

hal_server::error_code hal_server::process_device_create(uint32_t device) {
  hal_binary_decoder decoder;
  auto payload_status =
      receive_payload(hal_binary_encoder::COMMAND::DEVICE_CREATE, payload);
  if (payload_status != hal_server::status_success) {
    return payload_status;
  }
  const bool decoder_status =
      decoder.decode(hal_binary_encoder::COMMAND::DEVICE_CREATE, payload.data(),
                     payload.size());
  if (!decoder_status) {
    return hal::hal_server::status_decode_failed;
  }
  hal_device = hal->device_create(device);
  if (debug) {
    std::cerr << "Hal Server:: device_create " << device << " -> " << hal_device
              << "\n";
  }
  hal_binary_encoder encode_reply;
  encode_reply.encode_device_create_reply(hal_device ? true : false);
  auto send_res =
      transmitter->send(encode_reply.data(), encode_reply.size(), true);

  if (!send_res) {
    return hal_server::status_transmitter_failed;
  }
  return hal_server::status_success;
}

hal_server::error_code hal_server::process_device_delete(uint32_t device) {
  hal_binary_decoder decoder;
  auto payload_status =
      receive_payload(hal_binary_encoder::COMMAND::DEVICE_DELETE, payload);
  if (payload_status != hal_server::status_success) {
    return payload_status;
  }
  // TODO - call delete with looked up device
  const bool decoder_status =
      decoder.decode(hal_binary_encoder::COMMAND::DEVICE_DELETE, payload.data(),
                     payload.size());
  if (!decoder_status) {
    return hal::hal_server::status_decode_failed;
  }
  hal->device_delete(hal_device);
  hal_device = nullptr;
  if (debug) {
    std::cerr << "Hal Server:: device_delete " << device << "\n";
  }
  hal_binary_encoder encode_reply;
  encode_reply.encode_device_delete_reply(true);
  auto send_res =
      transmitter->send(encode_reply.data(), encode_reply.size(), true);
  if (!send_res) {
    return hal_server::status_transmitter_failed;
  }
  return hal_server::status_success;
}

hal_server::error_code hal_server::process_command() {
  struct {
    hal_binary_encoder::COMMAND command;
    uint32_t device;
  } prefix;
  static_assert(sizeof(prefix) == 8);

  const bool receive_ok = transmitter->receive(&prefix, 8);
  if (!receive_ok) {
    return hal_server::status_transmitter_failed;
  }
  if (debug) {
    std::cerr << "Hal Server:: received command " << (uint32_t)prefix.command
              << "\n";
  }
  assert(prefix.device == 0);

  // Don't support device != 0, so shutdown for now, we'll look to implement
  // this further down the line.
  if (prefix.device != 0) {
    return hal_server::status_device_not_supported;
  }

  auto ret_val = hal_server::status_success;
  switch (prefix.command) {
    case hal_binary_encoder::COMMAND::MEM_ALLOC:
      ret_val = process_mem_alloc(prefix.device);
      break;
    case hal_binary_encoder::COMMAND::MEM_FREE:
      ret_val = process_mem_free(prefix.device);
      break;
    case hal_binary_encoder::COMMAND::MEM_WRITE:
      ret_val = process_mem_write(prefix.device);
      break;
    case hal_binary_encoder::COMMAND::MEM_READ:
      ret_val = process_mem_read(prefix.device);
      break;
    case hal_binary_encoder::COMMAND::MEM_FILL:
      ret_val = process_mem_fill(prefix.device);
      break;
    case hal_binary_encoder::COMMAND::MEM_COPY:
      ret_val = process_mem_copy(prefix.device);
      break;
    case hal_binary_encoder::COMMAND::PROGRAM_FREE:
      ret_val = process_program_free(prefix.device);
      break;
    case hal_binary_encoder::COMMAND::FIND_KERNEL:
      ret_val = process_find_kernel(prefix.device);
      break;
    case hal_binary_encoder::COMMAND::PROGRAM_LOAD:
      ret_val = process_program_load(prefix.device);
      break;
    case hal_binary_encoder::COMMAND::KERNEL_EXEC:
      ret_val = process_kernel_exec(prefix.device);
      break;
    case hal_binary_encoder::COMMAND::DEVICE_CREATE:
      ret_val = process_device_create(prefix.device);
      break;
    case hal_binary_encoder::COMMAND::DEVICE_DELETE:
      ret_val = process_device_delete(prefix.device);
      break;
    default:
      assert(0 && "Unknown command");
      return hal_server::status_unknown_command;
  }
  if (!receive_ok) {
    return hal_server::status_transmitter_failed;
  }

  return ret_val;
}
}  // namespace hal
