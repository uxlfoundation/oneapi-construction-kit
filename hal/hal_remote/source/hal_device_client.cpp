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
#include <hal_remote/hal_device_client.h>

namespace hal {

hal_device_client::hal_device_client(hal::hal_device_info_t *info,
                                     std::mutex &hal_lock,
                                     hal::hal_transmitter *transmitter)
    : hal::hal_device_t(info), transmitter(transmitter), hal_lock(hal_lock) {
  if (const char *env = getenv("CA_HAL_DEBUG")) {
    if (*env == '1') debug = true;
  }
}

hal::hal_addr_t hal_device_client::mem_alloc(hal::hal_size_t size,
                                             hal::hal_size_t alignment) {
  const std::lock_guard<std::mutex> locker(hal_lock);
  hal_binary_encoder encoder;
  hal_binary_decoder decoder;

  encoder.encode_mem_alloc(size, alignment);
  bool ok = transmitter->send(encoder.data(), encoder.size(), true);
  ok = ok && receive_decode_reply(hal_binary_encoder::COMMAND::MEM_ALLOC_REPLY,
                                  decoder);

  if (ok) {
    return decoder.message.alloc_reply;
  } else {
    assert(0 && "Error Failed to get MEM_ALLOC_REPLY\n");
    return 0;
  }
}

bool hal_device_client::mem_write(hal::hal_addr_t dst, const void *src,
                                  hal::hal_size_t size) {
  const std::lock_guard<std::mutex> locker(hal_lock);
  hal_binary_encoder encoder;
  hal_binary_decoder decoder;
  encoder.encode_mem_write(dst, size);
  bool ok = transmitter->send(encoder.data(), encoder.size(), false);
  ok = ok && transmitter->send(src, size, true);
  ok = ok && receive_decode_reply(hal_binary_encoder::COMMAND::MEM_WRITE_REPLY,
                                  decoder);

  if (ok) {
    return decoder.message.write_reply;
  } else {
    assert(0 && "Error Failed to get MEM_WRITE_REPLY\n");
    return false;
  }
}

bool hal_device_client::mem_fill(hal::hal_addr_t dst, const void *pattern,
                                 hal::hal_size_t pattern_size,
                                 hal::hal_size_t size) {
  const std::lock_guard<std::mutex> locker(hal_lock);
  hal_binary_encoder encoder;
  hal_binary_decoder decoder;
  encoder.encode_mem_fill(dst, pattern_size, size);
  bool ok = transmitter->send(encoder.data(), encoder.size(), false);
  ok = ok && transmitter->send(pattern, pattern_size, true);
  ok = ok && receive_decode_reply(hal_binary_encoder::COMMAND::MEM_FILL_REPLY,
                                  decoder);
  if (ok) {
    return decoder.message.fill_reply;
  } else {
    assert(0 && "Error Failed to get MEM_FILL_REPLY\n");
    return false;
  }
}

bool hal_device_client::program_free(hal::hal_program_t program) {
  const std::lock_guard<std::mutex> locker(hal_lock);
  hal_binary_encoder encoder;
  hal_binary_decoder decoder;
  encoder.encode_program_free(program);
  bool ok = transmitter->send(encoder.data(), encoder.size(), true);
  ok = ok && receive_decode_reply(
                 hal_binary_encoder::COMMAND::PROGRAM_FREE_REPLY, decoder);
  if (ok) {
    return decoder.message.free_reply;
  } else {
    assert(0 && "Error Failed to get PROGRAM_FREE_REPLY\n");
    return false;
  }
}

hal::hal_program_t hal_device_client::program_load(const void *data,
                                                   hal::hal_size_t size) {
  const std::lock_guard<std::mutex> locker(hal_lock);
  hal_binary_encoder encoder;
  hal_binary_decoder decoder;

  encoder.encode_program_load(size);
  bool ok = transmitter->send(encoder.data(), encoder.size(), false);
  ok = ok && transmitter->send(data, size, true);
  ok = ok && receive_decode_reply(
                 hal_binary_encoder::COMMAND::PROGRAM_LOAD_REPLY, decoder);
  if (ok) {
    return decoder.message.prog_load_reply;
  } else {
    assert(0 && "Error Failed to get PROGRAM_LOAD_REPLY\n");
    return false;
  }
}

hal::hal_kernel_t hal_device_client::program_find_kernel(
    hal::hal_program_t program, const char *name) {
  const std::lock_guard<std::mutex> locker(hal_lock);
  hal_binary_encoder encoder;
  hal_binary_decoder decoder;
  const uint32_t name_length = strlen(name) + 1;
  encoder.encode_find_kernel(program, name_length);
  bool ok = transmitter->send(encoder.data(), encoder.size(), false);
  ok = ok && transmitter->send(name, name_length, true);
  ok = ok && receive_decode_reply(
                 hal_binary_encoder::COMMAND::FIND_KERNEL_REPLY, decoder);
  if (ok) {
    return decoder.message.find_kernel_reply;
  } else {
    assert(0 && "Error Failed to get FIND_KERNEL_REPLY\n");
    return false;
  }
}

bool hal_device_client::mem_free(hal::hal_addr_t addr) {
  const std::lock_guard<std::mutex> locker(hal_lock);
  hal_binary_encoder encoder;
  hal_binary_decoder decoder;

  encoder.encode_mem_free(addr);
  bool ok = transmitter->send(encoder.data(), encoder.size(), true);
  ok = ok && receive_decode_reply(hal_binary_encoder::COMMAND::MEM_FREE_REPLY,
                                  decoder);

  if (ok) {
    return decoder.message.free_reply;
  } else {
    assert(0 && "Error Failed to get MEM_FREE_REPLY\n");
    return false;
  }
}

bool hal_device_client::mem_copy(hal::hal_addr_t dst, hal::hal_addr_t src,
                                 hal::hal_size_t size) {
  const std::lock_guard<std::mutex> locker(hal_lock);
  hal_binary_encoder encoder;
  hal_binary_decoder decoder;
  encoder.encode_mem_copy(dst, src, size);
  bool ok = transmitter->send(encoder.data(), encoder.size(), true);
  ok = ok && receive_decode_reply(hal_binary_encoder::COMMAND::MEM_COPY_REPLY,
                                  decoder);

  if (ok) {
    return decoder.message.copy_reply;
  } else {
    assert(0 && "Error Failed to get MEM_COPY_REPLY\n");
    return false;
  }
}

bool hal_device_client::kernel_exec(hal::hal_program_t program,
                                    hal::hal_kernel_t kernel,
                                    const hal::hal_ndrange_t *nd_range,
                                    const hal::hal_arg_t *args,
                                    uint32_t num_args, uint32_t work_dim) {
  const std::lock_guard<std::mutex> locker(hal_lock);
  if (hal_debug()) {
    std::cerr << "hal_device_client::kernel_exec(kernel=" << kernel
              << " num_args=" << num_args << " global = <"
              << nd_range->global[0] << ":" << nd_range->global[1] << ":"
              << nd_range->global[2] << "> local = <" << nd_range->local[0]
              << ":" << nd_range->local[1] << ":" << nd_range->local[2]
              << ">\n";
  }
  hal_binary_encoder encoder;
  hal_binary_decoder decoder;
  encoder.encode_kernel_exec(program, kernel, nd_range, num_args, work_dim);
  bool ok = transmitter->send(encoder.data(), encoder.size(), false);
  encoder.clear();
  encoder.encode_kernel_exec_args(args, num_args);
  ok = ok && transmitter->send(encoder.data(), encoder.size(), true);
  ok = ok && receive_decode_reply(
                 hal_binary_encoder::COMMAND::KERNEL_EXEC_REPLY, decoder);

  if (ok) {
    return decoder.message.kernel_exec_reply;
  } else {
    assert(0 && "Error Failed to get KERNEL_EXEC_REPLY\n");
    return false;
  }
}
bool hal_device_client::mem_read(void *dst, hal::hal_addr_t src,
                                 hal::hal_size_t size) {
  const std::lock_guard<std::mutex> locker(hal_lock);
  hal_binary_encoder encoder;
  hal_binary_decoder decoder;
  encoder.encode_mem_read(src, size);

  bool ok = transmitter->send(encoder.data(), encoder.size(), true);
  ok = ok && receive_decode_reply(hal_binary_encoder::COMMAND::MEM_READ_REPLY,
                                  decoder);
  if (ok) {
    ok = ok && transmitter->receive(dst, size);
    return ok && decoder.message.read_reply;
  } else {
    assert(0 && "Error Failed to get MEM_READ_REPLY\n");
    return false;
  }
  return ok;
}

bool hal_device_client::receive_decode_reply(
    hal_binary_encoder::COMMAND expected_command, hal_binary_decoder &decoder) {
  hal_binary_encoder::COMMAND command;
  bool ok = true;
  ok =
      ok && transmitter->receive(&command, sizeof(hal_binary_encoder::COMMAND));
  assert(ok);
  ok = ok && (command == expected_command);
  assert(ok);
  if (ok) {
    const uint32_t data_required =
        decoder.decode_command_data_required(command);
    std::vector<uint8_t> payload(data_required);
    ok = ok && transmitter->receive(payload.data(), data_required);
    if (ok) {
      ok = ok && decoder.decode(command, payload.data(), data_required);
      assert(ok);
    }
  } else {
    return false;
  }
  return ok;
}

}  // namespace hal
