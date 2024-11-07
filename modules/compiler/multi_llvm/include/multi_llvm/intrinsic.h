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

#ifndef MULTI_LLVM_MULTI_INTRINSIC_H_INCLUDED
#define MULTI_LLVM_MULTI_INTRINSIC_H_INCLUDED

#include <multi_llvm/llvm_version.h>

namespace llvm {
namespace Intrinsic {
static inline auto getOrInsertDeclaration(...) { return nullptr; }
static inline auto getDeclaration(...) { return nullptr; }
}  // namespace Intrinsic
}  // namespace llvm

namespace multi_llvm {
namespace IntrinsicHelper {
template <bool>
struct SelectIf;
template <>
struct SelectIf<false> {
  template <typename T>
  auto operator()(T) {
    return nullptr;
  }
};
template <>
struct SelectIf<true> {
  template <typename T>
  auto operator()(T t) {
    return t;
  }
};
}  // namespace IntrinsicHelper

static inline llvm::Function *GetOrInsertIntrinsicDeclaration(
    llvm::Module *M, llvm::Intrinsic::ID id,
    llvm::ArrayRef<llvm::Type *> Tys = {}) {
  auto goid = llvm::Intrinsic::getOrInsertDeclaration(M, id, Tys);
  constexpr bool use_goid = std::is_same_v<decltype(goid), llvm::Function *>;
  if constexpr (use_goid) {
    return goid;
  } else {
    // Even if use_goid is true and this block is never executed, we need a
    // workaround to avoid a deprecation warning.
    IntrinsicHelper::SelectIf<!use_goid> SI;
    return llvm::Intrinsic::getDeclaration(SI(M), SI(id), SI(Tys));
  }
}

}  // namespace multi_llvm

#endif  // MULTI_LLVM_MULTI_INTRINSIC_H_INCLUDED
