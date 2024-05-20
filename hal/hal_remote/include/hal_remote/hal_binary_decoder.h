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

#ifndef _HAL_BINARY_DECODER_H
#define _HAL_BINARY_DECODER_H

#include <assert.h>
#include <hal_remote/hal_binary_encoder.h>
#include <hal_types.h>

#include <iostream>

namespace hal {

/// @brief Decoder for hal binary commands
/// @note This decodes a command and can return information about the number of
/// bytes needed or decode it into internal structure `message` which contains
/// the information that can be used to call hal functions.
class hal_binary_decoder {
 public:
  hal_binary_decoder() {}

  static const uint32_t data_required_unknown = 0xffffffff;

  /// @brief get the number of additional bytes needed for a command
  /// @param command enum representing a command
  /// @return number of bytes needed or data_required_unknown (0xffffffff) if
  /// unknown command
  uint32_t decode_command_data_required(hal_binary_encoder::COMMAND command) {
    switch (command) {
      case hal_binary_encoder::COMMAND::MEM_ALLOC:
        return sizeof(message.alloc.size) + sizeof(message.alloc.alignment);
      case hal_binary_encoder::COMMAND::MEM_FREE:
        return sizeof(message.free.addr);
      case hal_binary_encoder::COMMAND::MEM_WRITE:
        return sizeof(message.write.dst) + sizeof(message.write.size);
      case hal_binary_encoder::COMMAND::MEM_READ:
        return sizeof(message.read.src) + sizeof(message.read.size);
      case hal_binary_encoder::COMMAND::MEM_FILL:
        return sizeof(message.fill.dst) + sizeof(message.fill.pattern_size) +
               sizeof(message.fill.size);
      case hal_binary_encoder::COMMAND::MEM_COPY:
        return sizeof(message.copy.dst) + sizeof(message.read.src) +
               sizeof(message.read.size);
      case hal_binary_encoder::COMMAND::PROGRAM_FREE:
        return sizeof(message.prog_free.program);
      case hal_binary_encoder::COMMAND::PROGRAM_LOAD:
        return sizeof(message.prog_load.size);
      case hal_binary_encoder::COMMAND::FIND_KERNEL:
        return sizeof(message.prog_find_kernel.kernel_name_size) +
               sizeof(message.prog_find_kernel.program);
      case hal_binary_encoder::COMMAND::MEM_ALLOC_REPLY:
        return sizeof(message.alloc_reply);
      case hal_binary_encoder::COMMAND::PROGRAM_LOAD_REPLY:
        return sizeof(message.prog_load_reply);
      case hal_binary_encoder::COMMAND::KERNEL_EXEC:
        return sizeof(message.kernel_exec.program) +
               // 8
               sizeof(message.kernel_exec.kernel) +
               // 16
               sizeof(message.kernel_exec.num_args) +
               // 20
               sizeof(message.kernel_exec.nd_range.offset[0]) * 9 +
               // 56

               sizeof(message.kernel_exec.work_dim) +
               sizeof(message.kernel_exec.args_data_size);
        // 64
      case hal_binary_encoder::COMMAND::DEVICE_CREATE:
      case hal_binary_encoder::COMMAND::DEVICE_DELETE:
        return 0;
      case hal_binary_encoder::COMMAND::FIND_KERNEL_REPLY:
        return sizeof(message.find_kernel_reply);
      case hal_binary_encoder::COMMAND::MEM_READ_REPLY:
      case hal_binary_encoder::COMMAND::MEM_WRITE_REPLY:
      case hal_binary_encoder::COMMAND::MEM_FILL_REPLY:
      case hal_binary_encoder::COMMAND::MEM_FREE_REPLY:
      case hal_binary_encoder::COMMAND::MEM_COPY_REPLY:
      case hal_binary_encoder::COMMAND::PROGRAM_FREE_REPLY:
      case hal_binary_encoder::COMMAND::KERNEL_EXEC_REPLY:
      case hal_binary_encoder::COMMAND::DEVICE_CREATE_REPLY:
      case hal_binary_encoder::COMMAND::DEVICE_DELETE_REPLY:
        return sizeof(uint32_t);  // bool as 32 bit
      default:
        std::cerr << "Unknown command : " << (uint32_t)command << "\n";
        return 0;
    }
    return data_required_unknown;
  }

