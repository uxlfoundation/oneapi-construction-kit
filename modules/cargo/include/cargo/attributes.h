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
/// @brief C++ attribute detection.

#ifndef CARGO_ATTRIBUTE_H_INCLUDED
#define CARGO_ATTRIBUTE_H_INCLUDED

#ifdef __has_cpp_attribute

/// @def CARGO_REINITIALIZES
/// @brief Clang-specific `[[clang::reinitializes]]` attribute.
#if __has_cpp_attribute(clang::reinitializes)
#define CARGO_REINITIALIZES [[clang::reinitializes]]
#endif  // __has_cpp_attribute(clang::reinitializes)

#endif  // __has_cpp_attribute

// Ensure that all attribute macros are defined even when the attribute is not
// supported by the compiler.

#ifndef CARGO_REINITIALIZES
#define CARGO_REINITIALIZES
#endif  // CARGO_REINITIALIZES

#endif  // CARGO_ATTRIBUTE_H_INCLUDED
