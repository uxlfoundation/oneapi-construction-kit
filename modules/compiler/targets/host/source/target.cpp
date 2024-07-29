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

#include <llvm/ADT/StringRef.h>
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
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/SubtargetFeature.h>
#include <multi_llvm/multi_llvm.h>

#if LLVM_VERSION_GREATER_EQUAL(18, 0)
#include <llvm/TargetParser/Host.h>
#else
#include <llvm/Support/Host.h>
#endif

#include <compiler/utils/gdb_registration_listener.h>
#include <compiler/utils/llvm_global_mutex.h>

#include "host/device.h"
#include "host/info.h"
#include "host/module.h"
#ifdef CA_ENABLE_HOST_BUILTINS
#include "compiler/utils/memory_buffer.h"
#include "host/resources.h"
#endif

namespace host {

// Create a target machine, `abi` is optional and may be empty
static llvm::TargetMachine *createTargetMachine(llvm::Triple TT,
                                                llvm::StringRef CPU,
                                                llvm::StringRef Features) {
  // Init the llvm target machine.
  std::string Error;
  const std::string &TripleStr = TT.str();
  const llvm::Target *LLVMTarget =
      llvm::TargetRegistry::lookupTarget(TripleStr, Error);
  if (nullptr == LLVMTarget) {
    return nullptr;
  }

  std::optional<llvm::CodeModel::Model> CM;
  // Unlike other architectures, RISC-V does not currently support a large code
  // model and does not change the default code model as a result of setting
  // JIT=true. The default "medium low" code model (CodeModel::Low) allows code
  // to address the entire RV32 address space, or (roughly) the lowest and
  // highest 2GiB of the RV64 address space. We do not ensure that the code and
  // its symbols are loaded somewhere in this range, so we change the code model
  // to "medium any" (CodeModel::Medium), which allows addressing roughly 2GiB
  // above and below the current position. Beware that this means we cannot
  // import any symbol from the host, as those may not be within 2GiB of the
  // kernel. This means any function needs to be included in the kernel itself.
  if (TT.isRISCV() && TT.isArch64Bit()) {
    CM = llvm::CodeModel::Medium;
  }
  // Aarch64 fails on UnitCL test
  // Execution/Execution.Barrier_02_Barrier_No_Duplicates/OfflineOpenCLC if we
  // don't set `JIT` to true. JIT for x86_64 and Aarch64 set the large code
  // model, which seems to be to do with lack of guarantees of how far away the
  // JIT memory managers find a new page. Because we use a similar mechanism for
  // loading on Host regardless of JIT, we set the flag here to set up the code
  // models for the architecture.
  // TODO: Investigate whether we can use a loader that does not have this
  // issue.
  const llvm::TargetOptions Options;
  return LLVMTarget->createTargetMachine(
      TripleStr, CPU, Features, Options, /*RM=*/std::nullopt, CM,
      multi_llvm::CodeGenOptLevel::Aggressive,
      /*JIT=*/true);
}

HostTarget::HostTarget(const HostInfo *compiler_info,
                       compiler::Context *context,
                       compiler::NotifyCallbackFn callback)
    : BaseTarget(compiler_info, context, callback),
      llvm_ts_context(std::make_unique<llvm::LLVMContext>()) {}

compiler::Result HostTarget::initWithBuiltins(
    std::unique_ptr<llvm::Module> builtins_module) {
  builtins = std::move(builtins_module);
  gdb_registration_listener = compiler::utils::createGDBRegistrationListener();

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

    if (HostInfo::arches & (host::arch::RISCV32 | host::arch::RISCV64)) {
#ifdef HOST_LLVM_RISCV
      LLVMInitializeRISCVTarget();
      LLVMInitializeRISCVTargetInfo();
      LLVMInitializeRISCVAsmPrinter();
      LLVMInitializeRISCVTargetMC();
#else
      llvm_unreachable("RISCV backend requested with no LLVM support");
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
      switch (host_device_info.os) {
        case host::os::LINUX:
          triple = llvm::Triple("aarch64-linux-gnu-elf");
          break;
        case host::os::MACOS:
          assert(host_device_info.native &&
                 "macOS cross-compile not supported");
          // Our generated code does not meet Mach-O restrictions: "mach-o
          // section specifier requires a segment and section separated by a
          // comma" when we use "notes" as a section name. Force ELF generation
          // instead, which makes things easier for us anyway as we can reuse
          // the existing ELF loading logic.
          triple = llvm::Triple(llvm::sys::getProcessTriple() + "-elf");
          break;
        default:
          assert(!"AArch64 cross-compile only supports Linux");
          break;
      }
      break;
    case host::arch::RISCV32:
      assert(host::os::LINUX == host_device_info.os &&
             "RISCV cross-compile only supports Linux");
      // For cross compiled RISCV builds, we can't rely on sys::getProcessTriple
      // to determine our target. Instead, we set it to a known working triple.
      triple = llvm::Triple("riscv32-unknown-elf");
      break;
    case host::arch::RISCV64:
      assert(host::os::LINUX == host_device_info.os &&
             "RISCV cross-compile only supports Linux");
      triple = llvm::Triple("riscv64-unknown-elf");
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
#if defined(__MINGW32__) || defined(__MINGW64__)
          triple = llvm::Triple(host::arch::X86 == host_device_info.arch
                                    ? "i386-pc-windows-gnu-elf"
                                    : "x86_64-w64-windows-gnu-elf");
#else
          triple = llvm::Triple(host::arch::X86 == host_device_info.arch
                                    ? "i386-pc-windows-msvc-elf"
                                    : "x86_64-pc-windows-msvc-elf");
#endif
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

  std::string CPU;
  llvm::SubtargetFeatures Features;

  switch (triple.getArch()) {
#ifdef HOST_LLVM_ARM
    case llvm::Triple::arm:
      // We do not support denormals for single precision floating points, but
      // we do for double precision. To support that we use neon (which is FTZ)
      // for single precision floating points, and use the VFP with denormal
      // support enabled for doubles. The neonfp feature enables the use of neon
      // for single precision floating points.
      Features.AddFeature("strict-align", true);
      Features.AddFeature("neonfp", true);
      Features.AddFeature("neon", true);
      // Hardware division instructions might not exist on all ARMv7 CPUs, but
      // they probably exist on all the ones we might care about.
      Features.AddFeature("hwdiv", true);
      Features.AddFeature("hwdiv-arm", true);
      if (host_device_info.half_capabilities) {
        Features.AddFeature("fp16", true);
      }
      break;
#endif
#ifdef HOST_LLVM_RISCV
    case llvm::Triple::riscv32:
    case llvm::Triple::riscv64:
      CPU = triple.getArch() == llvm::Triple::riscv32 ? "generic-rv32"
                                                      : "generic-rv64";
      // The following features are important for OpenCL, and generally
      // constitute a minimum requirement for non-embedded profile. Without
      // these features, we'd need compiler-rt support. Atomics are absolutely
      // essential.
      Features.AddFeature("m", true);  // Integer multiplication and division
      Features.AddFeature("f", true);  // Floating point support
      Features.AddFeature("a", true);  // Atomics
#if defined(CA_HOST_ENABLE_FP64)
      Features.AddFeature("d", true);  // Double support
#endif
#if defined(CA_HOST_ENABLE_FP16)
      Features.AddFeature("zfh", true);  // Half support
#endif
      break;
#endif
#ifdef HOST_LLVM_X86
    case llvm::Triple::x86:
    case llvm::Triple::x86_64:
      CPU = "x86-64-v3";
      break;
#endif
    default:
      break;
  }

  auto SetCPUFeatures = [&](std::string NewCPU, std::string NewFeatures) {
    if (!NewCPU.empty()) {
      CPU = NewCPU;
      Features = llvm::SubtargetFeatures();
    }
    if (!NewFeatures.empty()) {
      std::vector<std::string> NewFeatureVector;
      llvm::SubtargetFeatures::Split(NewFeatureVector, NewFeatures);
      for (auto &NewFeature : NewFeatureVector) Features.AddFeature(NewFeature);
    }
  };

  switch (triple.getArch()) {
#ifdef HOST_LLVM_ARM
    case llvm::Triple::arm:
#ifndef CA_HOST_TARGET_ARM_CPU
#define CA_HOST_TARGET_ARM_CPU ""
#endif
#ifndef CA_HOST_TARGET_ARM_FEATURES
#define CA_HOST_TARGET_ARM_FEATURES ""
#endif
      SetCPUFeatures(CA_HOST_TARGET_ARM_CPU, CA_HOST_TARGET_ARM_FEATURES);
      break;
#endif
#ifdef HOST_LLVM_AARCH64
    case llvm::Triple::aarch64:
#ifndef CA_HOST_TARGET_AARCH64_CPU
#define CA_HOST_TARGET_AARCH64_CPU ""
#endif
#ifndef CA_HOST_TARGET_AARCH64_FEATURES
#define CA_HOST_TARGET_AARCH64_FEATURES ""
#endif
      SetCPUFeatures(CA_HOST_TARGET_AARCH64_CPU,
                     CA_HOST_TARGET_AARCH64_FEATURES);
      break;
#endif
#ifdef HOST_LLVM_RISCV
    case llvm::Triple::riscv32:
#ifndef CA_HOST_TARGET_RISCV32_CPU
#define CA_HOST_TARGET_RISCV32_CPU ""
#endif
#ifndef CA_HOST_TARGET_RISCV32_FEATURES
#define CA_HOST_TARGET_RISCV32_FEATURES ""
#endif
      SetCPUFeatures(CA_HOST_TARGET_RISCV32_CPU,
                     CA_HOST_TARGET_RISCV32_FEATURES);
      break;
    case llvm::Triple::riscv64:
#ifndef CA_HOST_TARGET_RISCV64_CPU
#define CA_HOST_TARGET_RISCV64_CPU ""
#endif
#ifndef CA_HOST_TARGET_RISCV64_FEATURES
#define CA_HOST_TARGET_RISCV64_FEATURES ""
#endif
      SetCPUFeatures(CA_HOST_TARGET_RISCV64_CPU,
                     CA_HOST_TARGET_RISCV64_FEATURES);
      break;
#endif
#ifdef HOST_LLVM_X86
    case llvm::Triple::x86:
#ifndef CA_HOST_TARGET_X86_CPU
#define CA_HOST_TARGET_X86_CPU ""
#endif
#ifndef CA_HOST_TARGET_X86_FEATURES
#define CA_HOST_TARGET_X86_FEATURES ""
#endif
      SetCPUFeatures(CA_HOST_TARGET_X86_CPU, CA_HOST_TARGET_X86_FEATURES);
      break;
    case llvm::Triple::x86_64:
#ifndef CA_HOST_TARGET_X86_64_CPU
#define CA_HOST_TARGET_X86_64_CPU ""
#endif
#ifndef CA_HOST_TARGET_X86_64_FEATURES
#define CA_HOST_TARGET_X86_64_FEATURES ""
#endif
      SetCPUFeatures(CA_HOST_TARGET_X86_64_CPU, CA_HOST_TARGET_X86_64_FEATURES);
      break;
#endif
    default:
      break;
  }

#if !defined(NDEBUG) || defined(CA_ENABLE_DEBUG_SUPPORT)
  {
    auto GetEnv = [](const char *Name) -> std::string {
      auto *Value = std::getenv(Name);
      return Value ? Value : "";
    };
    SetCPUFeatures(GetEnv("CA_HOST_TARGET_CPU"),
                   GetEnv("CA_HOST_TARGET_FEATURES"));
    ;
  }
#endif

  if (CPU == "native") {
    CPU = llvm::sys::getHostCPUName();

    llvm::SubtargetFeatures NativeFeatures;

#if LLVM_VERSION_GREATER_EQUAL(19, 0)
    auto FeatureMap = llvm::sys::getHostCPUFeatures();
#else
    llvm::StringMap<bool> FeatureMap;
    llvm::sys::getHostCPUFeatures(FeatureMap);
#endif
    for (auto &[FeatureName, IsEnabled] : FeatureMap) {
      NativeFeatures.AddFeature(FeatureName, IsEnabled);
    }

    NativeFeatures.addFeaturesVector(Features.getFeatures());
    Features = std::move(NativeFeatures);
  }

  if (compiler_info->supports_deferred_compilation()) {
    llvm::orc::JITTargetMachineBuilder TMBuilder(triple);
    TMBuilder.setCPU(CPU);
    TMBuilder.setCodeGenOptLevel(multi_llvm::CodeGenOptLevel::Aggressive);
    TMBuilder.getFeatures().addFeaturesVector(Features.getFeatures());
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

          // Register the GDB JIT event listener.
          ObjLinkingLayer->registerJITEventListener(*gdb_registration_listener);

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
  } else {
    // No JIT support so create target machine directly.
    target_machine.reset(
        createTargetMachine(triple, CPU, Features.getString()));
  }

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

llvm::Module *HostTarget::getBuiltins() const { return builtins.get(); }

}  // namespace host
