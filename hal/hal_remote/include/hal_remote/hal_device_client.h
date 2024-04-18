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

#ifndef _HAL_DEVICE_CLIENT_H
#define _HAL_DEVICE_CLIENT_H

#include <hal.h>
#include <hal_remote/hal_binary_decoder.h>
#include <hal_remote/hal_transmitter.h>
#include <hal_types.h>

#include <mutex>

namespace hal {

/// @brief A hal device which will communicate with a remote server
/// to perform the hal device actions.
/// @note This will use a default encoding through the `hal_binary_encoder`
/// class but all functions are overridable so that a user may encode
/// in their own way. For example `kernel_exec` could encrypt the kernel.
///
/// @note This uses a `hal_transmitter` to send and receive the information
/// This is a simple abstract class that could use different methods of
/// communication e.g. sockets, file descriptors etc. Also it should be noted
/// that this does not deal with device creation which should be done by the
/// `hal` and use the same transmitter.
class hal_device_client : public hal::hal_device_t {
 public:
  hal_device_client(hal::hal_device_info_t *info, std::mutex &hal_lock,
                    hal::hal_transmitter *transmitter);

  // @brief find program kernel based on `name`
  hal::hal_kernel_t program_find_kernel(hal::hal_program_t program,
                                        const char *name) override;

  // @brief load an ELF file into target memory
  // returns `hal_invalid_program` if the program could not be loaded
  hal::hal_program_t program_load(const void *data,
                                  hal::hal_size_t size) override;

  // @brief execute a kernel on the target
  bool kernel_exec(hal::hal_program_t program, hal::hal_kernel_t kernel,
                   const hal::hal_ndrange_t *nd_range,
                   const hal::hal_arg_t *args, uint32_t num_args,
                   uint32_t work_dim) override;

  // @brief unload a program from the target
  bool program_free(hal::hal_program_t program) override;

  // @brief allocate a memory range on the target
  // @return allocated address or `hal_nullptr` if the operation was
  // unsuccessful
  hal::hal_addr_t mem_alloc(hal::hal_size_t size,
                            hal::hal_size_t alignment) override;

  // @brief copy memory between target buffers
  bool mem_copy(hal::hal_addr_t dst, hal::hal_addr_t src,
                hal::hal_size_t size) override;

  // @brief free a memory range on the target
  bool mem_free(hal::hal_addr_t addr) override;

  // @brief fill memory with a pattern
  bool mem_fill(hal::hal_addr_t dst, const void *pattern,
                hal::hal_size_t pattern_size, hal::hal_size_t size) override;

  // @brief read memory from the target to the host
  bool mem_read(void *dst, hal::hal_addr_t src, hal::hal_size_t size) override;

  // @brief write host memory to the target
  bool mem_write(hal::hal_addr_t dst, const void *src,
                 hal::hal_size_t size) override;
  bool hal_debug() { return debug; }

 protected:
  /// @brief A small helper function to check the reply and download the related
  /// data into `decoder`
  /// @return true if the receive and decode worked.
  bool receive_decode_reply(hal_binary_encoder::COMMAND expected_command,
                            hal_binary_decoder &decoder);
  hal::hal_transmitter *transmitter;
  bool debug = false;
  std::mutex &hal_lock;
};
}  // namespace hal
#endif
