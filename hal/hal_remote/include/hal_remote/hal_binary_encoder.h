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

#ifndef _HAL_BINARY_ENCODER_H
#define _HAL_BINARY_ENCODER_H

#include <hal_types.h>

#include <cstring>
#include <vector>

namespace hal {

/// @brief A simple encoder class for converting hal related commands into a
/// binary format. Note this does not encrypt in any way; if this is required
/// this should be done on top of this.
/// @note Unlike the decoder this also encodes the command and a device id for
/// the non-reply commands.
class hal_binary_encoder {
 public:
  enum class COMMAND : uint32_t {
    UNKNOWN,  // To aid debugging, force a non-zero command for real commands
    MEM_ALLOC = 1,
    MEM_ALLOC_REPLY = 2,
    MEM_WRITE = 3,
    MEM_WRITE_REPLY = 4,
    MEM_READ = 5,
    MEM_READ_REPLY = 6,
    MEM_FILL = 7,
    MEM_FILL_REPLY = 8,
    MEM_FREE = 9,
    MEM_FREE_REPLY = 10,
    MEM_COPY = 11,
    MEM_COPY_REPLY = 12,
    PROGRAM_FREE = 13,
    PROGRAM_FREE_REPLY = 14,
    KERNEL_EXEC = 15,
    KERNEL_EXEC_REPLY = 16,
    PROGRAM_LOAD = 17,
    PROGRAM_LOAD_REPLY = 18,
    FIND_KERNEL = 19,
    FIND_KERNEL_REPLY = 20,
    DEVICE_CREATE = 21,
    DEVICE_CREATE_REPLY = 22,
    DEVICE_DELETE = 23,
    DEVICE_DELETE_REPLY = 24
  };

  hal_binary_encoder(uint32_t device = 0) { device = device; }

  /// @brief encode mem alloc on a device
  /// @param size
  /// @param alignment
  void encode_mem_alloc(hal::hal_size_t size, hal::hal_size_t alignment) {
    push(COMMAND::MEM_ALLOC);
    push(device);
    push(size);
    push(alignment);
  }

  /// @brief Encode mem alloc address as a reply
  /// @param reply
  void encode_mem_alloc_reply(hal::hal_addr_t reply) {
    push(COMMAND::MEM_ALLOC_REPLY);
    push(reply);
  }

  /// @brief encode device mem free
  /// @param addr
  void encode_mem_free(hal::hal_addr_t addr) {
    push(COMMAND::MEM_FREE);
    push(device);
    push(addr);
  }

  /// @brief encode mem free reply of true/false
  /// @param reply
  void encode_mem_free_reply(bool reply) {
    push(COMMAND::MEM_FREE_REPLY);
    push(reply);
  }

  /// @brief encode device mem write
  /// @param size
  /// @param alignment
  void encode_mem_write(hal::hal_addr_t dst, hal::hal_size_t size) {
    push(COMMAND::MEM_WRITE);
    push(device);
    push(dst);
    push(size);
  }

  /// @brief Encode mem write reply of true/false
  /// @param reply
  void encode_mem_write_reply(bool reply) {
    push(COMMAND::MEM_WRITE_REPLY);
    push(reply);
  }

  /// @brief Encode device mem/fill
  /// @param dst
  /// @param pattern_size
  /// @param size
  /// @note that we need to encode the pattern data separately, which should
  ///       be pushed as a `size` bytes
  void encode_mem_fill(hal::hal_addr_t dst, hal::hal_size_t pattern_size,
                       hal::hal_size_t size) {
    push(COMMAND::MEM_FILL);
    push(device);
    push(dst);
    push(pattern_size);
    push(size);
  }

  /// @brief Encode mem fill reply as a boolean
  /// @param reply
  void encode_mem_fill_reply(bool reply) {
    push(COMMAND::MEM_FILL_REPLY);
    push(reply);
  }

  /// @brief Encode device mem read
  /// @param src
  /// @param size
  void encode_mem_read(hal::hal_addr_t src, hal::hal_size_t size) {
    push(COMMAND::MEM_READ);
    push(device);
    push(src);
    push(size);
  }

  /// @brief Encode mem read reply as a bool
  /// @param reply
  void encode_mem_read_reply(bool reply) {
    push(COMMAND::MEM_READ_REPLY);
    push(reply);
  }

  /// @brief Encode device mem copy
  /// @param dst
  /// @param src
  /// @param size
  void encode_mem_copy(hal::hal_addr_t dst, hal::hal_addr_t src,
                       hal::hal_size_t size) {
    push(COMMAND::MEM_COPY);
    push(device);
    push(dst);
    push(src);
    push(size);
  }

  /// @brief Encode mem copy reply as a bool
  /// @param reply
  void encode_mem_copy_reply(bool reply) {
    push(COMMAND::MEM_COPY_REPLY);
    push(reply);
  }

  /// @brief Encode device find kernel, the name_length should be the length of
  /// the string + 1 for the null terminator
  /// @param program
  /// @param name_length
  /// @note The name string should follow this, including the null terminator
  void encode_find_kernel(hal::hal_program_t program,
                          hal::hal_size_t name_length) {
    push(COMMAND::FIND_KERNEL);
    push(device);
    push(program);
    push(name_length);
  }

