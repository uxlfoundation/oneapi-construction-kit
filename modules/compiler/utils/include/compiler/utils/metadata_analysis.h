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

/// @file
///
/// @brief Metadata Analysis.

#ifndef COMPILER_UTILS_METADATA_ANALYSIS_H_INCLUDED
#define COMPILER_UTILS_METADATA_ANALYSIS_H_INCLUDED

#include <compiler/utils/metadata.h>
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Printable.h>
#include <metadata/handler/generic_metadata.h>
#include <metadata/handler/vectorize_info_metadata.h>

namespace compiler {
namespace utils {

template <typename T>
llvm::Printable print(FixedOrScalableQuantity<T> Q) {
  return llvm::Printable([Q](llvm::raw_ostream &Out) {
    if (Q.isScalable()) {
      Out << "vscale x ";
    }
    Out << Q.getKnownMinValue();
  });
}

class GenericMetadataAnalysis
    : public llvm::AnalysisInfoMixin<GenericMetadataAnalysis> {
  friend AnalysisInfoMixin<GenericMetadataAnalysis>;

 public:
  using Result = handler::GenericMetadata;
  GenericMetadataAnalysis() = default;

  Result run(llvm::Function &Fn, llvm::FunctionAnalysisManager &);

  /// @brief Return the name of the pass.
  static llvm::StringRef name() { return "Generic Metadata analysis"; }

 private:
  /// @brief Unique identifier for the pass.
  static llvm::AnalysisKey Key;
};

class GenericMetadataPrinterPass
    : public llvm::PassInfoMixin<GenericMetadataPrinterPass> {
  llvm::raw_ostream &OS;

 public:
  explicit GenericMetadataPrinterPass(llvm::raw_ostream &OS) : OS(OS) {}

  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

class VectorizeMetadataAnalysis
    : public llvm::AnalysisInfoMixin<VectorizeMetadataAnalysis> {
  friend AnalysisInfoMixin<VectorizeMetadataAnalysis>;

 public:
  using Result = handler::VectorizeInfoMetadata;
  VectorizeMetadataAnalysis() = default;

  Result run(llvm::Function &Fn, llvm::FunctionAnalysisManager &AM);

  /// @brief Return the name of the pass.
  static llvm::StringRef name() { return "Vectorize Metadata analysis"; }

 private:
  /// @brief Unique identifier for the pass.
  static llvm::AnalysisKey Key;
};

class VectorizeMetadataPrinterPass
    : public llvm::PassInfoMixin<VectorizeMetadataPrinterPass> {
  llvm::raw_ostream &OS;

 public:
  explicit VectorizeMetadataPrinterPass(llvm::raw_ostream &OS) : OS(OS) {}

  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_METADATA_ANALYSIS_H_INCLUDED
