// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Compiler program info API.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BASE_PROGRAM_METADATA_H_INCLUDED
#define BASE_PROGRAM_METADATA_H_INCLUDED

#include <base/module.h>

namespace llvm {
class Module;
}  // namespace llvm

namespace compiler {
/// @addtogroup cl_compiler
/// @{

/// @brief Initialize information for all kernels by scanning a LLVM module.
///
/// @param[in] callback Callback to return kernel information.
/// @param[in] module The module to extract information from.
/// @param[in] store_argument_metadata Whether to store additional argument
/// metadata as required by -cl-kernel-arg-info.
///
/// @return Return a status code.
/// @retval `Result::SUCCESS` if program info extraction was successful.
/// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
/// @retval `Result::FINALIZE_PROGRAM_FAILURE` when there was a problem with the
/// LLVM IR.
Result moduleToProgramInfo(KernelInfoCallback callback,
                           llvm::Module *const module,
                           bool store_argument_metadata);

/// @}
}  // namespace compiler

#endif  // BASE_PROGRAM_METADATA_H_INCLUDED
