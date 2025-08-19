// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef UNITCL_CALLBACKS_H_INCLUDED
#define UNITCL_CALLBACKS_H_INCLUDED

#include <CL/cl.h>

namespace ucl {
/// @brief Callback for use during context creation.
///
/// Prints the `errinfo` string to `stderr`, example usage:
///
/// ```cpp
/// cl_int error;
/// cl_context context = clCreateContext(nullptr, 1, &device,
///                                      UCL::contextCallback, nullptr, &error);
/// ```
///
/// @param errinfo is a pointer to an error string.
/// @param private_info unused pointer to binary info.
/// @param cb unused size of binary info.
/// @param user_data unused pointer to user data.
void CL_CALLBACK contextCallback(const char *errinfo, const void *private_info,
                                 size_t cb, void *user_data);

/// @brief Callback for clBuildProgram to print build log.
///
/// @param program Program to print build log for.
void CL_CALLBACK buildLogCallback(cl_program program, void *);
}  // namespace ucl

#endif  // UNITCL_CALLBACKS_H_INCLUDED
