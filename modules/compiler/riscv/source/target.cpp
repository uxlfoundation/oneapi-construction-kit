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

#include <compiler/module.h>
#include <hal_riscv.h>
#include <llvm/Target/CodeGenCWrappers.h>
#include <llvm/Target/TargetMachine.h>
#include <multi_llvm/llvm_version.h>
#include <multi_llvm/multi_llvm.h>
#include <riscv/bakery.h>
#include <riscv/module.h>
#include <riscv/target.h>

namespace {
void addFeature(std::string &features, const char *feature, bool &hasFeature) {
  features += hasFeature ? "," : "";
  features += feature;
  hasFeature = true;
}

void setTargetFeatureString(const riscv::hal_device_info_riscv_t *info,
                            std::string &features) {
  bool hasFeature = false;
  if (info->extensions & riscv::rv_extension_M) {
    addFeature(features, "+m", hasFeature);
  }
  if (info->extensions & riscv::rv_extension_F) {
    addFeature(features, "+f", hasFeature);
  }
  if (info->extensions & riscv::rv_extension_A) {
    addFeature(features, "+a", hasFeature);
  }
  if (info->extensions & riscv::rv_extension_C) {
    addFeature(features, "+c", hasFeature);
  }
  if (info->extensions & riscv::rv_extension_D) {
    addFeature(features, "+d", hasFeature);
  }
  if (info->extensions & riscv::rv_extension_E) {
    addFeature(features, "+e", hasFeature);
  }

  if (info->extensions & riscv::rv_extension_V) {
    addFeature(features, "+v", hasFeature);
    if (info->vlen) {
      const std::string zvl = "+zvl" + std::to_string(info->vlen) + "b";
      addFeature(features, zvl.c_str(), hasFeature);
    }
  }
  if (info->extensions & riscv::rv_extension_Zfh) {
    addFeature(features, "+zfh", hasFeature);
    if (info->extensions & riscv::rv_extension_V) {
#if LLVM_VERSION_GREATER_EQUAL(17, 0)
      addFeature(features, "+zvfh", hasFeature);
#else
      addFeature(features, "+experimental-zvfh", hasFeature);
#endif
    }
  }
  if (info->extensions & riscv::rv_extension_Zba) {
    addFeature(features, "+zba", hasFeature);
  }
  if (info->extensions & riscv::rv_extension_Zbb) {
    addFeature(features, "+zbb", hasFeature);
  }
  if (info->extensions & riscv::rv_extension_Zbc) {
    addFeature(features, "+zbc", hasFeature);
  }
  if (info->extensions & riscv::rv_extension_Zbs) {
    addFeature(features, "+zbs", hasFeature);
  }
}

}  // namespace

namespace riscv {

RiscvTarget::RiscvTarget(const compiler::Info *compiler_info,
                         const hal_device_info_riscv_t *hal_device_info,
                         compiler::Context *context,
                         compiler::NotifyCallbackFn callback)
    : BaseAOTTarget(compiler_info, context, callback) {
  env_debug_prefix = "CA_RISCV";

  static std::once_flag llvm_initialized;
  std::call_once(llvm_initialized, [&]() {
    // Init llvm targets.
    LLVMInitializeRISCVTarget();
    LLVMInitializeRISCVTargetInfo();
    LLVMInitializeRISCVAsmPrinter();
    LLVMInitializeRISCVTargetMC();
    LLVMInitializeRISCVAsmParser();
  });

  riscv_hal_device_info = hal_device_info;

  if (riscv_hal_device_info->should_link) {
    if (riscv_hal_device_info->word_size == 64) {
      rt_lib = get_rtlib64_data();
      rt_lib_size = get_rtlib64_size();
    } else {
      rt_lib = get_rtlib32_data();
      rt_lib_size = get_rtlib32_size();
    }
  }

  setTargetFeatureString(riscv_hal_device_info, llvm_features);

  if (riscv_hal_device_info->word_size == 32) {
    llvm_triple = "riscv32-unknown-elf";
    llvm_cpu = "generic-rv32";
    llvm_abi = "ilp32d";
  } else {
    llvm_triple = "riscv64-unknown-elf";
    llvm_cpu = "generic-rv64";
    llvm_abi = "lp64d";
  }
  assert(riscv_hal_device_info->supports_doubles &&
         "ABI support only for RISC-V with double support");
}

RiscvTarget::~RiscvTarget() {}

compiler::Result RiscvTarget::initWithBuiltins(
    std::unique_ptr<llvm::Module> builtins_module) {
  builtins = std::move(builtins_module);

  return compiler::Result::SUCCESS;
}

std::unique_ptr<compiler::Module> RiscvTarget::createModule(
    uint32_t &num_errors, std::string &log) {
  return std::make_unique<RiscvModule>(
      *this, static_cast<compiler::BaseContext &>(context), num_errors, log);
}
}  // namespace riscv
