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
#include <llvm/ExecutionEngine/JITLink/EHFrameSupport.h>
#include <llvm/ExecutionEngine/Orc/EPCEHFrameRegistrar.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/TargetProcess/RegisterEHFrames.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/DiagnosticInfo.h>
#include <llvm/IR/DiagnosticPrinter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <multi_llvm/multi_llvm.h>

#if LLVM_VERSION_GREATER_EQUAL(18, 0)
#include <llvm/TargetParser/Host.h>
#else
#include <llvm/Support/Host.h>
#endif

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
    : BaseTarget(compiler_info, context, callback),
      llvm_ts_context(std::make_unique<llvm::LLVMContext>()) {}

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

  llvm::Triple triple;

  auto host_device_info =
      static_cast<host::device_info_s &>(*compiler_info->device_info);

  switch (host_device_info.arch) {
    case host::arch::ARM:
      assert(host::os::LINUX == host_device_info.os &&
             "ARM cross-compile only supports Linux");
      // For cross compiled ARM builds, we can't rely on sys::getProcessTriple
      // to determine our target. Instead, we set it to a known working triple.
      triple = llvm::Triple("armv7-unknown-linux-gnueabihf-elf");
      break;
    case host::arch::AARCH64:
      assert(host::os::LINUX == host_device_info.os &&
             "AArch64 cross-compile only supports Linux");
      triple = llvm::Triple("aarch64-linux-gnu-elf");
      break;
    case host::arch::X86:
    case host::arch::X86_64:
      switch (host_device_info.os) {
        case host::os::ANDROID:
        case host::os::LINUX:
          triple = llvm::Triple(host::arch::X86 == host_device_info.arch
                                    ? "i386-unknown-unknown-elf"
                                    : "x86_64-unknown-unknown-elf");
          break;
        case host::os::WINDOWS:
          // Using windows here ensures that _chkstk() is called which is
          // important for paging in the stack.
          triple = llvm::Triple(host::arch::X86 == host_device_info.arch
                                    ? "i386-pc-windows-msvc-elf"
                                    : "x86_64-pc-windows-msvc-elf");
          break;
        case host::os::MACOS:
          assert(host_device_info.native &&
                 "macOS cross-compile not supported");
          // On Apple, the MachO loader has a bug and can't JIT correctly. We
          // have to force elf generation for MachO builds. See Redmine #7621.
          triple = llvm::Triple(llvm::sys::getProcessTriple() + "-elf");
          break;
      }
      break;
  }

  llvm::StringRef CPUName = "";
  llvm::StringMap<bool> FeatureMap;

  if (llvm::Triple::arm == triple.getArch()) {
    FeatureMap["strict-align"] = true;
    // We do not support denormals for single precision floating points, but we
    // do for double precision. To support that we use neon (which is FTZ) for
    // single precision floating points, and use the VFP with denormal support
    // enabled for doubles. The neonfp feature enables the use of neon for
    // single precision floating points.
    FeatureMap["neonfp"] = true;
    FeatureMap["neon"] = true;
    // Hardware division instructions might not exist on all ARMv7 CPUs, but
    // they probably exist on all the ones we might care about.
    FeatureMap["hwdiv"] = true;
    FeatureMap["hwdiv-arm"] = true;
    if (host_device_info.half_capabilities) {
      FeatureMap["fp16"] = true;
    }
  } else if (triple.isX86()) {
    CPUName = "x86-64-v3";  // Default only, may be overridden below.
  }

#ifndef NDEBUG
  if (const char *E = getenv("CA_HOST_TARGET_CPU")) {
    CPUName = E;
  }
#endif

#ifdef CA_HOST_TARGET_CPU_NATIVE
  CPUName = "native";
#elif defined(CA_HOST_TARGET_CPU)
  CPUName = CA_HOST_TARGET_CPU;
#endif

  if (CPUName == "native") {
    CPUName = llvm::sys::getHostCPUName();
    FeatureMap.clear();
    llvm::sys::getHostCPUFeatures(FeatureMap);
  }

  llvm::orc::JITTargetMachineBuilder TMBuilder(triple);
  TMBuilder.setCPU(CPUName.str());
  TMBuilder.setCodeGenOptLevel(multi_llvm::CodeGenOptLevel::Aggressive);
  for (auto &Feature : FeatureMap) {
    TMBuilder.getFeatures().AddFeature(Feature.first(), Feature.second);
  }
  auto Builder = llvm::orc::LLJITBuilder();

  Builder.setJITTargetMachineBuilder(TMBuilder);

  // Customize the JIT linking layer to provide better profiler/debugger
  // integration.
  Builder.setObjectLinkingLayerCreator(
      [&](llvm::orc::ExecutionSession &ES, const llvm::Triple &)
          -> llvm::Expected<std::unique_ptr<llvm::orc::ObjectLayer>> {
        auto GetMemMgr = []() {
          return std::make_unique<llvm::SectionMemoryManager>();
        };
        auto ObjLinkingLayer =
            std::make_unique<llvm::orc::RTDyldObjectLinkingLayer>(
                ES, std::move(GetMemMgr));

        // Register the event listener.
        ObjLinkingLayer->registerJITEventListener(
            *llvm::JITEventListener::createGDBRegistrationListener());

        // Make sure the debug info sections aren't stripped.
        ObjLinkingLayer->setProcessAllSections(true);

        return std::move(ObjLinkingLayer);
      });

  auto JIT = Builder.create();
  if (auto err = JIT.takeError()) {
    if (auto callback = getNotifyCallbackFn()) {
      callback(llvm::toString(std::move(err)).c_str(), /*data*/ nullptr,
               /*data_size*/ 0);
    } else {
      llvm::consumeError(std::move(err));
    }
    return compiler::Result::OUT_OF_MEMORY;
  }
  orc_engine = std::move(*JIT);

  auto TM = TMBuilder.createTargetMachine();
  if (auto err = TM.takeError()) {
    if (auto callback = getNotifyCallbackFn()) {
      callback(llvm::toString(std::move(err)).c_str(), /*data*/ nullptr,
               /*data_size*/ 0);
    } else {
      llvm::consumeError(std::move(err));
    }
    return compiler::Result::FAILURE;
  }
  target_machine = std::move(*TM);

  return compiler::Result::SUCCESS;
}

std::unique_ptr<compiler::Module> HostTarget::createModule(uint32_t &num_errors,
                                                           std::string &log) {
  return std::unique_ptr<HostModule>{
      new HostModule(*this, context, num_errors, log)

  };
}

llvm::LLVMContext &HostTarget::getLLVMContext() {
  return *llvm_ts_context.getContext();
}

const llvm::LLVMContext &HostTarget::getLLVMContext() const {
  return *llvm_ts_context.getContext();
}

llvm::Module *HostTarget::getBuiltins() const { return builtins.get(); };

}  // namespace host