  /// @brief  decode a command given data of a particular sise
  /// @param command
  /// @param data
  /// @param size length of data in bytes
  /// @return true if succeeded otherwise false.
  bool decode(hal_binary_encoder::COMMAND command, const void *data,
              uint32_t size) {
    const uint8_t *data_ptr = static_cast<const uint8_t *>(data);
    uint32_t offset = 0;
    bool ok = true;
    // This assumes both the command and device have been read.
    switch (command) {
      case hal_binary_encoder::COMMAND::MEM_ALLOC: {
        ok = ok && pull(message.alloc.size, data_ptr, offset, size);
        ok = ok && pull(message.alloc.alignment, data_ptr, offset, size);
        break;
      }

      case hal_binary_encoder::COMMAND::MEM_ALLOC_REPLY: {
        ok = ok && pull(message.alloc_reply, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::MEM_FREE: {
        ok = ok && pull(message.free.addr, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::MEM_FREE_REPLY: {
        ok = ok && pull(message.free_reply, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::MEM_WRITE: {
        ok = ok && pull(message.write.dst, data_ptr, offset, size);
        ok = ok && pull(message.write.size, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::MEM_WRITE_REPLY: {
        ok = ok && pull(message.write_reply, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::MEM_READ: {
        ok = ok && pull(message.read.src, data_ptr, offset, size);
        ok = ok && pull(message.read.size, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::MEM_READ_REPLY: {
        ok = ok && pull(message.read_reply, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::MEM_COPY: {
        ok = ok && pull(message.copy.dst, data_ptr, offset, size);
        ok = ok && pull(message.copy.src, data_ptr, offset, size);
        ok = ok && pull(message.copy.size, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::MEM_COPY_REPLY: {
        ok = ok && pull(message.copy_reply, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::PROGRAM_FREE: {
        ok = ok && pull(message.prog_free.program, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::PROGRAM_FREE_REPLY: {
        ok = ok && pull(message.prog_free_reply, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::PROGRAM_LOAD: {
        ok = ok && pull(message.prog_load.size, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::PROGRAM_LOAD_REPLY: {
        ok = ok && pull(message.prog_load_reply, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::FIND_KERNEL: {
        ok = ok &&
             pull(message.prog_find_kernel.program, data_ptr, offset, size);
        ok = ok && pull(message.prog_find_kernel.kernel_name_size, data_ptr,
                        offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::FIND_KERNEL_REPLY: {
        ok = ok && pull(message.find_kernel_reply, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::KERNEL_EXEC: {
        ok = ok && pull(message.kernel_exec.program, data_ptr, offset, size);
        ok = ok && pull(message.kernel_exec.kernel, data_ptr, offset, size);
        ok = ok && pull(message.kernel_exec.num_args, data_ptr, offset, size);
        for (uint32_t d = 0; d < 3; d++) {
          ok = ok && pull(message.kernel_exec.nd_range.offset[d], data_ptr,
                          offset, size);
        }
        for (uint32_t d = 0; d < 3; d++) {
          ok = ok && pull(message.kernel_exec.nd_range.global[d], data_ptr,
                          offset, size);
        }
        for (uint32_t d = 0; d < 3; d++) {
          ok = ok && pull(message.kernel_exec.nd_range.local[d], data_ptr,
                          offset, size);
        }
        ok = ok && pull(message.kernel_exec.work_dim, data_ptr, offset, size);
        ok = ok &&
             pull(message.kernel_exec.args_data_size, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::KERNEL_EXEC_REPLY: {
        ok = ok && pull(message.kernel_exec_reply, data_ptr, offset, size);
        break;
      }
      // device index is implicit in all commands
      case hal_binary_encoder::COMMAND::DEVICE_CREATE:
      case hal_binary_encoder::COMMAND::DEVICE_DELETE:
        break;
      case hal_binary_encoder::COMMAND::DEVICE_CREATE_REPLY: {
        ok = ok && pull(message.device_create_reply, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::DEVICE_DELETE_REPLY: {
        ok = ok && pull(message.device_delete_reply, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::MEM_FILL: {
        ok = ok && pull(message.fill.dst, data_ptr, offset, size);
        ok = ok && pull(message.fill.pattern_size, data_ptr, offset, size);
        ok = ok && pull(message.fill.size, data_ptr, offset, size);
        break;
      }
      case hal_binary_encoder::COMMAND::MEM_FILL_REPLY: {
        ok = ok && pull(message.mem_fill_reply, data_ptr, offset, size);
        break;
      }
      default:
        assert(0 && "Unable to decode message");
        return false;
    }
    assert(offset <= size);
    return ok;
  }

  bool decode_kernel_exec_args(void *data, uint32_t num_args, uint32_t size) {
    uint32_t offset = 0;
    uint8_t *data_ptr = static_cast<uint8_t *>(data);
    kernel_args.clear();
    bool ok = true;
    for (uint32_t a = 0; a < num_args; a++) {
      hal::hal_arg_t arg;
      uint32_t value;
      ok = ok && pull(value, data_ptr, offset, size);
      if (ok) {
        arg.kind = static_cast<hal::hal_arg_kind_t>(value);
      }
      ok = ok && pull(value, data_ptr, offset, size);
      if (ok) {
        arg.space = static_cast<hal::hal_addr_space_t>(value);
      }
      ok = ok && pull(arg.size, data_ptr, offset, size);
      if (ok) {
        if (arg.kind == hal::hal_arg_kind_t::hal_arg_address) {
          ok = ok && pull(arg.address, data_ptr, offset, size);
        } else {
          // point the pod_data to the allocated data.
          arg.pod_data = data_ptr + offset;
          offset += arg.size;
        }
      }
      if (!ok) {
        break;
      }
      kernel_args.push_back(arg);
    }
    return ok;
  }

  enum hal_binary_encoder::COMMAND command;
  struct MemAlloc {
    hal::hal_size_t size;
    hal::hal_addr_t alignment;
  };
  struct MemWrite {
    hal::hal_addr_t dst;
    hal::hal_size_t size;
    // We don't encode the actual data here which is expected to follow
  };
  struct MemRead {
    hal::hal_addr_t src;
    hal::hal_size_t size;
  };
  struct MemFill {
    hal::hal_addr_t dst;
    hal::hal_size_t pattern_size;
    hal::hal_size_t size;
    // We don't encode the actual pattern data here which is expected to follow
  };
  struct MemFree {
    hal::hal_addr_t addr;
  };
  struct MemCopy {
    hal::hal_addr_t dst;
    hal::hal_addr_t src;
    hal::hal_size_t size;
  };
  struct ProgramFree {
    hal::hal_program_t program;
  };
  struct ProgramLoad {
    hal::hal_size_t size;
    // data to follow
  };
  struct ProgramFindKernel {
    hal::hal_program_t program;
    hal::hal_size_t kernel_name_size;
  };
  struct KernelExec {
    hal::hal_program_t program;
    hal::hal_kernel_t kernel;
    hal::hal_ndrange_t nd_range;
    uint32_t num_args;
    uint32_t work_dim;
    uint32_t args_data_size;
    // actual args passed separately
  };
  struct DeviceCreate {
    uint32_t size;
    // data to follow
  };
  union {
    MemAlloc alloc;
    MemWrite write;
    MemRead read;
    MemFill fill;
    MemFree free;
    MemCopy copy;
    ProgramFree prog_free;
    ProgramLoad prog_load;
    ProgramFindKernel prog_find_kernel;
    KernelExec kernel_exec;
    hal::hal_addr_t alloc_reply;
    bool fill_reply;
    bool write_reply;
    bool read_reply;
    bool free_reply;
    bool copy_reply;
    bool prog_free_reply;
    bool kernel_exec_reply;
    bool device_create_reply;
    bool device_delete_reply;
    bool mem_fill_reply;
    hal::hal_program_t prog_load_reply;
    hal::hal_kernel_t find_kernel_reply;
  } message;
  std::vector<hal::hal_arg_t> kernel_args;

 private:
  bool pull(uint64_t &value, const uint8_t *data, uint32_t &offset,
            uint32_t size) {
    const uint32_t size_required = sizeof(uint64_t);
    if (offset + size_required > size) {
      return false;
    }
    std::memcpy(&value, data + offset, size_required);
    offset += size_required;
    return true;
  }
  bool pull(uint32_t &value, const uint8_t *data, uint32_t &offset,
            uint32_t size) {
    const uint32_t size_required = sizeof(uint32_t);
    if (offset + size_required > size) {
      return false;
    }
    std::memcpy(&value, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    return true;
  }
  bool pull(bool &value, const uint8_t *data, uint32_t &offset, uint32_t size) {
    const uint32_t size_required = sizeof(uint32_t);
    uint32_t val32 = 0;
    if (offset + size_required > size) {
      return false;
    }
    std::memcpy(&val32, data + offset, sizeof(uint32_t));
    value = static_cast<bool>(val32);
    offset += sizeof(uint32_t);
    return true;
  }
};
}  // namespace hal

#endif
