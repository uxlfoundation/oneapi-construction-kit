// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_LLVM_GLOBAL_MUTEX_H_INCLUDED
#define COMPILER_UTILS_LLVM_GLOBAL_MUTEX_H_INCLUDED

#include <llvm/Support/CommandLine.h>

#include <mutex>

namespace compiler {
namespace utils {
/// @brief Get the mutex for protecting access to LLVM's global state.
///
/// LLVM contains static global state to implement its command-line option
/// parsing which can be written to at any time a number of different entry
/// points. This is problematic when those entry points are being invoked in a
/// multi-threaded context such as ComputeAorta which can result in data races
/// and in extreme cases deadlocks.
///
/// @return Returns a reference to the global LLVM mutex object.
std::mutex &getLLVMGlobalMutex();
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_LLVM_GLOBAL_MUTEX_H_INCLUDED
