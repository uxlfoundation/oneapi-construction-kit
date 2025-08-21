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

/// @file
///
/// @brief Implementation of cl_codeplay_soft_math extension.

#ifndef EXTENSION_CODEPLAY_SOFT_MATH_H_INCLUDED
#define EXTENSION_CODEPLAY_SOFT_MATH_H_INCLUDED

#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief Definition of cl_codeplay_soft_math extension.
class codeplay_soft_math final : public extension {
public:
  /// @brief Default constructor.
  codeplay_soft_math();
};

/// @}
} // namespace extension

#endif // EXTENSION_CODEPLAY_SOFT_MATH_H_INCLUDED
