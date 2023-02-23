// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Reduce-to-function pass
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_REDUCE_TO_FUNCTION_PASS_H_INCLUDED
#define COMPILER_UTILS_REDUCE_TO_FUNCTION_PASS_H_INCLUDED

#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass which removes dead functions not used by the target kernel.
///
/// The LLVM module when passed to scheduled kernel can contain multiple kernel
/// functions present in the device side program, however by this stage of
/// compilation we are only interested in running a single kernel. In order to
/// improve the speed of subsequent passes and reduce code size, this pass
/// removes dead functions not used by the target kernel. On pass creation
/// `ReduceToFunction` takes a string list of functions names to preserve, which
/// will include the name of our enqueued kernel and potentially some internal
/// functions needed for later passes, like DMA preload.
///
/// Runs over all kernels with "kernel" metadata.
class ReduceToFunctionPass final
    : public llvm::PassInfoMixin<ReduceToFunctionPass> {
 public:
  ReduceToFunctionPass() {}
  ReduceToFunctionPass(const llvm::ArrayRef<llvm::StringRef> &RefNames) {
    llvm::transform(RefNames, std::back_inserter(Names),
                    [](llvm::StringRef N) { return N.str(); });
  }

  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);

 private:
  llvm::SmallVector<std::string, 4> Names;
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REDUCE_TO_FUNCTION_PASS_H_INCLUDED
