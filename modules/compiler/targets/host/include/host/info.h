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

#ifndef HOST_INFO_H
#define HOST_INFO_H

#include <llvm/IR/CallingConv.h>

#include <mutex>
#include <unordered_set>
#include <vector>

#include "compiler/info.h"
#include "host/device.h"
#include "mux/config.h"
#include "mux/mux.h"

#if __ANDROID__
#define HOST_OS host::os::ANDROID
#elif defined(__linux__)
#define HOST_OS host::os::LINUX
#elif defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#define HOST_OS host::os::WINDOWS
#elif defined(__APPLE__)
#define HOST_OS host::os::MACOS
#else
#error cant detect host operating system
#endif

namespace host {

/// @brief Host compiler info.
struct HostInfo : compiler::Info {
  /// @brief Instantiate a host compiler info for the current platform.
  HostInfo();

  /// @brief Instantiate a host compiler info for the argument combination.
  ///
  /// @param arch Architecture info of the represented device.
  /// @param os Operating system info of the represented device.
  /// @param host_device_info The host device info to target. Lifetime needs to
  /// be as long or longer than the lifetime of the HostInfo.
  HostInfo(host::arch arch, host::os os, host::device_info_s *host_device_info);

  /// @see Info::createTarget
  std::unique_ptr<compiler::Target> createTarget(
      compiler::Context *context,
      compiler::NotifyCallbackFn callback) const override;

  static void get(compiler::AddCompilerFn add_compiler);

  llvm::CallingConv::ID cc;

  /// @brief Bitfield of all `host::arch`'s being targeted.
  static uint8_t arches;

  bool supports_deferred_compilation() const override;

 private:
  bool deferred_compilation_enabled;
  mutable const char *deferred_compilation_warning;
};

}  // namespace host

#endif  // HOST_INFO_H
