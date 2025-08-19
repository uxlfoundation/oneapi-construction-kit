// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef _REFSIDRV_REFSI_COMMAND_PROCESSOR_H
#define _REFSIDRV_REFSI_COMMAND_PROCESSOR_H

#include <condition_variable>
#include <thread>
#include <vector>

#include "refsi_device.h"

struct RefSiDevice;

/// @brief Represents a request to execute a command buffer on the CMP.
struct RefSiCommandRequest {
  /// @brief Address where the command buffer is located in device memory.
  refsi_addr_t command_buffer_addr;
  /// @brief Size of the command buffer, in bytes.
  size_t command_buffer_size;
};

/// @brief Utility structure holding the state needed to execute a CMP command.
struct RefSiCommandContext {
  /// @brief Decoded opcode for the command.
  refsi_cmp_command_id opcode = CMP_NOP;
  /// @brief Array of chunks for the command.
  uint64_t *chunks = nullptr;
  /// @brief Number of chunks in @p chunks.
  uint32_t num_chunks = 0;
  /// @brief Contents of the command's inline chunk.
  uint32_t inline_chunk = 0;
  /// @brief Reference to a mutex held while executing CMP commands.
  RefSiLock &lock;

  /// @brief Create a new CMP command context.
  /// @param lock Mutex to hold while executing CMP commands.
  RefSiCommandContext(RefSiLock &lock) : lock(lock) {}
};

/// @brief Represents a RefSi command processor (CMP) in a RefSi platform. The
/// command processor is responsible for executing command buffers when requests
/// are added to its command request queue. The CMP can coordinate access to
/// different processing elements in the RefSi platform, such as RISC-V
/// accelerator cores or the DMA controller.
struct RefSiCommandProcessor {
  /// @brief Create a new CMP device.
  /// @param soc RefSi device to use when executing commands.
  RefSiCommandProcessor(RefSiDevice &soc);

  /// @brief Start processing command buffer requests.
  /// @param lock Mutex to hold while executing CMP commands.
  void start(RefSiLock &lock);
  /// @brief Stop processing command buffer requests until @p start is called
  /// again.
  /// @param lock Mutex to hold while executing CMP commands.
  void stop(RefSiLock &lock);

  /// @brief Add a command request to the CMP's queue.
  /// @param lock Mutex to hold while executing CMP commands.
  void enqueueRequest(RefSiCommandRequest request, RefSiLock &lock);

  /// @brief Wait for the CMP's queue to be empty. This can be used to wait for
  /// the CMP to have finished executing all command requests that have been
  /// previously added to its queue.
  /// @param lock Mutex to hold while executing CMP commands.
  void waitEmptyQueue(RefSiLock &lock);

  /// @brief Build a textual representation of the register ID.
  /// @param reg_id Register to get a textual representation for.
  static std::string getRegisterName(refsi_cmp_register_id reg_id);

  /// @brief Build a textual representation of the device address.
  /// @param address Device address to get a textual representation for.
  std::string formatDeviceAddress(refsi_addr_t address);

 private:
  /// @brief Function called when the CMP's worker thread is started, which
  /// removes command requests from the CMP queue and executes them.
  /// @param cmp Command processor object..
  static void workerMain(RefSiCommandProcessor *cmp);
  /// @brief Execute a command request on the CMP.
  /// @param request Command request to execute.
  /// @param lock Mutex to hold while executing CMP commands.
  refsi_result execute(RefSiCommandRequest request, RefSiLock &lock);
  /// @brief Execute a decoded command on the CMP.
  /// @param cmd State needed to execute the command.
  refsi_result executeCommand(RefSiCommandContext &cmd);
  /// @brief Execute a WRITE_REG64 command on the CMP.
  /// @param cmd State needed to execute the command.
  refsi_result executeWRITE_REG64(RefSiCommandContext &cmd);
  /// @brief Execute a LOAD_REG64 command on the CMP.
  /// @param cmd State needed to execute the command.
  refsi_result executeLOAD_REG64(RefSiCommandContext &cmd);
  /// @brief Execute a STORE_REG64 command on the CMP.
  /// @param cmd State needed to execute the command.
  refsi_result executeSTORE_REG64(RefSiCommandContext &cmd);
  /// @brief Execute a STORE_IMM64 command on the CMP.
  /// @param cmd State needed to execute the command.
  refsi_result executeSTORE_IMM64(RefSiCommandContext &cmd);
  /// @brief Execute a COPY_MEM64 command on the CMP.
  /// @param cmd State needed to execute the command.
  refsi_result executeCOPY_MEM64(RefSiCommandContext &cmd);
  /// @brief Execute a RUN_KERNEL_SLICE command on the CMP.
  /// @param cmd State needed to execute the command.
  refsi_result executeRUN_KERNEL_SLICE(RefSiCommandContext &cmd);
  /// @brief Execute a RUN_INSTANCES command on the CMP.
  /// @param cmd State needed to execute the command.
  refsi_result executeRUN_INSTANCES(RefSiCommandContext &cmd);
  /// @brief Execute a SYNC_CACHE command on the CMP.
  /// @param cmd State needed to execute the command.
  refsi_result executeSYNC_CACHE(RefSiCommandContext &cmd);

  std::condition_variable dispatched;
  std::condition_variable executed;
  std::vector<RefSiCommandRequest> requests;
  std::vector<uint64_t> registers;
  const size_t max_requests = 4;
  RefSiDevice &soc;
  bool started = false;
  bool stopping = false;
  std::thread *worker_thread = nullptr;
  bool debug = false;
};

#endif  // _REFSIDRV_REFSI_COMMAND_PROCESSOR_H
