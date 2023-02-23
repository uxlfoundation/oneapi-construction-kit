// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// EncodeBuiltinRangeMetadataPass pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_ENCODE_BUILTIN_RANGE_METADATA_PASS_H_INCLUDED
#define COMPILER_UTILS_ENCODE_BUILTIN_RANGE_METADATA_PASS_H_INCLUDED

#include <llvm/ADT/Optional.h>
#include <llvm/IR/PassManager.h>

#include <array>

namespace compiler {
namespace utils {

struct EncodeBuiltinRangeMetadataOptions {
  std::array<llvm::Optional<uint64_t>, 3> MaxLocalSizes;
  std::array<llvm::Optional<uint64_t>, 3> MaxGlobalSizes;
};

struct EncodeBuiltinRangeMetadataPass
    : public llvm::PassInfoMixin<EncodeBuiltinRangeMetadataPass> {
  EncodeBuiltinRangeMetadataPass(EncodeBuiltinRangeMetadataOptions Opts)
      : MaxLocalSizes(Opts.MaxLocalSizes),
        MaxGlobalSizes(Opts.MaxGlobalSizes) {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);

 private:
  std::array<llvm::Optional<uint64_t>, 3> MaxLocalSizes;
  std::array<llvm::Optional<uint64_t>, 3> MaxGlobalSizes;
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_ENCODE_BUILTIN_RANGE_METADATA_PASS_H_INCLUDED
