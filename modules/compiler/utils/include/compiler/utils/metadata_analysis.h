// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Metadata Analysis.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_METADATA_ANALYSIS_H_INCLUDED
#define COMPILER_UTILS_METADATA_ANALYSIS_H_INCLUDED

#include <compiler/utils/metadata.h>
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Printable.h>
#include <metadata/handler/generic_metadata.h>
#include <metadata/handler/vectorize_info_metadata.h>
#include <multi_llvm/optional_helper.h>

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
