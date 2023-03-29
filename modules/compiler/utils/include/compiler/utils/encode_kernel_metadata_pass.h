// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// EncodeKernelMetadataPass pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_ENCODE_KERNEL_METADATA_PASS_H_INCLUDED
#define COMPILER_UTILS_ENCODE_KERNEL_METADATA_PASS_H_INCLUDED

#include <llvm/ADT/Optional.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/PassManager.h>
#include <multi_llvm/optional_helper.h>

namespace compiler {
namespace utils {

/// @brief Sets up the per-function mux metadata used by later passes.
/// Transfers per-module !opencl.kernel metadata to mux kernel metadata.
struct TransferKernelMetadataPass
    : public llvm::PassInfoMixin<TransferKernelMetadataPass> {
  explicit TransferKernelMetadataPass() {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

struct EncodeKernelMetadataPassOptions {
  std::string KernelName;
  multi_llvm::Optional<std::array<uint64_t, 3>> LocalSizes = multi_llvm::None;
};

struct EncodeKernelMetadataPass
    : public llvm::PassInfoMixin<EncodeKernelMetadataPass> {
  EncodeKernelMetadataPass(EncodeKernelMetadataPassOptions Options)
      : KernelName(Options.KernelName), LocalSizes(Options.LocalSizes) {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);

 private:
  std::string KernelName;
  multi_llvm::Optional<std::array<uint64_t, 3>> LocalSizes;
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_ENCODE_KERNEL_METADATA_PASS_H_INCLUDED
