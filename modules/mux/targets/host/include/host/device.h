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

/// @file
/// Host's device interface.

#ifndef HOST_DEVICE_H_INCLUDED
#define HOST_DEVICE_H_INCLUDED

#include "host/builtin_kernel.h"
#include "host/queue.h"
#include "host/thread_pool.h"
#include "mux/mux.h"

#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
#include "host/papi_counter.h"
#endif

namespace host {
/// @addtogroup host
/// @{

/// @brief Enumeration of target architectures.
enum arch : uint8_t {
  ARM = 0x1 << 0,
  AARCH64 = 0x1 << 1,
  X86 = 0x1 << 2,
  X86_64 = 0x1 << 3,
  RISCV32 = 0x1 << 4,
  RISCV64 = 0x1 << 5,
};

enum os : uint8_t {
  LINUX,
  WINDOWS,
  MACOS,
  ANDROID,
};

struct device_info_s final : public mux_device_info_s {
  /// @brief Default constructor, delegates to the main constructor.
  ///
  /// Detects the device's OS and architecture and assumes that the device
  /// compiled natively for the host architecture.
  device_info_s();

  /// @brief Constructor setting the device name and delegates to the main
  /// constructor.
  ///
  /// Detects the device's OS and architecture and assumes that the device
  /// compiled natively for the host architecture.
  ///
  /// @param device_name Name of the device. Lifetime needs to be as long or
  /// longer than the lifetime of the created object.
  device_info_s(const char *device_name);

  /// @brief Main constructor, actually initialises the object.
  ///
  /// @param arch The target architecture.
  /// @param os The target operating system.
  /// @param native Flag to specify if the device is compiling natively, `true`
  /// when targeting the host architecture, `false` otherwise.
  /// @param device_name Name of the device. Lifetime needs to be as long or
  /// longer than the lifetime of the created object.
  device_info_s(host::arch arch, host::os os, bool native,
                const char *device_name);

  /// @brief Semi-colon separated list of builtin kernel names.
  std::string builtin_kernel_list;

  host::builtin_kernel_map builtin_kernel_map;
  /// @brief The target architecture.
  host::arch arch;
  /// @brief The target operating system.
  host::os os;
  /// @brief Flag to specify if the compiler using this device info is compiling
  /// natively.
  bool native;

#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
  cargo::dynamic_array<host_papi_counter> papi_counters;
#endif

  /// @brief Detects the device's architecture.
  static host::arch detectHostArch();
  /// @brief Detects the device's OS.
  static host::os detectHostOS();
  /// @brief Returns the static instance of the host device info, returned by
  /// hostGetDeviceInfos.
  static host::device_info_s &getHostInstance();
};

struct device_s final : public mux_device_s {
  /// @brief Main constructor.
  ///
  /// @param info The device info associated with this device.
  /// @param allocator The mux allocator to use for allocations.
  explicit device_s(device_info_s *info, mux_allocator_info_t allocator);

  /// @brief The thread-pool providing multi-threaded execution.
  thread_pool_s thread_pool;

  /// @brief Host's single queue for command execution.
  host::queue_s queue;
};

/// @}
}  // namespace host

#endif  // HOST_DEVICE_H_INCLUDED
