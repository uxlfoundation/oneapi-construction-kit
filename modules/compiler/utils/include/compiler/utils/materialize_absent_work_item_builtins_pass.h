// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Materialize missing builtins.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_MATERIALIZE_ABSENT_WORK_ITEM_BUILTINS_PASS_H_INCLUDED
#define COMPILER_UTILS_MATERIALIZE_ABSENT_WORK_ITEM_BUILTINS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A small utility pass that implements the body of a few specific
/// builtins in terms of `__mux` builtins which are added to the module in this
/// pass.
///
/// The LLVM IR produced by spirv-ll can emit calls to the following builtins:
/// 1) size_t get_global_linear_id() -> _Z20get_global_linear_idv.
/// 2) size_t get_local_linear_id() -> _Z19get_local_linear_idv.
/// 3) size_t get_enqueued_local_size(uint) -> _Z23get_enqueued_local_sizej.
///
/// However, because these correspond to OpenCL2.X builtins, and since this IR
/// can be produced even when targeting a 1.2 driver, we need to add the
/// corresponding `__mux` builtins inside the compiler. The alternative
/// would be to unconditionally include definitions of these functions in the
/// headers that are used to produce the module of builtins linked in
/// `LinkBuiltins.cpp`, this is what we currently do for 1.2 builtins.
///
/// Note that the `__mux` builtins themselves are not implemented in this pass.
/// Like the rest of the builtins this happens later in the pipeline and is
/// split between the DefineMuxBuiltinsPass after the appropriate
/// scheduling parameters to the functions that require them by the
/// AddSchedulingParametersPass.
class MaterializeAbsentWorkItemBuiltinsPass final
    : public llvm::PassInfoMixin<MaterializeAbsentWorkItemBuiltinsPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_MATERIALIZE_ABSENT_WORK_ITEM_BUILTINS_PASS_H_INCLUDED
