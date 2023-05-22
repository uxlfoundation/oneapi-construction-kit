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

#include "host/target.h"

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/RuntimeDyld.h>
#include <llvm/IR/DiagnosticInfo.h>
#include <llvm/IR/DiagnosticPrinter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>

#include "host/device.h"
#include "host/info.h"
#include "host/module.h"

#ifdef CA_ENABLE_HOST_BUILTINS
#include "compiler/utils/memory_buffer.h"
#include "host/resources.h"
#endif

namespace host {
HostTarget::HostTarget(const HostInfo *compiler_info,
                       compiler::Context *context,
                       compiler::NotifyCallbackFn callback)
    : BaseTarget(compiler_info, context, callback) {
  const char *target_name = "host";
  supported_target_snapshots = {
      compiler::BaseModule::getTargetSnapshotName(target_name,
                                                  HOST_SNAPSHOT_VECTORIZED),
      compiler::BaseModule::getTargetSnapshotName(target_name,
                                                  HOST_SNAPSHOT_BARRIER),
      compiler::BaseModule::getTargetSnapshotName(target_name,
                                                  HOST_SNAPSHOT_SCHEDULED),
  };
}

compiler::Result HostTarget::initWithBuiltins(
    std::unique_ptr<llvm::Module> builtins_module) {
  builtins = std::move(builtins_module);

#ifdef CA_ENABLE_HOST_BUILTINS
  auto loadedModule = llvm::getOwningLazyBitcodeModule(
      std::make_unique<compiler::utils::MemoryBuffer>(
          rc::host::host_x86_64_unknown_unknown_bc.data(),
          rc::host::host_x86_64_unknown_unknown_bc.size()),
      getLLVMContext());
  if (!loadedModule) {
    return compiler::Result::FAILURE;
  }
  builtins_host = std::move(loadedModule.get());
#endif

  // initialize LLVM targets
  static std::once_flag llvm_initialized;
  std::call_once(llvm_initialized, [&]() {
    if (HostInfo::arches & host::arch::ARM) {
#ifdef HOST_LLVM_ARM
      LLVMInitializeARMTarget();
      LLVMInitializeARMTargetInfo();
      LLVMInitializeARMAsmPrinter();
      LLVMInitializeARMTargetMC();
#else
      llvm_unreachable("ARM backend requested with no LLVM support");
#endif
    }

    if (HostInfo::arches & host::arch::AARCH64) {
#ifdef HOST_LLVM_AARCH64
      LLVMInitializeAArch64Target();
      LLVMInitializeAArch64TargetInfo();
      LLVMInitializeAArch64AsmPrinter();
      // We create inline assembly in IR for setting the floating point control
      // register, since there is currently no intrinsic.
      LLVMInitializeAArch64AsmParser();
      LLVMInitializeAArch64TargetMC();
#else
      llvm_unreachable("AArch64 backend requested with no LLVM support");
#endif
    }

    if (HostInfo::arches & (host::arch::X86 | host::arch::X86_64)) {
#ifdef HOST_LLVM_X86
      LLVMInitializeX86Target();
      LLVMInitializeX86TargetInfo();
      LLVMInitializeX86AsmPrinter();
      LLVMInitializeX86TargetMC();
#else
      llvm_unreachable("X86 backend requested with no LLVM support");
#endif
    }
  });

  // We need this module to init our EngineBuilder, which in turn is used to
  // create our TargetMachine.
  std::unique_ptr<llvm::Module> module(new llvm::Module("", getLLVMContext()));
  if (nullptr == module) {
    return compiler::Result::FAILURE;
  }

  auto host_device_info =
      static_cast<host::device_info_s &>(*compiler_info->device_info);

  switch (host_device_info.arch) {
    case host::arch::ARM:
      assert(host::os::LINUX == host_device_info.os &&
             "ARM cross-compile only supports Linux");
      // For cross compiled ARM builds, we can't rely on sys::getProcessTriple
      // to determine our target. Instead, we set it to a known working triple.
      module->setTargetTriple("armv7-unknown-linux-gnueabihf-elf");
      break;
    case host::arch::AARCH64:
      assert(host::os::LINUX == host_device_info.os &&
             "AArch64 cross-compile only supports Linux");
      module->setTargetTriple("aarch64-linux-gnu-elf");
      break;
    case host::arch::X86:
    case host::arch::X86_64:
      switch (host_device_info.os) {
        case host::os::ANDROID:
        case host::os::LINUX:
          module->setTargetTriple(host::arch::X86 == host_device_info.arch
                                      ? "i386-unknown-unknown-elf"
                                      : "x86_64-unknown-unknown-elf");
          break;
        case host::os::WINDOWS:
          // Using windows here ensures that _chkstk() is called which is
          // important for paging in the stack.
          module->setTargetTriple(host::arch::X86 == host_device_info.arch
                                      ? "i386-pc-windows-msvc-elf"
                                      : "x86_64-pc-windows-msvc-elf");
          break;
        case host::os::MACOS:
          assert(host_device_info.native &&
                 "macOS cross-compile not supported");
          // On Apple, the MachO loader has a bug and can't JIT correctly. We
          // have to force elf generation for MachO builds. See Redmine #7621.
          module->setTargetTriple(llvm::sys::getProcessTriple() + "-elf");
          break;
      }
      break;
  }
  // Grab a reference to the module to be able to remove it later.
  llvm::Module *modulePtr = module.get();

  llvm::EngineBuilder builder(std::move(module));

  builder.setEngineKind(llvm::EngineKind::JIT);

  // Note: Aggressive optimizations in the backend can lead to problems
  // debugging kernels, change this to `CodeGenOpt::None` to solve these.
  // Ideally we should have a programmatic way of setting this.
  builder.setOptLevel(llvm::CodeGenOpt::Aggressive);

  // set the triple to what the module is using
  llvm::Triple triple;
  triple.setTriple(modulePtr->getTargetTriple());

  llvm::SmallVector<std::string, 8> features;

  if (llvm::Triple::arm == triple.getArch()) {
    features.push_back("+strict-align");
    // We do not support denormals for single precision floating points, but we
    // do for double precision. To support that we use neon (which is FTZ) for
    // single precision floating points, and use the VFP with denormal support
    // enabled for doubles. The neonfp feature enables the use of neon for
    // single precision floating points.
    features.push_back("+neonfp");
    features.push_back("+neon");
    // Hardware division instructions might not exist on all ARMv7 CPUs, but
    // they probably exist on all the ones we might care about.
    features.push_back("+hwdiv");
    features.push_back("+hwdiv-arm");
    if (host_device_info.half_capabilities) {
      features.push_back("+fp16");
    }
  } else if ((llvm::Triple::x86 == triple.getArch()) && triple.isArch32Bit()) {
    features.push_back("+sse2");
  }

  // and select our target machine based on this information
  llvm::StringRef CPUName;
#ifndef NDEBUG
  if (const char *E = getenv("CA_HOST_TARGET_CPU")) {
    CPUName = E;
  } else {
#endif
#ifdef CA_HOST_TARGET_CPU_NATIVE
    CPUName = llvm::sys::getHostCPUName();
#elif defined(CA_HOST_TARGET_CPU)
  CPUName = CA_HOST_TARGET_CPU;
#else
  CPUName = "";
#endif
#ifndef NDEBUG
  }
#endif

  auto *TM = builder.selectTarget(triple, "", CPUName, features);
  engine.reset(builder.create(TM));

  if (nullptr == engine) {
    return compiler::Result::OUT_OF_MEMORY;
  }

  // ... and remove the module.
  if (engine->removeModule(modulePtr)) {
    // removeModule() releases the original module without deleting it so we
    // need to delete it here.
    delete modulePtr;
  }

  // don't verify input modules
  engine->setVerifyModules(false);

  // disable lazy compilation
  engine->DisableLazyCompilation(true);

  // enable symbol searching
  engine->DisableSymbolSearching(false);

  return compiler::Result::SUCCESS;
}

std::unique_ptr<compiler::Module> HostTarget::createModule(uint32_t &num_errors,
                                                           std::string &log) {
  return std::unique_ptr<HostModule>{
      new HostModule(*this, context, num_errors, log)

  };
}

}  // namespace host
