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

#ifndef HOST_UTILS_RELOCATIONS_H_INCLUDED
#define HOST_UTILS_RELOCATIONS_H_INCLUDED

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace host {
namespace utils {

#if defined(__riscv)
#define HOST_UTILS_HAVE_RELOCATIONS 0
inline std::vector<std::pair<std::string, uint64_t>> getRelocations() {
  return {};
}
#else
#define HOST_UTILS_HAVE_RELOCATIONS 1
/// @brief Returns the function relocations required to execute host kernels.
///
/// Returns a list of pairs of mangled function symbol name and function address
/// in memory, e.g.:
///
///   {"memcpy",   0xf00f00f0},
///   {"__divdi3", 0xb00b00b0},
///
/// The list of functions should be complete such that all external symbols that
/// may possibly be called from a host-compiled kernel executable will be
/// resolved.
std::vector<std::pair<std::string, uint64_t>> getRelocations();
#endif  //  !defined(__riscv)

}  // namespace utils
}  // namespace host

#endif  // HOST_UTILS_RELOCATIONS_H_INCLUDED
