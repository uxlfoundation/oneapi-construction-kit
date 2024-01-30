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

#include <compiler/utils/metadata_hooks.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/Casting.h>
#include <metadata/metadata.h>
#include <multi_llvm/multi_llvm.h>

namespace compiler {
namespace utils {

md_hooks getElfMetadataWriteHooks() {
  md_hooks md_hooks{};

  md_hooks.finalize = [](void *) {};

  md_hooks.write = [](void *userdata, const void *src, size_t n) -> md_err {
    auto *M = static_cast<llvm::Module *>(userdata);
    auto &Ctx = M->getContext();
    const std::string globalName = std::string(MD_NOTES_SECTION) + "_global";

    auto *GlobalMD = M->getGlobalVariable(globalName);
    if (GlobalMD) {
      auto *OldData =
          llvm::dyn_cast<llvm::ConstantDataArray>(GlobalMD->getInitializer());
      auto OldBytes = OldData->getRawDataValues();

      // Append data
      llvm::SmallVector<uint8_t, 100> Data{OldBytes.begin(), OldBytes.end()};
      Data.insert(Data.end(), (uint8_t *)src, (uint8_t *)src + n);

      GlobalMD->eraseFromParent();

      auto *MDTy =
          llvm::ArrayType::get(llvm::Type::getInt8Ty(Ctx), Data.size());
      auto *MDInit = llvm::ConstantDataArray::get(
          Ctx, llvm::ArrayRef(Data.data(), Data.size()));

      GlobalMD = llvm::dyn_cast_or_null<llvm::GlobalVariable>(
          M->getOrInsertGlobal(globalName, MDTy));
      GlobalMD->setInitializer(MDInit);
    } else {
      auto MDDataArr = llvm::ArrayRef((uint8_t *)src, n);
      auto *MDTy =
          llvm::ArrayType::get(llvm::Type::getInt8Ty(Ctx), MDDataArr.size());
      auto *MDInit = llvm::ConstantDataArray::get(Ctx, MDDataArr);
      GlobalMD = llvm::dyn_cast_or_null<llvm::GlobalVariable>(
          M->getOrInsertGlobal(globalName, MDTy));
      GlobalMD->setInitializer(MDInit);
    }

    GlobalMD->setAlignment(llvm::Align(1));
    GlobalMD->setSection(MD_NOTES_SECTION);
    GlobalMD->setLinkage(llvm::GlobalValue::ExternalLinkage);
    GlobalMD->setConstant(true);
    return md_err::MD_SUCCESS;
  };

  return md_hooks;
}
}  // namespace utils
}  // namespace compiler
