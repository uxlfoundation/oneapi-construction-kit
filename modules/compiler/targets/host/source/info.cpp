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

#include <host/device.h>
#include <host/host.h>
#include <host/info.h>
#include <host/target.h>

namespace host {
namespace {
/// @brief Create a static instance of a host compiler info for the template
/// argument combination.
///
/// @tparam arch architecture info of the represented device.
/// @tparam os operating system info of the represented device.
///
/// @param[in] device_name info of the represented device. Must have the same
///            or longer lifetime as the static device info object created.
///
/// @return pointer to static device info for the arch and os combination.
template <host::arch arch, host::os os>
compiler::Info *getCrossCompilerInfo(const char *device_name) {
  // A new static device info and compiler info will be created each time the
  // template is instantiated.
  static host::device_info_s device_info{arch, os, false /* native */,
                                         device_name};
  static HostInfo compiler_info{arch, os, &device_info};
  return &compiler_info;
}
}  // namespace

uint8_t HostInfo::arches = 0;

HostInfo::HostInfo()
    : HostInfo(host::device_info_s::detectHostArch(),
               host::device_info_s::detectHostOS(),
               &host::device_info_s::getHostInstance()) {}

HostInfo::HostInfo(host::arch arch, host::os os,
                   host::device_info_s *host_device_info) {
  // Bitwise-or this devices arch with the arches to properly initialize LLVM
  // on Target creation.
  arches |= arch;

  cc = llvm::CallingConv::C;
  if (arch == host::arch::X86_64) {
    // x86_64 requires it's own calling convention.
    if (os == host::os::WINDOWS) {
      cc = llvm::CallingConv::Win64;  // GCOVR_EXCL_LINE non-deterministic
    } else {
      cc = llvm::CallingConv::X86_64_SysV;
    }
  }

  // If we're instantiating a compiler for the current system, then we know it
  // supports runtime compilation, otherwise it's a cross compiler.
  device_info = host_device_info;
  supports_deferred_compilation = host_device_info->native;

  // JIT compilation is not yet supported on RISC-V
  if (arch == host::arch::RISCV32 || arch == host::arch::RISCV64) {
    supports_deferred_compilation = false;
  }
  vectorizable = true;
  dma_optimizable = true;
  scalable_vector_support = false;
  kernel_debug = true;
#ifdef CA_ENABLE_DEBUG_SUPPORT
  // Dummy values for testing. Enabled only on debug enabled builds with a
  // compiler. Report both an option which requires a value and an option which
  // is just a build flag.
  compilation_options =
      "--dummy-host-flag,0,no-op build flag;"
      "--dummy-host-flag2,0,no-op build flag;"
      "--dummy-host-option,1,no-op option which takes a value";
#else
  compilation_options = "";
#endif
}

std::unique_ptr<compiler::Target> HostInfo::createTarget(
    compiler::Context *context, compiler::NotifyCallbackFn callback) const {
  if (!context) {
    return nullptr;
  }

  return std::unique_ptr<compiler::Target>{
      new HostTarget(this, context, callback)};
}

void HostInfo::get(compiler::AddCompilerFn add_compiler) {
  // Host compiler.
  static HostInfo compiler_info;
  add_compiler(&compiler_info);

  // Cross compilers.
#ifdef HOST_CROSS_ARM
  add_compiler(getCrossCompilerInfo<host::arch::ARM, host::os::LINUX>(
      HOST_CROSS_DEVICE_NAME_ARM));
#endif
#ifdef HOST_CROSS_AARCH64
  add_compiler(getCrossCompilerInfo<host::arch::AARCH64, host::os::LINUX>(
      HOST_CROSS_DEVICE_NAME_AARCH64));
#endif
#ifdef HOST_CROSS_X86
  add_compiler(getCrossCompilerInfo<host::arch::X86, HOST_OS>(
      HOST_CROSS_DEVICE_NAME_X86));
#endif
#ifdef HOST_CROSS_X86_64
  add_compiler(getCrossCompilerInfo<host::arch::X86_64, HOST_OS>(
      HOST_CROSS_DEVICE_NAME_X86_64));
#endif
#ifdef HOST_CROSS_RISCV32
  add_compiler(getCrossCompilerInfo<host::arch::RISCV32, host::os::LINUX>(
      HOST_CROSS_DEVICE_NAME_RISCV32));
#endif
#ifdef HOST_CROSS_RISCV64
  add_compiler(getCrossCompilerInfo<host::arch::RISCV64, host::os::LINUX>(
      HOST_CROSS_DEVICE_NAME_RISCV64));
#endif
}
}  // namespace host
