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

#include <base/context.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Pass.h>
#include <spirv-ll/context.h>
#include <spirv-ll/module.h>

#if !defined(NDEBUG) || defined(CA_ENABLE_LLVM_OPTIONS_IN_RELEASE)
#include <compiler/utils/llvm_global_mutex.h>
#include <llvm/Support/CommandLine.h>
#endif

namespace compiler {
BaseContext::BaseContext() {
#if !defined(NDEBUG) || defined(CA_ENABLE_LLVM_OPTIONS_IN_RELEASE)
  static std::once_flag parseEnvironmentOptionsFlag;
  std::call_once(parseEnvironmentOptionsFlag, [this]() {
    const char *argv[] = {"ComputeAortaCL"};
    const std::scoped_lock lock(compiler::utils::getLLVMGlobalMutex());
    llvm::cl::ParseCommandLineOptions(1, argv, "", /*Errs=*/nullptr,
#if LLVM_VERSION_GREATER_EQUAL(22, 0)
                                      /*VFS=*/nullptr,
#endif
                                      "CA_LLVM_OPTIONS");

    llvm_time_passes = llvm::TimePassesIsEnabled;

    llvm_verify_each = compiler::utils::VerifyEachIsEnabled;

    llvm_debug_passes = compiler::utils::DebugPasses;
  });
#endif
}

BaseContext::~BaseContext() {}

bool BaseContext::isValidSPIRV(cargo::array_view<const uint32_t> code) {
  if (code.size() < 1) {
    // Need at least one word in `code` to be able to check the magic number.
    return false;
  }

  auto moduleHeader = spirv_ll::ModuleHeader{{code.data(), code.size()}};
  return moduleHeader.isValid();
}

cargo::expected<spirv::SpecializableConstantsMap, std::string>
BaseContext::getSpecializableConstants(cargo::array_view<const uint32_t> code) {
  auto specializable =
      spirv_ll::Context{}.getSpecializableConstants({code.data(), code.size()});
  if (!specializable) {
    return cargo::make_unexpected(specializable.error().message);
  }

  spirv::SpecializableConstantsMap constants_map;
  for (const auto &entry : *specializable) {
    auto &constant = constants_map[entry.getFirst()];
    switch (entry.getSecond().constantType) {
      case spirv_ll::SpecializationType::BOOL:
        constant.constant_type = spirv::SpecializationType::BOOL;
        break;
      case spirv_ll::SpecializationType::INT:
        constant.constant_type = spirv::SpecializationType::INT;
        break;
      case spirv_ll::SpecializationType::FLOAT:
        constant.constant_type = spirv::SpecializationType::FLOAT;
        break;
    }
    constant.size_in_bits = entry.getSecond().sizeInBits;
  }
  return {std::move(constants_map)};
}
}  // namespace compiler