  /// @brief Encode find kernel reply as a `hal::hal_kernel_t`
  /// @param reply
  void encode_find_kernel_reply(hal::hal_kernel_t reply) {
    push(COMMAND::FIND_KERNEL_REPLY);
    push(reply);
  }

  /// @brief Encode device program load, `size` should be the length of the
  /// executable
  /// @param size
  /// @note The executable should be sent immediately after this. Note this does
  /// no encryption, it is up to the user to encrypt if needed.
  void encode_program_load(hal::hal_size_t size) {
    push(COMMAND::PROGRAM_LOAD);
    push(device);
    push(size);
  }

  /// @brief Encode program load reply as a hal_program_t
  /// @param reply
  void encode_program_load_reply(hal::hal_program_t reply) {
    push(COMMAND::PROGRAM_LOAD_REPLY);
    push(reply);
  }

  /// @brief Encode device program free
  /// @param program
  void encode_program_free(hal::hal_program_t program) {
    push(COMMAND::PROGRAM_FREE);
    push(device);
    push(program);
  }

  /// @brief encode program free reply as a boolean
  /// @param reply
  void encode_program_free_reply(bool reply) {
    push(COMMAND::PROGRAM_FREE_REPLY);
    push(reply);
  }

  /// @brief Encode
  /// @param program
  /// @param kernel
  /// @param nd_range
  /// @param num_args
  /// @param work_dim
  void encode_kernel_exec(hal::hal_program_t program, hal::hal_kernel_t kernel,
                          const hal::hal_ndrange_t *nd_range, uint32_t num_args,
                          uint32_t work_dim) {
    encoding.clear();
    push(COMMAND::KERNEL_EXEC);
    push(device);
    push(program);
    push(kernel);
    push(num_args);
    for (uint32_t d = 0; d < 3; d++) {
      push(nd_range->offset[d]);
    }
    for (uint32_t d = 0; d < 3; d++) {
      push(nd_range->global[d]);
    }
    for (uint32_t d = 0; d < 3; d++) {
      push(nd_range->local[d]);
    }
    push(work_dim);
  }
  void encode_kernel_exec_args(const hal::hal_arg_t *args, uint32_t num_args) {
    /*
      Encoding this information
      hal::hal_arg_kind_t kind;
      hal_addr_space_t space;
      hal::hal_size_t size;
      union {
        hal::hal_addr_t address;
        void *pod_data;
      }
  };*/
    uint32_t bytes_required = 0;
    uint32_t pod_data_required = 0;
    for (uint32_t a = 0; a < num_args; a++) {
      auto &arg = args[a];
      // kind, space (32 bit) and size (64 bit)
      bytes_required += sizeof(uint32_t) * 2 + sizeof(arg.size);
      if (arg.kind == hal::hal_arg_kind_t::hal_arg_address) {
        bytes_required += sizeof(arg.address);
      } else {
        pod_data_required += arg.size;
      }
    }
    bytes_required += pod_data_required;
    push(bytes_required);

    for (uint32_t a = 0; a < num_args; a++) {
      auto &arg = args[a];
      push(static_cast<uint32_t>(arg.kind));
      push(static_cast<uint32_t>(arg.space));
      push(arg.size);
      if (arg.kind == hal::hal_arg_kind_t::hal_arg_address) {
        push(arg.address);
      } else {
        push(arg.pod_data, arg.size);
      }
    }
  }
  void encode_kernel_exec_reply(bool reply) {
    push(COMMAND::KERNEL_EXEC_REPLY);
    push(reply);
  }

  void encode_device_create() {
    push(COMMAND::DEVICE_CREATE);
    push(device);
  }
  void encode_device_create_reply(bool success) {
    push(COMMAND::DEVICE_CREATE_REPLY);
    push(success);
  }
  void encode_device_delete() {
    push(COMMAND::DEVICE_DELETE);
    push(device);
  }
  void encode_device_delete_reply(bool success) {
    push(COMMAND::DEVICE_DELETE_REPLY);
    push(success);
  }

  void *data() { return encoding.data(); }
  uint32_t size() { return encoding.size(); }

  void clear() { encoding.clear(); }

 private:
  void expand(uint32_t size) { encoding.resize(encoding.size() + size); }
  void push(hal::hal_size_t size) {
    const uint32_t offset = encoding.size();
    expand(sizeof(hal::hal_size_t));
    std::memcpy(encoding.data() + offset, &size, sizeof(hal::hal_size_t));
  }

  void push(uint32_t val) {
    const uint32_t offset = encoding.size();
    expand(sizeof(val));
    std::memcpy(encoding.data() + offset, &val, sizeof(val));
  }
  void push(bool val) {
    const uint32_t val32 = val;
    push(val32);
  }

  void push(const void *data, uint32_t size) {
    const uint32_t offset = encoding.size();
    expand(size);
    std::memcpy(encoding.data() + offset, data, size);
  }

  void push(enum COMMAND command) {
    const uint32_t offset = encoding.size();
    expand(sizeof(command));
    std::memcpy(encoding.data() + offset, &command, sizeof(command));
  }
  std::vector<uint8_t> encoding;
  uint32_t offset = 0;
  uint32_t device = 0;
};
}  // namespace hal

#endif
