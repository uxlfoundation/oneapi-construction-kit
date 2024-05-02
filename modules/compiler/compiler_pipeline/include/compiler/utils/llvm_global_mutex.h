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

/// @file
///
/// @brief

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
